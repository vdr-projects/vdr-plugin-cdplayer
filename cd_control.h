/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010-2012 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This module implements the cControl and cPlayer for the cd player.
 */

#ifndef _CD_CONTROL_H
#define _CD_CONTROL_H

#include <vdr/player.h>
#include "bufferedcdio.h"
#include "pes_audio_converter.h"
#include "service.h"

// The maximum size of a single frame (up to HDTV 1920x1080):
#define TS_SIZE 188
#define CDMAXFRAMESIZE  (KILOBYTE(1024) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE to avoid breaking up TS packets
#define MAX_SPEED 2

#define ASCII_CHAR_PAUSE   "||"
#define ASCII_CHAR_PLAY    ">"
#define ASCII_CHAR_RESTART "R"
#define ASCII_CHAR_NORMAL  "N"
#define ASCII_CHAR_RANDOM  "r"
#define ASCII_CHAR_SORTED  "S"

#define UTF8_CHAR_PAUSE   "\u01c1"     // ǁ
#define UTF8_CHAR_PLAY    "\u00BB"     // »
#define UTF8_CHAR_RESTART "\u21BA"     // ↺
#define UTF8_CHAR_NORMAL  "\u21A6"     // ↦
#define UTF8_CHAR_RANDOM  "\u21AD"     // ↭
#define UTF8_CHAR_SORTED  "\u21F5"     // ⇵

#define GRAPHTFT_CHAR_PAUSE     "\x88"
#define GRAPHTFT_CHAR_PLAY      "\u00BB"     // »
#define GRAPHTFT_CHAR_RESTART   "\x02"
#define GRAPHTFT_CHAR_NORMAL    "\u00ff"
#define GRAPHTFT_CHAR_RANDOM    "\x01"
#define GRAPHTFT_CHAR_SORTED    "\x85"

#define GRAPHTFT_CHAR_DISK      "\x81"

class cCdPlayer: public cPlayer, public cThread {
protected:
    cBufferedCdio mBufCdio;
    uchar *pStillBuf;
    int mStillBufLen;
    bool mPlayRandom;
    volatile int mSpeed;
    volatile bool mTrackChange;  // Indication for external track change
    volatile bool mPurge;
    static const PCM_FREQ_T mSpeedTypes[MAX_SPEED+1];
    cMutex mPlayerMutex;

    virtual void Activate(bool On);
    void Action(void);
    void DeviceClear() {mPurge = true; cPlayer::DeviceClear();}
    void DisplayStillPicture (void);
    bool PlayData (const uint8_t *buf, int frame);
    cPlugin *mSpanPlugin;

public:
    cCdPlayer(void);
    virtual ~cCdPlayer();
    virtual double FramesPerSecond(void) { return (double)CDIO_CD_FRAMES_PER_SEC; };
    virtual bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
    virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
    void LoadStillPicture (const std::string FileName);

    void SetRestartMode (bool restart) {mBufCdio.SetRestartMode(restart);}
    void RandomPlay(void)
    {
        mBufCdio.RandomPlay();
        mPlayRandom = true;
    }
    void SortedPlay(void) {
        mBufCdio.SortedPlay();
        mPlayRandom = false;
    }

    void SetTrack(TRACK_IDX_T track);
    void NextTrack(void);
    void PrevTrack(void);
    void Stop(void);
    void Pause(void);

    void Play();
    void SpeedNormal(void) {mSpeed = 0;}
    void SpeedFaster(void) {if (mSpeed < MAX_SPEED) mSpeed++;}
    void SpeedSlower(void) {if (mSpeed > 0) mSpeed--;}
    void ChangeTime(int tm);
    TRACK_IDX_T GetNumTracks (void) {
        cMutexLock MutexLock(&mPlayerMutex);
        return mBufCdio.GetNumTracks();
    };
    // Return current track index and optional the total length in seconds and
    // the current second
    TRACK_IDX_T GetCurrTrack(int *total = NULL, int *curr = NULL) {
        cMutexLock MutexLock(&mPlayerMutex);
        return mBufCdio.GetCurrTrack(total, curr);
    };
    void GetCdTextFields(const TRACK_IDX_T track, CD_TEXT_T &txt) {
        cMutexLock MutexLock(&mPlayerMutex);
        mBufCdio.GetCdTextFields(track, txt);
    };
    void GetCdInfo (CD_TEXT_T &txt) {
        cMutexLock MutexLock(&mPlayerMutex);
        mBufCdio.GetCdInfo(txt);
    }
    int GetSpeed(void) {
        return mSpeed;
    }
    BUFCDIO_STATE_T GetState(void) {
        return mBufCdio.GetState();
    }
    const string &GetErrorText(void) {
        return mBufCdio.GetErrorText();
    }
    void GetTrackTime (const TRACK_IDX_T track, int *min, int *sec) {
        mBufCdio.GetTrackTime (track, min, sec);
    }

    bool CDDBInfoAvailable(void) {
        return mBufCdio.CDDBInfoAvailable();
    }
};

class cCdControl: public cControl {
private:
    typedef enum {
        CD_CHAR_PAUSE,
        CD_CHAR_PLAY,
        CD_CHAR_RESTART,
        CD_CHAR_NORMAL,
        CD_CHAR_RANDOM,
        CD_CHAR_SORTED,
        CH_CHAR_LAST
    } CD_MENU_CHAR;

    typedef enum {
        CD_DISPLAY_ASCII,
        CD_DISPLAY_UTF8,
        CD_DISPLAY_LAST
    } CD_DISPLAY_TYPE;

    cCdPlayer *mCdPlayer;
    cSkinDisplayMenu *mMenuPlaylist;
    cMutex mControlMutex;
    bool mShowDetail;
    bool mPlayRandom;
    bool mRestart;
    bool mIsUTF8;
    TRACK_IDX_T mCurrtitle;
    static const char *menukindPlayList;
    static const char *menukindDetail;
    static const char *specialMenu[CD_DISPLAY_LAST][CH_CHAR_LAST];

    void Replace (std::string &, const char *, const char *);
    const char *GetString(CD_MENU_CHAR cdchar)
    {
        if (mIsUTF8){
            return (specialMenu[CD_DISPLAY_UTF8][cdchar]);
        }
        return (specialMenu[CD_DISPLAY_ASCII][cdchar]);
    };
    char *BuildOSDStr(TRACK_IDX_T);
    char *BuildMenuStr(TRACK_IDX_T);
    void SetHelpkeys(void);
    void ShowDetail(void);
    void ShowList(void);
    void ShowPlaylist(void);
    void DisplayLine(const char *buf, int line);
public:
    cCdControl(void);
    virtual ~cCdControl();
    virtual void Hide(void);
    virtual cOsdObject *GetInfo(void) { return NULL; }
    virtual eOSState ProcessKey(eKeys Key);
};

#endif
