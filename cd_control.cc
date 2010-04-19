#include "cdplayer.h"
#include "pes_audio_converter.h"
#include "bufferedcdio.h"

cCdControl::cCdControl(void)
    :cControl(mCdPlayer = new cCdPlayer), mMenuPlaylist(NULL)
{

}

cCdControl::~cCdControl()
{
    if (mMenuPlaylist != NULL) {
        delete mMenuPlaylist;
    }
}

void cCdControl::Hide(void)
{
    printf("Hide\n");
    mControlMutex.Lock();
    if (mMenuPlaylist != NULL) {
        delete mMenuPlaylist;
    }
    mMenuPlaylist = NULL;
    mControlMutex.Unlock();
    printf("Hide End\n");
}

cOsdObject *cCdControl::GetInfo(void)
{
    printf("GetInfo\n");
    return NULL;
}

eOSState cCdControl::ProcessKey(eKeys Key)
{
    eOSState state = cOsdObject::ProcessKey(Key);
    printf("ProcessKey %d\n",Key);
    mControlMutex.Lock();

    if (mMenuPlaylist == NULL) {
        mMenuPlaylist = Skins.Current()->DisplayMenu();
    }
    if (state == osUnknown) {
        switch (Key & ~k_Repeat) {
        case kFastFwd:
            mCdPlayer->SpeedFaster();
            state = osContinue;
            break;
        case kFastRew:
            mCdPlayer->SpeedSlower();
            state = osContinue;
            break;
        case kPlay:
            mCdPlayer->SpeedNormal();
            state = osContinue;
            break;
        case kDown:
        case kNext:
            mCdPlayer->NextTrack();
            state = osContinue;
            break;
        case kUp:
        case kPrev:
            mCdPlayer->PrevTrack();
            state = osContinue;
            break;
        case kOk:
        case kMenu:
        case kStop:
            mCdPlayer->Stop();
            state = osEnd;
        default:
            break;
        }
    }
    mControlMutex.Unlock();
    ShowPlaylist();
    return (state);
}

void cCdControl::ShowPlaylist()
{
    mControlMutex.Lock();
    if (mMenuPlaylist != NULL) {
        mMenuPlaylist->Clear();
        mMenuPlaylist->SetTitle(tr("Playlist"));
        mMenuPlaylist->SetTabs(4, 15);
        for (TRACK_IDX_T i = 0; i < mCdPlayer->GetNumTracks(); i++) {
            const CD_TEXT_T text = mCdPlayer->GetCdTextFields(i);
            string artist = text[CDTEXT_PERFORMER];
            string title = text[CDTEXT_TITLE];
            char *str;
            asprintf(&str, "%2d\t %s\t %s", i + 1, artist.c_str(),
                    title.c_str());
            mMenuPlaylist->SetItem(str, i, (i == mCdPlayer->GetCurrTrack()),
                    true);
            free(str);
        }
    }
    mControlMutex.Unlock();
}


// ------------- Player -----------------------

const PCM_FREQ_T cCdPlayer::mSpeedTypes[] = {PCM_FREQ_44100, PCM_FREQ_48000, PCM_FREQ_96000};

cCdPlayer::cCdPlayer(void)
{
    pStillBuf = NULL;
    mStillBufLen = 0;
    mSpeed = 0;
}

cCdPlayer::~cCdPlayer()
{
    mPlayerMutex.Lock();
    Detach();
    free (pStillBuf);
    mPlayerMutex.Unlock();
}

void cCdPlayer::DisplayStillPicture (void)
{
    if (pStillBuf != NULL) {
        DeviceStillPicture((const uchar *)pStillBuf, mStillBufLen);
    }
}
void cCdPlayer::LoadStillPicture (const std::string FileName)
{
    int fd;
    int len;
    int size = MAXFRAMESIZE;
    mPlayerMutex.Lock();
    if (pStillBuf != NULL) {
        free (pStillBuf);
    }
    pStillBuf = NULL;
    mStillBufLen = 0;

    fd = open(FileName.c_str(), O_RDONLY);
    if (fd < 0) {
        esyslog("%s %d Can not open still picture %s",
                __FILE__, __LINE__, FileName.c_str());
        mPlayerMutex.Unlock();
        return;
    }

    pStillBuf = (uchar *)malloc (MAXFRAMESIZE);
    if (pStillBuf == NULL) {
        esyslog("%s %d Out of memory", __FILE__, __LINE__);
        close(fd);
        mPlayerMutex.Unlock();
        return;
    }
    do {
        len = read(fd, &pStillBuf[mStillBufLen], MAXFRAMESIZE);
        if (len < 0) {
            esyslog ("%s %d read error %d", __FILE__, __LINE__, errno);
            close(fd);
            mPlayerMutex.Unlock();
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
                    mPlayerMutex.Unlock();
                    return;
                }
            }
        }
    } while (len > 0);
    close(fd);
    mPlayerMutex.Unlock();
    DisplayStillPicture();
}

void cCdPlayer::Activate(bool On)
{
    mPlayerMutex.Lock();
    printf("%.16s (%d) Activate On=%s\n", __FILE__, __LINE__, On ? "true" : "false");
    if (On) {

        std::string file = cPluginCdplayer::GetStillPicName();
        printf("\n\nStarting Player %s\n", file.c_str());
        LoadStillPicture (file);
        Start ();
        DevicePlay();
    }
    else {
        Cancel(0);
    }
    mPlayerMutex.Unlock();
}

#define FRAME_DIV 4

void cCdPlayer::Action(void)
{
    bool play = true;
    cPesAudioConverter converter;
    cPoller oPoller;

    uint8_t buf[CDIO_CD_FRAMESIZE_RAW];
    const uchar *pesdata;
    int peslen;
    int chunksize = CDIO_CD_FRAMESIZE_RAW / FRAME_DIV;

    cdio.OpenDevice ("/dev/sr0");
    cdio.Start();
    while (play) {
        if (!cdio.GetData(buf)) {
            play = false;
        }
        int i = 0;
        while (i < FRAME_DIV) {
            if (DevicePoll(oPoller, 100)) {
                converter.SetFreq(mSpeedTypes[mSpeed]);
                converter.SetData(&buf[i * chunksize], chunksize);
                pesdata = converter.GetPesData();
                peslen = converter.GetPesLength();
                if (PlayPes(pesdata, peslen, false) < 0) {
                    play = false;
                    break;
                }
                i++;
            }
            if (!Running()) {
                play = false;
                break;
            }
        }
    }
    cdio.Stop();
}
