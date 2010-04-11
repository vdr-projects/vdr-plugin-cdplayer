#include "cdplayer.h"
#include "pes_audio_converter.h"
#include "bufferedcdio.h"

cCdControl::cCdControl(void)
    :cControl(mCdPlayer = new cCdPlayer)
{

}

void cCdControl::Hide(void)
{
    printf("Hide\n");
}

cOsdObject *cCdControl::GetInfo(void)
{
    printf("GetInfo\n");
    return NULL;
}

eOSState cCdControl::ProcessKey(eKeys Key)
{
    printf("ProcessKey %d\n",Key);

    switch (Key) {
    case kFastFwd:
        mCdPlayer->SpeedFaster();
        break;
    case kFastRew:
        mCdPlayer->SpeedSlower();
        break;
    case kPlay:
        mCdPlayer->SpeedNormal();
        break;
    case kRight:
    case kNext:
        mCdPlayer->NextTrack();
        break;
    case kLeft:
    case kPrev:
        mCdPlayer->PrevTrack();
        break;
    case kOk:
    case kMenu:
    case kStop:
        mCdPlayer->Stop();
        return (osEnd);
    default:
        break;
    }
    return (osContinue);
}

// ------------- Player -----------------------

const PCM_FREQ_T cCdPlayer::mSpeedTypes[] = {PCM_FREQ_44100, PCM_FREQ_48000, PCM_FREQ_96000};

cCdPlayer::cCdPlayer(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    pStillBuf = NULL;
    mStillBufLen = 0;
    mSpeed = 0;
}

cCdPlayer::~cCdPlayer()
{
    free (pStillBuf);
}

void cCdPlayer::DisplayStillPicture (void)
{
    if (pStillBuf != NULL) {
        DeviceStillPicture((const uchar *)pStillBuf, mStillBufLen);
    }
}
void cCdPlayer::LoadStillPicture (char *FileName)
{
    int fd;
    int len;
    int size = MAXFRAMESIZE;

    if (pStillBuf != NULL) {
        free (pStillBuf);
    }
    pStillBuf = NULL;
    mStillBufLen = 0;

    fd = open(FileName, O_RDONLY);
    if (fd < 0) {
        esyslog("%s %d Can not open still picture %s",
                __FILE__, __LINE__, FileName);
        return;
    }

    pStillBuf = (uchar *)malloc (MAXFRAMESIZE);
    if (pStillBuf == NULL) {
        esyslog("%s %d Out of memory", __FILE__, __LINE__);
        close(fd);
        return;
    }
    do {
        len = read(fd, &pStillBuf[mStillBufLen], MAXFRAMESIZE);
        if (len < 0) {
            esyslog ("%s %d read error %d", __FILE__, __LINE__, errno);
            close(fd);
            return;
        }
        if (len > 0) {
            mStillBufLen += len;
            if (mStillBufLen >= size) {
                size += MAXFRAMESIZE;
                pStillBuf = (uchar *) realloc(pStillBuf, size);
                if (pStillBuf == NULL) {
                    close(fd);
                    esyslog("%s %d Out of memory", __FILE__, __LINE__);
                    return;
                }
            }
        }
    } while (len > 0);
    close(fd);
    DisplayStillPicture();
}

void cCdPlayer::Activate(bool On)
{
    printf("%.16s (%d) Activate On=%s\n", __FILE__, __LINE__, On ? "true" : "false");
    if (On) {
        printf("\n\nStarting Player\n");
        LoadStillPicture ((const char *)"/video/conf/plugins/radio/radio.mpg");
        Start ();
        DevicePlay();
    }
    else {
        Cancel(0);
    }
}

#define FRAME_DIV 4

void cCdPlayer::Action(void)
{
    cPesAudioConverter converter;
    cPoller oPoller;

    uint8_t buf[CDIO_CD_FRAMESIZE_RAW];
    const uchar *pesdata;
    int peslen;
    int chunksize = CDIO_CD_FRAMESIZE_RAW / FRAME_DIV;

    cdio.OpenDevice ("/dev/sr0");
    cdio.Start();
    while (true) {
        if (!cdio.GetData(buf)) {
            cdio.Stop();
            return;
        }
        int i = 0;
        while (i < FRAME_DIV) {
            if (DevicePoll(oPoller, 1000)) {
                converter.SetFreq(mSpeedTypes[mSpeed]);
                converter.SetData(&buf[i * chunksize], chunksize);
                pesdata = converter.GetPesData();
                peslen = converter.GetPesLength();
                if (PlayPes(pesdata, peslen, false) < 0) {
                    cdio.Stop();
                    return;
                }
                i++;
            }
            if (!Running()) {
                cdio.Stop();
                return;
            }
        }
    }
}
