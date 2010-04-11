#include "cdplayer.h"
#include "pes_audio_converter.h"
#include "cdio.h"

cCdControl::cCdControl(void)
    :cControl(player = new cCdPlayer)
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
    static const char *na[] = {
            "kUp",
             "kDown",
             "kMenu",
             "kOk",
             "kBack",
             "kLeft",
             "kRight",
             "kRed",
             "kGreen",
             "kYellow",
             "kBlue"
    };
    const char *txt;
    if (Key <= kBlue) txt = na[Key];
    else txt = "?";
    printf("ProcessKey %d %s\n",Key, txt);
    if ((Key == kOk) || (Key == kMenu)){
        printf("End\n");
        return (osEnd);
    }
    return (osContinue);
}

// ------------- Player -----------------------

cCdPlayer::cCdPlayer(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    pStillBuf = NULL;
    mStillBufLen = 0;
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
    printf("%.16s (%d) On=%s\n", __FILE__, __LINE__, On ? "true" : "false");
    if (On) {
        printf("\n\nStarting Player\n");
        LoadStillPicture ((const char *)"/video/conf/plugins/radio/radio.mpg");
        Start ();
        DevicePlay();
    }
    else {
        Cancel(3);
    }
    printf("Activate %d\n", On);
}

void cCdPlayer::Action(void)
{
    cPesAudioConverter converter;
    cPoller oPoller;
    cBufferedCdio cdio;
    uint8_t buf[CDIO_CD_FRAMESIZE_RAW];
    const uchar *pesdata;
    int peslen;

    cdio.OpenDevice ("/dev/sr0");
    cdio.Start();
    while (true) {
        if (DevicePoll(oPoller, 1000)) {
            if (!cdio.GetData(buf)) {
                return;
            }
            converter.SetData (buf, CDIO_CD_FRAMESIZE_RAW);
            pesdata = converter.GetPesData ();
            peslen = converter.GetPesLength();
            if (PlayPes(pesdata, peslen, false) < 0) {
               return;
            }
        }
    }
}
