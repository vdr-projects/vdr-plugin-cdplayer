
#include "bufferedcdio.h"

const char *cBufferedCdio::cd_text_field[MAX_CDTEXT_FIELDS+1] = {
        tr("ARRANGER"),     /**< name(s) of the arranger(s) */
        tr("COMPOSER"),     /**< name(s) of the composer(s) */
        tr("DISCID"),       /**< disc identification information */
        tr("GENRE"),        /**< genre identification and genre information */
        tr("MESSAGE"),      /**< ISRC code of each track */
        tr("ISRC"),         /**< message(s) from the content provider or artist */
        tr("PERFORMER"),    /**< name(s) of the performer(s) */
        tr("SIZE_INFO"),    /**< size information of the block */
        tr("SONGWRITER"),   /**< name(s) of the songwriter(s) */
        tr("TITLE"),        /**< title of album name or track titles */
        tr("TOC_INFO"),     /**< table of contents information */
        tr("TOC_INFO2"),    /**< second table of contents information */
        tr("UPC_EAN"),
        tr("Invalid")
};

cBufferedCdio::cBufferedCdio(void) :
        mRingBuffer(CCDIO_MAX_BLOCKS)
{
    pCdio = NULL;
    mCurrTrackIdx = 0;
};

cBufferedCdio::~cBufferedCdio(void)
{
    CloseDevice();
}

const char *cBufferedCdio::GetCdTextField(const cdtext_field_t type)
{
    if (type > MAX_CDTEXT_FIELDS) {
        return cd_text_field[CDTEXT_INVALID];
    }
    return cd_text_field[type];
}

void cBufferedCdio::GetCDText (const track_t track_no, StringMap &cd_text)
{
    int i;
    const cdtext_t *cdtext = cdio_get_cdtext(pCdio, track_no);
    if (cdtext == NULL) {
        return;
    }
    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        if (cdtext->field[i] != NULL) {
            cd_text[i] = cdtext->field[i];
            dsyslog ("CD-Text %d: %s", i, cdtext->field[i]);
        }
    }
}

void cBufferedCdio::CloseDevice(void)
{
    if (pCdio != NULL) {
        cdio_destroy(pCdio);
    }
    mCdText.clear();
    mTrackInfo.clear();
    mRingBuffer.Clear();
    mCurrTrackIdx = 0;
}

bool cBufferedCdio::GetData (uint8_t *data)
{
    bool ret;
    if (pCdio == NULL) {
        return false;
    }
    ret = mRingBuffer.GetBlock(data);
    if (!ret) {
        if (Active()) {
            esyslog("%s %d Timeout", __FILE__, __LINE__);
        }
        else {
            dsyslog("%s %d Thread ends", __FILE__, __LINE__);
        }
    }
    return ret;
}

bool cBufferedCdio::OpenDevice (const string &FileName)
{
    CloseDevice();
    pCdio = cdio_open(FileName.c_str(), DRIVER_DEVICE);
    if (pCdio == NULL) {
        esyslog("%s %d Can not open %s", __FILE__, __LINE__, FileName.c_str());
        return false;
    }

    mFirstTrackNum = cdio_get_first_track_num(pCdio);
    mNumOfTracks = cdio_get_num_tracks(pCdio);
    printf("CD-ROM Track List (%d - %d)\n", mFirstTrackNum, mNumOfTracks);

    GetCDText (0, mCdText);
    for (int i = 0; i < mNumOfTracks; i++) {
        track_t track_no = (track_t)i + mFirstTrackNum;
        lsn_t startlsn = cdio_get_track_lsn(pCdio, track_no);
        lsn_t endlsn = cdio_get_track_last_lsn(pCdio, track_no);
        track_format_t fmt = cdio_get_track_format(pCdio, track_no);
        if ((fmt == TRACK_FORMAT_AUDIO) && (startlsn != CDIO_INVALID_LSN)) {
            dsyslog ("get_track_info for track %d S %d E %d",
                     track_no, startlsn, endlsn);
            cTrackInfo ti(startlsn, endlsn);
            GetCDText (track_no, ti.mCdTextFields);
            mTrackInfo.push_back(ti);
        }
    }
    return true;
}

bool cBufferedCdio::ReadTrack (TRACK_IDX_T trackidx)
{
    uint8_t buf[CDIO_CD_FRAMESIZE_RAW];
    cTrackInfo ti = GetTrackInfo(trackidx);
    lsn_t currlsn = ti.GetCDDAStartLsn();
    dsyslog("%s %d Read Track %d Start %d End %d",
            __FILE__, __LINE__, trackidx, currlsn, ti.GetCDDAEndLsn());
    while (currlsn < ti.GetCDDAEndLsn()) {
        if (cdio_read_audio_sectors(pCdio, buf, currlsn, 1) != DRIVER_OP_SUCCESS) {
            return false;
        }
        mRingBuffer.PutBlock(buf);
        currlsn++;
        if (!Running()) {
            return false;
        }
        if (mTrackChange) {
            return true;
        }
    }
    return true;
}
//
// Thread for reading from CDDA
//


void cBufferedCdio::Action(void)
{
    TRACK_IDX_T numTracks = GetNumTracks();
    mRingBuffer.Clear();
    mCurrTrackIdx=0;
    while (mCurrTrackIdx < numTracks) {
        mTrackChange = false;
        if (!ReadTrack (mCurrTrackIdx)) {
            return;
        }
        if (!Running()) {
            return;
        }
        if (mTrackChange) {
            mRingBuffer.Clear();
        }
        else {
            mCurrTrackIdx++;
        }
    }
}

void cBufferedCdio::SetTrack (TRACK_IDX_T newtrack)
{
  if (newtrack > GetNumTracks()) {
      return;
  }
  mCurrTrackIdx = newtrack;
  mTrackChange = true;
}
