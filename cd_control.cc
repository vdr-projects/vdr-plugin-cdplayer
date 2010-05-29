#include "cdplayer.h"
#include "pes_audio_converter.h"
#include "bufferedcdio.h"
#include <time.h>

const char *cCdControl::menutitle = "CD Player";

cCdControl::cCdControl(void)
    :cControl(mCdPlayer = new cCdPlayer), mMenuPlaylist(NULL)
{

}

cCdControl::~cCdControl()
{
    cMutexLock MutexLock(&mControlMutex);
    Hide();
    cStatus::MsgOsdClear();
    delete mCdPlayer;
}

void cCdControl::Hide(void)
{
    cMutexLock MutexLock(&mControlMutex);
    if (mMenuPlaylist != NULL) {
        mMenuPlaylist->Clear();
        delete mMenuPlaylist;
    }
    mMenuPlaylist = NULL;
}

cOsdObject *cCdControl::GetInfo(void)
{
printf("GetInfo\n");
    return NULL;
}

eOSState cCdControl::ProcessKey(eKeys Key)
{
    eOSState state = osContinue; //cOsdObject::ProcessKey(Key);

    switch (Key & ~k_Repeat) {
    case kFastFwd:
        mCdPlayer->SpeedFaster();
        break;
    case kFastRew:
        mCdPlayer->SpeedSlower();
        break;
    case kPlay:
        mCdPlayer->SpeedNormal();
        break;
    case kDown:
    case kNext:
        mCdPlayer->NextTrack();
        break;
    case kUp:
    case kPrev:
        mCdPlayer->PrevTrack();
        break;
    case kOk:
    case kMenu:
    case kStop:
        state = osEnd;
        break;
    case kPause:
        mCdPlayer->Pause();
        break;
    default:
        state = osUnknown;
        break;
    }

    if (mCdPlayer->GetState() == BCDIO_FAILED) {
        cStatus::MsgOsdStatusMessage(mCdPlayer->GetErrorText().c_str());
        cSkinDisplay::Current()->SetMessage(mtError, mCdPlayer->GetErrorText().c_str());
        sleep(3);
        state = osEnd;
    }
    if (state != osEnd) {
        ShowPlaylist();
    }
    else {
        if (mMenuPlaylist != NULL) {
            mMenuPlaylist->SetTitle("----");
            cStatus::MsgOsdTitle("----");
        }
        Hide();
        mCdPlayer->Stop();
        cStatus::MsgOsdMenuDestroy();
    }
    return (state);
}

void cCdControl::ShowPlaylist()
{
    cMutexLock MutexLock(&mControlMutex);

    if (mMenuPlaylist == NULL) {
        mMenuPlaylist = Skins.Current()->DisplayMenu();
    }

    const CD_TEXT_T cd_info = mCdPlayer->GetCDInfo();
    string title;
    string cdtitle = cd_info[CDTEXT_TITLE];
    string perform = cd_info[CDTEXT_PERFORMER];
    if (!cdtitle.empty()) {
        title = cdtitle;
    }
    else if (!perform.empty()) {
        title = perform;
    } else {
        title = tr("Playlist");

    }

    title += "  ";
    switch (mCdPlayer->GetState()) {
    case BCDIO_STOP:
    case BCDIO_PAUSE:
        title += "||";
        break;
    case BCDIO_PLAY:
        title += ">>";
        break;
    default:
        break;
    }

    switch (mCdPlayer->GetSpeed()) {
    case 1:
        title += "  x1,1";
        break;
    case 2:
        title += "  x2";
        break;
    default:
        break;
    }
    mMenuPlaylist->SetTitle(title.c_str());
    mMenuPlaylist->SetTabs(4, 15);
    cStatus::MsgOsdMenuDisplay(menutitle);
    cStatus::MsgOsdClear();
    cStatus::MsgOsdTitle(title.c_str());
    string curr;
    for (TRACK_IDX_T i = 0; i < mCdPlayer->GetNumTracks(); i++) {
        const CD_TEXT_T text = mCdPlayer->GetCdTextFields(i);
        string artist = text[CDTEXT_PERFORMER];
        string title = text[CDTEXT_TITLE];
        char *str;

        asprintf(&str, "%2d\t %s\t %s", i + 1, artist.c_str(), title.c_str());
        mMenuPlaylist->SetItem(str, i, (i == mCdPlayer->GetCurrTrack()), true);
        free(str);
        asprintf(&str, "%2d %s", i + 1, title.c_str());
        cStatus::MsgOsdItem(str, i + 1);
        if (i == mCdPlayer->GetCurrTrack()) {
            curr = str;
        }
        free(str);
    }
    cStatus::MsgOsdCurrentItem (curr.c_str());
 //   mMenuPlaylist->SetButtons("Red", "Green", "Yellow", "Blue");

}


// ------------- Player -----------------------

const PCM_FREQ_T cCdPlayer::mSpeedTypes[] = {
        PCM_FREQ_44100,
        PCM_FREQ_48000,
        PCM_FREQ_96000
};

cCdPlayer::cCdPlayer(void)
{
    pStillBuf = NULL;
    mStillBufLen = 0;
    mSpeed = 0;
}

cCdPlayer::~cCdPlayer()
{
    cMutexLock MutexLock(&mPlayerMutex);
    Detach();
    free(pStillBuf);
}

void cCdPlayer::Stop(void) {
    Cancel(10);
    DeviceClear();
    Detach();
};

bool cCdPlayer::GetReplayMode(bool &Play, bool &Forward, int &Speed)
{
    cMutexLock MutexLock(&mPlayerMutex);
    Play = (GetState() == BCDIO_PLAY);
    Forward = true;
    Speed = -1;
    if (GetSpeed() != 0) {
        Speed = GetSpeed();
    }
    return (true);
}

bool cCdPlayer::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
    cMutexLock MutexLock(&mPlayerMutex);
    Current = GetCurrTrack();
    Total = GetNumTracks();
    return (true);
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
    cMutexLock MutexLock(&mPlayerMutex);

    if (pStillBuf != NULL) {
        free (pStillBuf);
    }
    pStillBuf = NULL;
    mStillBufLen = 0;

    fd = open(FileName.c_str(), O_RDONLY);
    if (fd < 0) {
        esyslog("%s %d Can not open still picture %s",
                __FILE__, __LINE__, FileName.c_str());
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
    cMutexLock MutexLock(&mPlayerMutex);
    if (On) {
        std::string file = cPluginCdplayer::GetStillPicName();
        LoadStillPicture(file);
        Start();
        DevicePlay();
    }
    else {
        Stop();
    }
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

    if (!cdio.OpenDevice (cPluginCdplayer::GetDeviceName())) {
        return;
    }
    cdio.Start();
    cDevice::PrimaryDevice()->SetCurrentAudioTrack(ttAudio);
    while (play) {
        if (!cdio.GetData(buf)) {
            play = false;
        }
        int i = 0;
        while ((i < FRAME_DIV) && (play)) {
            if (DevicePoll(oPoller, 100)) {
                cMutexLock MutexLock(&mPlayerMutex);
                converter.SetFreq(mSpeedTypes[mSpeed]);
                converter.SetData(&buf[i * chunksize], chunksize);
                pesdata = converter.GetPesData();
                peslen = converter.GetPesLength();

                if (PlayPes(pesdata, peslen, false) < 0) {
                    play = false;
                }
                i++;
            }
            if (!Running()) {
                play = false;
            }
        }
    }
    cdio.Stop();
}
