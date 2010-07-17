/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements buffered access to the audio cd via
 * libcdda.
 *
 */

#include "cdplayer.h"
#include "bufferedcdio.h"

// Translated description of the cd text field
const char *cBufferedCdio::cd_text_field[MAX_CDTEXT_FIELDS+1] = {
        "Arranger",     /**< name(s) of the arranger(s) */
        "Composer",     /**< name(s) of the composer(s) */
        "Disk ID",      /**< disc identification information */
        "Genre",        /**< genre identification and genre information */
        "Message",      /**< ISRC code of each track */
        "Isrc",         /**< message(s) from the content provider or artist */
        "Performer",    /**< name(s) of the performer(s) */
        "Size Info",    /**< size information of the block */
        "Songwriter",   /**< name(s) of the songwriter(s) */
        "Title",        /**< title of album name or track titles */
        "Info1",        /**< table of contents information */
        "Info2",        /**< second table of contents information */
        "Upc Ean",
        "Invalid"
};

cBufferedCdio::cBufferedCdio(void) :
        mRingBuffer(CCDIO_MAX_BLOCKS)
{
    cMutexLock MutexLock(&mCdMutex);
    pCdio = NULL;
    pParanoiaDrive = NULL;
    pParanoiaCd = NULL;
    mCurrTrackIdx = INVALID_TRACK_IDX;
    mState = BCDIO_STARTING;
    SetDescription("BufferedCdio");
    cd_text_field[CDTEXT_ARRANGER]  = tr("Arranger");
    cd_text_field[CDTEXT_COMPOSER]  = tr("Composer");
    cd_text_field[CDTEXT_DISCID]    = tr("Disk ID");
    cd_text_field[CDTEXT_GENRE]     = tr("Genre");
    cd_text_field[CDTEXT_MESSAGE]   = tr("Message");
    cd_text_field[CDTEXT_ISRC]      = tr("Isrc");
    cd_text_field[CDTEXT_PERFORMER] = tr("Performer");
    cd_text_field[CDTEXT_SIZE_INFO] = tr("Size Info");
    cd_text_field[CDTEXT_SONGWRITER] = tr("Songwriter");
    cd_text_field[CDTEXT_TITLE]     = tr("Title");
    cd_text_field[CDTEXT_TOC_INFO]  = tr("Info1");
    cd_text_field[CDTEXT_TOC_INFO2] = tr("Info2");
    cd_text_field[CDTEXT_UPC_EAN]   = tr("Upc Ean");
    cd_text_field[CDTEXT_INVALID]   = tr("Invalid");
};

cBufferedCdio::~cBufferedCdio(void)
{
    cMutexLock MutexLock(&mCdMutex);
    mState = BCDIO_STOP;
    if (Active()) {
        Cancel(3);
    }
#ifdef USE_PARANOIA
    pParanoiaDrive = NULL;
    if (pParanoiaCd != NULL) {
        cdio_paranoia_free(pParanoiaCd);
    }
#endif
    if (pCdio != NULL) {
        cdio_destroy(pCdio);
    }
}

const TRACK_IDX_T cBufferedCdio::GetCurrTrack(int *total, int *curr)
{
    if (total != NULL) {
        *total = (GetEndLsn(mCurrTrackIdx) - GetStartLsn(mCurrTrackIdx))
                            / CDIO_CD_FRAMES_PER_SEC;
    }
    if (curr != NULL) {
        *curr = (mCurrLsn - GetStartLsn(mCurrTrackIdx))  / CDIO_CD_FRAMES_PER_SEC;
    }
    return mCurrTrackIdx;
}
// Get name of a CD-Text field
const char *cBufferedCdio::GetCdTextField(const cdtext_field_t type)
{
    if (type > MAX_CDTEXT_FIELDS) {
        return cd_text_field[CDTEXT_INVALID];
    }
    return cd_text_field[type];
}

// Get all available CD-Text for a track
void cBufferedCdio::GetCDText (const track_t track_no, CD_TEXT_T &cd_text)
{
    int i;
    const cdtext_t *cdtext = cdio_get_cdtext(pCdio, track_no);
    if (cdtext == NULL) {
        dsyslog ("No CD-Text found");
        return;
    }
    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        if (cdtext->field[i] != NULL) {
            cd_text[i] = cdtext->field[i];
            dsyslog ("CD-Text %d: %s", i, cdtext->field[i]);
        }
    }
}

// Close access and destroy and reset all internal buffers
void cBufferedCdio::CloseDevice(void)
{
    cMutexLock MutexLock(&mCdMutex);
    mState = BCDIO_STOP;
#ifdef USE_PARANOIA
    pParanoiaDrive = NULL;
    if (pParanoiaCd != NULL) {
        cdio_paranoia_free(pParanoiaCd);
        pParanoiaCd = NULL;
    }
#endif
    if (pCdio != NULL) {
        cdio_destroy(pCdio);
        pCdio = NULL;
    }
    mCdInfo.Clear();
    mRingBuffer.Clear();
    mCurrTrackIdx = 0;
}

// Get a block of raw audio data from buffer
bool cBufferedCdio::GetData (uint8_t *data)
{
    if (pCdio == NULL) {
        return false;
    }
    if ((mState == BCDIO_FAILED) || (mState == BCDIO_STOP)) {
        return false;
    }
    while (!mRingBuffer.GetBlock(data))
    {
        if (!Running() || (pCdio == NULL) || (mState == BCDIO_FAILED) ||
            (mState == BCDIO_STOP)) {
            return false;
        }
    }
    return true;
}


bool cBufferedCdio::ParanoiaLogMsg (void)
{
    bool iserr = false;
    if (pParanoiaDrive != NULL) {
        char *err = cdio_cddap_errors(pParanoiaDrive);
        char *mes = cdio_cddap_messages(pParanoiaDrive);
        if (mes != NULL) {
            dsyslog (mes);
            free(mes);
        }
        if (err != NULL) {
            esyslog (err);
            free(err);
            iserr = true;
        }
    }
    return iserr;
}

// Open access to the audio cd and retrieve all available CD-Text
// information
bool cBufferedCdio::OpenDevice (const string &FileName)
{
    bool hasaudiotrack = false;
    string txt;
    CD_TEXT_T cdtxt;

    CloseDevice();
    cMutexLock MutexLock(&mCdMutex);
    mState = BCDIO_OPEN_DEVICE;
    pCdio = cdio_open(FileName.c_str(), DRIVER_DEVICE);
    if (pCdio == NULL) {
        mState = BCDIO_FAILED;
        txt = tr("Can not open");
        mErrtxt = txt  + " " + FileName;
        esyslog("%s %d Can not open %s", __FILE__, __LINE__, FileName.c_str());
        return false;
    }

#ifdef USE_PARANOIA
    pParanoiaDrive=cdio_cddap_identify_cdio(pCdio, 1, NULL);
    if (pParanoiaDrive == NULL) {
        esyslog ("Drive Init failed");
        CloseDevice();
        return false;
    }
    cdio_cddap_open (pParanoiaDrive);

    pParanoiaCd = cdio_paranoia_init(pParanoiaDrive);
    if (pParanoiaCd == NULL) {
        esyslog ("Paranoia Init failed");
        CloseDevice();
        return false;
    }
     /* Set reading mode for full paranoia, but allow skipping sectors. */
    cdio_paranoia_modeset(pParanoiaCd,
                             PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP);


#endif
    dsyslog("The driver selected is %s", cdio_get_driver_name(pCdio));
    dsyslog("The default device for this driver is %s",
            cdio_get_default_device(pCdio));

    mFirstTrackNum = cdio_get_first_track_num(pCdio);
    if (mFirstTrackNum == CDIO_INVALID_TRACK) {
        txt = tr("No disc in drive");
        mErrtxt = txt + " " + FileName;
        mState = BCDIO_FAILED;
        esyslog("%s %d Problem on read first track %s",
                __FILE__, __LINE__, FileName.c_str());
        return false;
    }
    mNumOfTracks = cdio_get_num_tracks(pCdio);
    if (mNumOfTracks == CDIO_INVALID_TRACK) {
        mState = BCDIO_FAILED;
        txt = tr("Problem on read");
        mErrtxt = txt + " " + FileName;
        esyslog("%s %d Problem on read no of tracks %s",
                __FILE__, __LINE__, FileName.c_str());
        return false;
    }
    dsyslog("CD-ROM Track List (%d - %d)\n", mFirstTrackNum, mNumOfTracks);

    GetCDText (0, cdtxt);
    mCdInfo.SetCdInfo (cdtxt);

    for (int i = 0; i < mNumOfTracks; i++) {
        track_t track_no = (track_t)i + mFirstTrackNum;
        lsn_t startlsn = cdio_get_track_lsn(pCdio, track_no);
        lsn_t endlsn = cdio_get_track_last_lsn(pCdio, track_no);
        lba_t lba = cdio_get_track_lba(pCdio, track_no);
        if (endlsn == CDIO_INVALID_LSN) {
            mState = BCDIO_FAILED;
            txt = tr("Problem on read lsn");
            mErrtxt = txt + " " + FileName;
            esyslog("%s %d Problem on read last lsn %s",
                     __FILE__, __LINE__, FileName.c_str());
            return false;
        }
        if (lba == CDIO_INVALID_LBA) {
            mState = BCDIO_FAILED;
            txt = tr("Problem on read lba");
            mErrtxt = txt + " " + FileName;
            esyslog("%s %d Problem on read lba %s",
                    __FILE__, __LINE__, FileName.c_str());
            return false;
        }
        track_format_t fmt = cdio_get_track_format(pCdio, track_no);
        if ((fmt == TRACK_FORMAT_AUDIO) && (startlsn != CDIO_INVALID_LSN)) {
            CD_TEXT_T cdtextfields;
            dsyslog ("get_track_info for track %d S %d E %d L %d",
                     track_no, startlsn, endlsn, lba);
            GetCDText (track_no, cdtextfields);
            mCdInfo.Add(startlsn, endlsn, lba, cdtextfields);
            hasaudiotrack = true;
        }
        else {
            mCdInfo.AddData(lba);
        }
    }
    if (!hasaudiotrack) {
        mState = BCDIO_FAILED;
        txt = tr("Not an audio disk");
        mErrtxt = txt + " " + FileName;
        esyslog("%s %d no audio track found %s",
                __FILE__, __LINE__, FileName.c_str());
        return false;
    }

    if (cPluginCdplayer::GetCDDBEnabled()) {
        mCdInfo.SetLeadOut (cdio_get_track_lba(pCdio, CDIO_CDROM_LEADOUT_TRACK));
        mCdInfo.Start(); // Start CDDB query
    }
    return true;
}

// Read an audio track from CD and buffer the output into ringbuffer
bool cBufferedCdio::ReadTrack (TRACK_IDX_T trackidx)
{
#ifdef USE_PARANOIA
    uint8_t *buf;
#else
    uint8_t buf[CDIO_CD_FRAMESIZE_RAW];
#endif

    mCurrLsn = mStartLsn;
    lsn_t endlsn = GetEndLsn(trackidx);
    dsyslog("%s %d Read Track %d Start %d End %d",
            __FILE__, __LINE__, trackidx, mCurrLsn, endlsn);
#ifdef USE_PARANOIA
    cdio_paranoia_seek(pParanoiaCd, mCurrLsn, SEEK_SET);
#endif
    while (mCurrLsn < endlsn) {
        if (mState == BCDIO_PAUSE) {
            cCondWait::SleepMs(250);
        }
        else {
            mCdMutex.Lock();
            if (pCdio == NULL) {
                mCdMutex.Unlock();
                mState = BCDIO_FAILED;
                return false;
            }
#ifdef USE_PARANOIA
            buf=(uint8_t *)cdio_paranoia_read(pParanoiaCd, NULL);
            if (ParanoiaLogMsg()) {
                mErrtxt = tr("Read error");
                mState = BCDIO_FAILED;
                mCdMutex.Unlock();
                return false;
            }
#else
            if (cdio_read_audio_sectors(pCdio, buf, mCurrLsn, 1)
                                                        != DRIVER_OP_SUCCESS) {
                mErrtxt = tr("Read error");
                mState = BCDIO_FAILED;
                mCdMutex.Unlock();
                return false;
            }
#endif
            mCurrLsn++;
            mCdMutex.Unlock();
            if (!Running()) {
                return false;
            }
            if (mTrackChange) {
                return true;
            }

            while (!mRingBuffer.PutBlock(buf)) {
                if (!Running()) {
                    return false;
                }
                if (mTrackChange) {
                    return true;
                }
            }
        }
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
    SetTrack(0);
    mState = BCDIO_PLAY;
    while (mCurrTrackIdx < numTracks) {
        if (!mTrackChange) {
            mStartLsn = GetStartLsn(mCurrTrackIdx);
        }
        mTrackChange = false;
        if (!ReadTrack (mCurrTrackIdx)) {
            mState = BCDIO_FAILED;
            return;
        }
        if (!Running()) {
            mState = BCDIO_STOP;
            return;
        }
        if (mTrackChange) {
            mRingBuffer.Clear();
        }
        else {
            mCurrTrackIdx++;
        }
    }
    mState = BCDIO_STOP;
}

// Set new track
void cBufferedCdio::SetTrack (TRACK_IDX_T newtrack)
{
  cMutexLock MutexLock(&mCdMutex);
  if (newtrack > GetNumTracks()-1) {
      return;
  }
  mStartLsn = GetStartLsn(newtrack);
  mCurrTrackIdx = newtrack;
  mTrackChange = true;
}

void cBufferedCdio::SkipTime(int tm) {
    cMutexLock MutexLock(&mCdMutex);
    lsn_t newlsn = mCurrLsn + (tm * CDIO_CD_FRAMES_PER_SEC);
    lsn_t lastlsn = GetEndLsn(GetNumTracks()-1)-CDIO_CD_FRAMES_PER_SEC;
    TRACK_IDX_T idx;
    TRACK_IDX_T newtrack = 0;

    if (newlsn < GetStartLsn(0)) {
        newlsn = GetStartLsn(0);
    }
    if (newlsn >= lastlsn) {
        newlsn = lastlsn;
    }
    // Find track which contains calculated lsn
    for (idx = 0; idx < GetNumTracks(); idx++) {
        if (newlsn < GetEndLsn(idx)) {
            newtrack = idx;
            break;
        }
    }
    mStartLsn = newlsn;
    mCurrTrackIdx = newtrack;
    mTrackChange = true;
}
