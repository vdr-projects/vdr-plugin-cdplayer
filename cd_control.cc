/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the control and player
 *
 */

#include "cdplayer.h"
#include "pes_audio_converter.h"
#include "bufferedcdio.h"
#include <time.h>

const char *cCdControl::menutitle = tr("CD Player");
const char *cCdControl::menukind = "CDPLAYER";

cCdControl::cCdControl(void)
    :cControl(mCdPlayer = new cCdPlayer), mMenuPlaylist(NULL)
{

}

cCdControl::~cCdControl()
{
    Hide();
    delete mCdPlayer;
}

void cCdControl::Hide(void)
{
    cMutexLock MutexLock(&mControlMutex);
    if (mMenuPlaylist != NULL) {
        mMenuPlaylist->Clear();
        delete mMenuPlaylist;
        cStatus::MsgOsdClear();
        cStatus::MsgOsdMenuDestroy();
    }
    mMenuPlaylist = NULL;
}

cOsdObject *cCdControl::GetInfo(void)
{
    return NULL;
}

eOSState cCdControl::ProcessKey(eKeys Key)
{
    eOSState state = cOsdObject::ProcessKey(Key);
    if (state != osUnknown) {
        return (state);
    }
    state = osContinue;
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
        Skins.QueueMessage(mtError, mCdPlayer->GetErrorText().c_str());
        state = osEnd;
    }
    if (state != osEnd) {
        ShowPlaylist();
    }
    else {
        Hide();
        mCdPlayer->Stop();
    }
    return (state);
}

void cCdControl::ShowPlaylist()
{
    cMutexLock MutexLock(&mControlMutex);

    // If any other OSD is open then dont show Playlist menu
    if (cOsd::IsOpen() && mMenuPlaylist == NULL) {
        return;
    }

    // Display Playlist menu
    if (mMenuPlaylist == NULL) {
        mMenuPlaylist = Skins.Current()->DisplayMenu();
        cStatus::MsgOsdMenuDisplay(menukind);
    }

    // Build title information
    CD_TEXT_T cd_info;
    mCdPlayer->GetCdInfo(cd_info);
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
    case BCDIO_FAILED:
        title += "--";
        break;
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

    // Build Playlist
    mMenuPlaylist->SetTabs(3, 6, 10);
    cStatus::MsgOsdClear();
    cStatus::MsgOsdTitle(title.c_str());
    string curr;
    for (TRACK_IDX_T i = 0; i < mCdPlayer->GetNumTracks(); i++) {
        CD_TEXT_T text;
        mCdPlayer->GetCdTextFields(i, text);
        string artist = text[CDTEXT_PERFORMER];
        string title = text[CDTEXT_TITLE];
        char *str;
        int min, sec;
        mCdPlayer->GetTrackTime(i, &min, &sec);
        asprintf(&str, "%2d\t%2d:%02d\t%s\t %s",
                  i + 1, min, sec, artist.c_str(), title.c_str());
        mMenuPlaylist->SetItem(str, i, (i == mCdPlayer->GetCurrTrack()), true);
        free(str);
        asprintf(&str, "%2d %2d:%02d %s", i + 1, min, sec, title.c_str());
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
    SetDescription ("cdplayer");
}

cCdPlayer::~cCdPlayer()
{
    cMutexLock MutexLock(&mPlayerMutex);
    Stop();
    free(pStillBuf);
}

void cCdPlayer::Stop(void) {
    DeviceClear();
    if (Active()) {
        Cancel(10);
    }
    Detach();
}

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
        string errtxt = tr("Can not open still picture: ");
        errtxt += FileName;
        Skins.QueueMessage(mtError, errtxt.c_str());
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
            free (pStillBuf);
            pStillBuf = NULL;
            mStillBufLen = 0;
            return;
        }
        if (len > 0) {
            mStillBufLen += len;
            if (mStillBufLen >= size) {
                size += MAXFRAMESIZE;
                pStillBuf = (uchar *) realloc(pStillBuf, size);
                if (pStillBuf == NULL) {
                    close(fd);
                    mStillBufLen = 0;
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
        if (GetState() != BCDIO_PLAY) {
            std::string file = cPluginCdplayer::GetStillPicName();
            LoadStillPicture(file);
            Start();
            DevicePlay();
        }
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
