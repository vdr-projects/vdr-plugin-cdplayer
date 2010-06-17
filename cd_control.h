/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
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

// The maximum size of a single frame (up to HDTV 1920x1080):
#define TS_SIZE 188
#define CDMAXFRAMESIZE  (KILOBYTE(1024) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE to avoid breaking up TS packets
#define MAX_SPEED 2

class cCdPlayer: public cPlayer, public cThread {
protected:

    cBufferedCdio cdio;
    uchar *pStillBuf;
    int mStillBufLen;
    int mSpeed;
    static const PCM_FREQ_T mSpeedTypes[MAX_SPEED+1];
    cMutex mPlayerMutex;
    virtual void Activate(bool On);
    void Action(void);
    void DisplayStillPicture (void);
public:
    cCdPlayer(void);
    virtual ~cCdPlayer();
    virtual bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
    virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
    void LoadStillPicture (const std::string FileName);

    void NextTrack(void) {cdio.NextTrack();}
    void PrevTrack(void) {cdio.PrevTrack();}
    void Stop(void);
    void Pause(void) {cdio.Pause();}
    void SpeedNormal(void) {mSpeed = 0;}
    void SpeedFaster(void) {if (mSpeed < MAX_SPEED) mSpeed++;}
    void SpeedSlower(void) {if (mSpeed > 0) mSpeed--;}
    void ChangeTime(int tm) {cdio.SkipTime(tm);}
    const TRACK_IDX_T GetNumTracks (void) {
        cMutexLock MutexLock(&mPlayerMutex);
        return cdio.GetNumTracks();
    };
    // Return current track index and optional the total length in seconds and
    // the current second
    const TRACK_IDX_T GetCurrTrack(int *total = NULL, int *curr = NULL) {
        cMutexLock MutexLock(&mPlayerMutex);
        return cdio.GetCurrTrack(total, curr);
    };
    void GetCdTextFields(const TRACK_IDX_T track, CD_TEXT_T &txt) {
        cMutexLock MutexLock(&mPlayerMutex);
        cdio.GetCdTextFields(track, txt);
    };
    void GetCdInfo (CD_TEXT_T &txt) {
        cMutexLock MutexLock(&mPlayerMutex);
        cdio.GetCdInfo(txt);
    }
    int GetSpeed(void) {
        return mSpeed;
    }
    BUFCDIO_STATE_T GetState(void) {
        return cdio.GetState();
    }
    const string &GetErrorText(void) {
        return cdio.GetErrorText();
    }
    void GetTrackTime (const TRACK_IDX_T track, int *min, int *sec) {
        cdio.GetTrackTime (track, min, sec);
    }

    bool CDDBInfoAvailable(void) {
        return cdio.CDDBInfoAvailable();
    }
};

class cCdControl: public cControl {
private:
    cCdPlayer *mCdPlayer;
    cSkinDisplayMenu *mMenuPlaylist;
    cMutex mControlMutex;
    bool mShowDetail;
    static const char *menutitle;
    static const char *menukindPlayList;
    static const char *menukindDetail;
    static const char *redtxt;
    static const char *greentxt;
    static const char *yellowtxt;
    static const char *bluetxtplay;
    static const char *bluetxtdet;
    char *BuildOSDStr(TRACK_IDX_T);
    char *BuildMenuStr(TRACK_IDX_T);
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
