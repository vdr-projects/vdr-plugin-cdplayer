/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010-2012 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
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
#include "cdmenu.h"
#include <time.h>
#include <assert.h>
#include <langinfo.h>

const char *cCdControl::menukindPlayList = "MenuCDPlayList";
const char *cCdControl::menukindDetail = "NormalDetail";
const char *cCdControl::specialMenu[CD_DISPLAY_LAST][CH_CHAR_LAST] =
{
    {ASCII_CHAR_PAUSE, ASCII_CHAR_PLAY, ASCII_CHAR_RESTART,
     ASCII_CHAR_NORMAL,ASCII_CHAR_RANDOM, ASCII_CHAR_SORTED},
    {UTF8_CHAR_PAUSE, UTF8_CHAR_PLAY, UTF8_CHAR_RESTART,
     UTF8_CHAR_NORMAL,UTF8_CHAR_RANDOM, UTF8_CHAR_SORTED}
};

cCdControl::cCdControl(void)
    : cControl(mCdPlayer = new cCdPlayer), mMenuPlaylist(NULL),
      mShowDetail(false), mCurrtitle(INVALID_TRACK_IDX)
{
    char *codeset = nl_langinfo(CODESET);

    cStatus::MsgReplaying(this,tr("CD Player"), NULL,true);
    mPlayRandom = cMenuCDPlayer::GetPlayMode();
    if (mPlayRandom) {
        mCdPlayer->RandomPlay();
    }
    else {
        mCdPlayer->SortedPlay();
    }
    mRestart = cMenuCDPlayer::GetRestart();
    mCdPlayer->SetRestartMode(mRestart);
    mIsUTF8 = false;
    dsyslog("Codeset %s", codeset);
    if (strcmp(codeset, "UTF-8") == 0) {
        mIsUTF8 = true;
    }
}

cCdControl::~cCdControl()
{
    dsyslog("Destroy cCdControl");
    cStatus::MsgReplaying(this,NULL,NULL,false);
    if (mMenuPlaylist != NULL) {
        delete mMenuPlaylist;
    }
    if (mCdPlayer != NULL) {
        delete mCdPlayer;
    }
}

void cCdControl::Hide(void)
{
    dsyslog("Hide cCdControl");
    cMutexLock MutexLock(&mControlMutex);
    if (mMenuPlaylist != NULL) {
        mMenuPlaylist->Clear();
        delete mMenuPlaylist;
        cStatus::MsgOsdClear();
#ifdef USE_GRAPHTFT
        cStatus::MsgOsdMenuDestroy();
#endif
    }
    mMenuPlaylist = NULL;
}


eOSState cCdControl::ProcessKey(eKeys Key)
{
    eOSState state = osContinue;

    switch (Key) {
    case kOk:
        Key = cMenuCDPlayer::GetOkKey();
        break;
    case kBack:
        Key = cMenuCDPlayer::GetBackKey();
        break;
    default:
        break;
    }

    switch (Key) {
    case kRed:
        if (mPlayRandom) {
            mCdPlayer->SortedPlay();
            mPlayRandom = false;
        }
        else {
            mCdPlayer->RandomPlay();
            mPlayRandom = true;
        }
        mCurrtitle = INVALID_TRACK_IDX;
        break;
    case kGreen: // 1min back
        mCdPlayer->ChangeTime(-60);
        break;
    case kYellow: //1min forward
        mCdPlayer->ChangeTime(60);
        break;
    case kBlue: // Switch info display
        mShowDetail = !mShowDetail;
        break;
    case kFastFwd:
        mCdPlayer->SpeedFaster();
        break;
    case kFastRew:
        mCdPlayer->SpeedSlower();
        break;
    case kPlay:
        mCdPlayer->Play();
        break;
    case kDown:
    case kNext:
        mCdPlayer->NextTrack();
        break;
    case kUp:
    case kPrev:
        mCdPlayer->PrevTrack();
        break;
    case kStop:
        state = osEnd;
        break;
    case kPause:
        mCdPlayer->Pause();
        break;
    case kLeft:
        mRestart = !mRestart;
        mCdPlayer->SetRestartMode(mRestart);
        break;
    default:
        if ((Key >= k1) && (Key <= k9))
        {
            mCdPlayer->SetTrack(Key - k1);
        }
        else
        {
            state = cOsdObject::ProcessKey(Key);
            if (state != osUnknown) {
                return (state);
            }
            state = osUnknown;
        }
        break;
    }

    if (mCdPlayer->GetState() == BCDIO_FAILED) {
        cStatus::MsgOsdStatusMessage(mCdPlayer->GetErrorText().c_str());
        Skins.QueueMessage(mtError, mCdPlayer->GetErrorText().c_str());
        state = osEnd;
    }
    if (mCdPlayer->GetState() == BCDIO_STOP) {
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

void cCdControl::DisplayLine(const char *buf, int line)
{
    mMenuPlaylist->SetItem(buf, line, false, true);
    cStatus::MsgOsdItem(buf, line);
}

void cCdControl::ShowDetail()
{
    CD_TEXT_T cd_info;
    char buf[100];
    int i;
    int lncnt = 1;
    string str;
    TRACK_IDX_T currtitle = mCdPlayer->GetCurrTrack();

    mMenuPlaylist->SetTabs(18);
    str = tr("CD Information");
    str = "\t" + str;
    DisplayLine(str.c_str(), lncnt++);
    mCdPlayer->GetCdInfo(cd_info);

    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        if (!cd_info[i].empty()) {
            sprintf(buf, "%.15s\t: %.50s",
                    cBufferedCdio::GetCdTextField((cdtext_field_t)i),
                    cd_info[i].c_str());
            DisplayLine(buf, lncnt++);
        }
    }
    DisplayLine("", lncnt++);
    str = tr("Song Information");
    str = "\t" + str;
    DisplayLine(str.c_str(), lncnt++);

    mCdPlayer->GetCdTextFields(currtitle, cd_info);
    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        if (!cd_info[i].empty()) {
            sprintf(buf, "%.15s\t: %.50s",
                     cBufferedCdio::GetCdTextField((cdtext_field_t) i),
                     cd_info[i].c_str());
            DisplayLine(buf, lncnt++);
        }
    }
    SetHelpkeys();
}

void cCdControl::SetHelpkeys(void)
{
    static const char *redtxtrand = tr("random");
    static const char *redtxtsort = tr("sort");
    static const char *greentxt = tr("1 min -");
    static const char *yellowtxt = tr("1 min +");
    static const char *bluetxtplay = tr("playlist");
    static const char *bluetxtdet = tr("detail");

    const char *btext = bluetxtdet;
    const char *rtext = redtxtrand;
    if (mShowDetail) {
        btext = bluetxtplay;
    }
    if (mPlayRandom) {
        rtext = redtxtsort;
    }
    cStatus::MsgOsdHelpKeys(rtext, greentxt, yellowtxt, btext);
    mMenuPlaylist->SetButtons(rtext, greentxt, yellowtxt, btext);
}

void cCdControl::ShowList()
{
    TRACK_IDX_T numtrk = mCdPlayer->GetNumTracks();
    TRACK_IDX_T currtitle = mCdPlayer->GetCurrTrack();
    int offset = 0;
    int maxitems = mMenuPlaylist->MaxItems();
    char *str;

    // Build Playlist
    mMenuPlaylist->SetTabs(3, 6, 10);
    if (numtrk > mMenuPlaylist->MaxItems()) {
        int itemcnt = maxitems / 2;
        if ((int) currtitle > itemcnt) {
            offset = currtitle - itemcnt;
            if (offset + maxitems > (int) mCdPlayer->GetNumTracks()) {
                offset = mCdPlayer->GetNumTracks() - maxitems;
            }
        }
        mMenuPlaylist->SetScrollbar(mCdPlayer->GetNumTracks(), offset);
    }
    if (maxitems > numtrk) {
        maxitems = numtrk;
    }
    for (int i = 0; i < maxitems; i++) {
        TRACK_IDX_T trk = i + offset;
        str = BuildMenuStr(trk);
        mMenuPlaylist->SetItem(str, i, (trk == mCdPlayer->GetCurrTrack()), true);
        free(str);
    }

    for (TRACK_IDX_T i = 0; i < numtrk; i++) {
        str = BuildOSDStr(i);
        cStatus::MsgOsdItem(str, i + 1);
        free(str);
    }
    if ((currtitle != INVALID_TRACK_IDX) && (numtrk > 0)) {
        str = BuildOSDStr(currtitle);
        cStatus::MsgOsdCurrentItem(str);
        free(str);
    }
    SetHelpkeys();
}
void cCdControl::Replace (string &s, const char *repl, const char *with)
{
    string::size_type idx;
    int len = strlen (repl);

    while ((idx = s.find(repl)) != string::npos) {
        s.replace (idx, len, with);
    }
}

void cCdControl::ShowPlaylist()
{
    cMutexLock MutexLock(&mControlMutex);
    CD_TEXT_T cd_info;
    bool render_all = false;
    static BUFCDIO_STATE_T state = BCDIO_FAILED;
    static int speed = -1;
    static int numtrk = -1;
    static bool cddbinfo = false;
    static bool detail = false;
    static bool restart = false;
    static bool playrandom = false;

    if ((mCurrtitle != mCdPlayer->GetCurrTrack()) ||
        (numtrk != mCdPlayer->GetNumTracks()) ||
        (state != mCdPlayer->GetState()) ||
        (cddbinfo != mCdPlayer->CDDBInfoAvailable()) ||
        (detail != mShowDetail) ||
        (restart != mRestart) ||
        (playrandom != mPlayRandom) ||
        (speed != mCdPlayer->GetSpeed())) {
        render_all = true;
    }
    // If no change in display and any other OSD is open then don't show Playlist menu
    if ((!render_all) && cOsd::IsOpen() && (mMenuPlaylist == NULL)) {
        return;
    }

    if (mMenuPlaylist != cSkinDisplay::Current()) {
        if (mMenuPlaylist != NULL) {
            delete mMenuPlaylist;
        }
        mMenuPlaylist = NULL;
    }

    // Display Playlist menu
    if (mMenuPlaylist == NULL) {
        dsyslog("Show OSD");
        render_all = true;
        mMenuPlaylist = Skins.Current()->DisplayMenu();
#ifdef USE_GRAPHTFT
        cStatus::MsgOsdMenuDestroy();
        cStatus::MsgOsdMenuDisplay(menukindPlayList);
#endif
    }

    if (render_all) {
        if (detail != mShowDetail) {
            mMenuPlaylist->Clear();
        }
        playrandom = mPlayRandom;
        restart = mRestart;
        detail = mShowDetail;
        mCurrtitle = mCdPlayer->GetCurrTrack();
        state = mCdPlayer->GetState();
        speed = mCdPlayer->GetSpeed();
        numtrk = mCdPlayer->GetNumTracks();
        cddbinfo = mCdPlayer->CDDBInfoAvailable();

        mCdPlayer->GetCdInfo(cd_info);
        string title;
        string cdtitle = cd_info[CDTEXT_TITLE];
        string perform = cd_info[CDTEXT_PERFORMER];

        // Build title information
        if (!cdtitle.empty()) {
            title = cdtitle;
        } else if (!perform.empty()) {
            title = perform;
        } else {
            title = tr("Playlist");
        }

        title += "  ";
        switch (mCdPlayer->GetState()) {
        case BCDIO_FAILED:
        case BCDIO_STARTING:
            title += "-";
            break;
        case BCDIO_STOP:
        case BCDIO_PAUSE:
            title += GetString(CD_CHAR_PAUSE);
            break;
        case BCDIO_PLAY:
            title += GetString(CD_CHAR_PLAY);
            break;
        default:
            break;
        }

        title += " ";
        if (mRestart) {
            title += GetString(CD_CHAR_RESTART);
        }
        else {
            title += GetString(CD_CHAR_NORMAL);
        }

        switch (mCdPlayer->GetSpeed()) {
        case 1:
            title += " x1,1";
            break;
        case 2:
            title += " x2";
            break;
        default:
            break;
        }

        title += " ";
        if (mPlayRandom) {
            title += GetString(CD_CHAR_RANDOM);
        }
        else {
            title += GetString(CD_CHAR_SORTED);
        }
        Skins.Message(mtStatus, NULL);
        mMenuPlaylist->SetTitle(title.c_str());
        cStatus::MsgOsdClear();
        if (cMenuCDPlayer::GetGraphTFT() && mIsUTF8) {
            Replace(title, UTF8_CHAR_PLAY, GRAPHTFT_CHAR_DISK);
            Replace(title, UTF8_CHAR_PAUSE, GRAPHTFT_CHAR_PAUSE);
            Replace(title, UTF8_CHAR_RESTART, GRAPHTFT_CHAR_RESTART);
            Replace(title, UTF8_CHAR_NORMAL, GRAPHTFT_CHAR_NORMAL);
            Replace(title, UTF8_CHAR_RANDOM, GRAPHTFT_CHAR_RANDOM);
            Replace(title, UTF8_CHAR_SORTED, GRAPHTFT_CHAR_SORTED);
        }
        cStatus::MsgOsdTitle(title.c_str());

        if (mShowDetail) {
            ShowDetail();
        } else {
            ShowList();
        }
        mMenuPlaylist->Flush();
    }
}


char *cCdControl::BuildOSDStr(TRACK_IDX_T idx)
{
    CD_TEXT_T text;
    char *str;
    int min, sec;
    mCdPlayer->GetCdTextFields(idx, text);
    string title = text[CDTEXT_TITLE];
    mCdPlayer->GetTrackTime(idx, &min, &sec);
    asprintf(&str, "%2d %2d:%02d %s", idx+1, min, sec, title.c_str());
    return str;
}

char *cCdControl::BuildMenuStr(TRACK_IDX_T idx)
{
    CD_TEXT_T text;
    char *str;
    int min, sec;

    mCdPlayer->GetCdTextFields(idx, text);
    string artist = text[CDTEXT_PERFORMER];
    string title = text[CDTEXT_TITLE];

    mCdPlayer->GetTrackTime(idx, &min, &sec);
    if (cMenuCDPlayer::GetShowArtist()) {
        asprintf(&str, "%2d\t%2d:%02d\t%s\t %s", idx+1, min, sec, artist.c_str(),
                title.c_str());
    }
    else {
        asprintf(&str, "%2d\t%2d:%02d\t%s", idx+1, min, sec, title.c_str());
    }
    return str;
}

// ------------- Player -----------------------

const PCM_FREQ_T cCdPlayer::mSpeedTypes[] = {
        PCM_FREQ_44100,
        PCM_FREQ_48000,
        PCM_FREQ_96000
};

cCdPlayer::cCdPlayer(void)
    :cPlayer (pmAudioVideo)
{
    pStillBuf = NULL;
    mStillBufLen = 0;
    mSpeed = 0;
    mPurge = false;
    mPlayRandom = false;
    mSpanPlugin = cPluginManager::CallFirstService(SPAN_SET_PCM_DATA_ID, NULL);
    SetDescription ("cdplayer");
}

cCdPlayer::~cCdPlayer()
{
    dsyslog("Destroy cCdPlayer");
    Stop();
    free(pStillBuf);
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

bool cCdPlayer::GetIndex(int &Current, int &Total, UNUSED_ARG bool SnapToIFrame)
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
    int size = CDMAXFRAMESIZE;
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

    pStillBuf = (uchar *)malloc (CDMAXFRAMESIZE);
    if (pStillBuf == NULL) {
        esyslog("%s %d Out of memory", __FILE__, __LINE__);
        close(fd);
        return;
    }
    do {
        len = read(fd, &pStillBuf[mStillBufLen], CDMAXFRAMESIZE);
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
                size += CDMAXFRAMESIZE;
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
        dsyslog("cCdPlayer Activate On");
        if (GetState() != BCDIO_PLAY) {
            DeviceClear();
            std::string file = cPluginCdplayer::GetStillPicName();
            LoadStillPicture(file);
            Play();
        }
    }
    else {
        dsyslog("cCdPlayer Activate Off");
        Stop();
    }
}

void cCdPlayer::Pause (void)
{
    dsyslog("cCdPlayer Pause");
    mBufCdio.Pause();
    if (mBufCdio.GetState() == BCDIO_PLAY) {
        DevicePlay();
    }
    else {
        DeviceFreeze();
    }
}

void cCdPlayer::Play ()
{
    dsyslog("cCdPlayer Start");
    // If CDIO Input thread is not running, start it
    if (!mBufCdio.Active()) {
        if (!mBufCdio.OpenDevice(cPluginCdplayer::GetDeviceName())) {
            return;
        }
        mBufCdio.Start();
    }
    if (mPlayRandom) {
        RandomPlay();
    }
    mBufCdio.Play();
    SpeedNormal();
    // If CD-Player output thread is not running, start it
    if (!Active()) {
        Start();
    }
}

void cCdPlayer::Stop(void)
{
    dsyslog("cCdPlayer Stop");

    DeviceClear();
    if (Active()) {
        Cancel(10);
    }
    Detach();
}

void cCdPlayer::SetTrack(TRACK_IDX_T track)
{
    dsyslog("cCdPlayer SetTrack");
    mBufCdio.SetTrack(track);
    DeviceClear();
}

void cCdPlayer::NextTrack(void)
{
    dsyslog("cCdPlayer Next");
    mBufCdio.NextTrack();
    DeviceClear();
}

void cCdPlayer::PrevTrack(void)
{
    dsyslog("cCdPlayer Prev");
    mBufCdio.PrevTrack();
    DeviceClear();
}

void cCdPlayer::ChangeTime(int tm)
{
    dsyslog("cCdPlayer ChangeTime");
    mBufCdio.SkipTime(tm);
    DeviceClear();
}

bool cCdPlayer::PlayData (const uint8_t *buf, int frame) {
    const uchar *pesdata;
    static uint8_t pesbuf[CDIO_CD_FRAMESIZE_RAW];
    int peslen;
    int idx = 0;
    cPesAudioConverter converter;
    cPoller oPoller;

#if 0
FILE *fp=fopen("/tmp/out.raw","a");
fwrite(buf,CDIO_CD_FRAMESIZE_RAW,1,fp);
fclose(fp);
#endif

    if (mSpanPlugin != NULL) {
        Span_SetPlayindex_1_0 SetPlayindexData;
        SetPlayindexData.index = (frame * 1000) / CDIO_CD_FRAMES_PER_SEC;
        cPluginManager::CallFirstService(SPAN_SET_PLAYINDEX_ID, &SetPlayindexData);
    }
    while (idx < CDIO_CD_FRAMESIZE_RAW) {
        if (DevicePoll(oPoller, 100)) {

            converter.SetFreq(mSpeedTypes[mSpeed]);
            converter.SetData(&buf[idx], CDIO_CD_FRAMESIZE_RAW/2);
            pesdata = converter.GetPesData();
            peslen = converter.GetPesLength();
#if 0
FILE *fp=fopen("/tmp/out.pes","a");
fwrite(pesdata,peslen,1,fp);
fclose(fp);
#endif
            memcpy (pesbuf, pesdata, peslen);
            if (mPurge) {
                return true;
            }

            if (PlayPes(pesbuf, peslen, false) < 0) {
                esyslog("%s %d PlayPes failed", __FILE__, __LINE__);
                return false;
            }
            idx += CDIO_CD_FRAMESIZE_RAW/2;
        }
        if (!Running()) {
            return false;
        }
        if (mPurge) {
            return true;
        }
    }
    return true;
}

void cCdPlayer::Action(void)
{
    bool play = true;
    uint8_t buf[CDIO_CD_FRAMESIZE_RAW];
    lsn_t lsn = 0;
    int frame = 0;
    // Clear and flush output device
    DeviceClear();
    DeviceFlush(100);
    DeviceSetCurrentAudioTrack(ttAudio);
    DevicePlay();

    // Wait until some Data is in the ring buffer
    mBufCdio.WaitBuffer();
    mPurge = false;
    while (play) {
        if (!mBufCdio.GetData(buf, &lsn, &frame)) {
            dsyslog ("cCdPlayer GetData stop");
            play = false;
        }
        if (play) {
            if (mPurge) {
                DevicePlay();
                DeviceSetCurrentAudioTrack(ttAudio);
                mPurge = false;
            } else {
                play = PlayData(buf, frame);
            }
        }
        if (!Running()) {
            play = false;
        }
    }
    mBufCdio.Stop();
}
