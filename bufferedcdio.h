/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements buffered access to the audio cd via
 * libcdda and gets all track information via CD-Text or cddb.
 *
 */

#ifndef __BUFFEREDCDIO_H__
#define __BUFFEREDCDIO_H__

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY 1

#include <string>
#include <stdio.h>
#include <stdint.h>
#include <vdr/plugin.h>
#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include <cdio/paranoia.h>
#include "cdioringbuf.h"
#include "cdinfo.h"

#define USE_PARANOIA 1


using namespace std;

// Maximum number of raw blocks to buffer
static const int CCDIO_MAX_BLOCKS=32;

typedef enum _bufcdio_state {
    BCDIO_STOP = 0,
    BCDIO_STARTING,
    BCDIO_OPEN_DEVICE,
    BCDIO_PAUSE,
    BCDIO_PLAY,
    BCDIO_NEWTIME,
    BCDIO_FAILED
} BUFCDIO_STATE_T;

// Class for accessing the audio cd
class cBufferedCdio: public cThread {
private:
    static const char *cd_text_field[MAX_CDTEXT_FIELDS+1];
#ifdef USE_PARANOIA
    cdrom_drive_t   *pParanoiaDrive;
    cdrom_paranoia_t *pParanoiaCd;
#endif
    CdIo_t          *pCdio;

    track_t         mFirstTrackNum;  // CDIO first track
    track_t         mNumOfTracks;    // CDIO number of tracks

    lsn_t           mStartLsn;
    lsn_t           mCurrLsn;
    TRACK_IDX_T     mCurrTrackIdx; // Audio Track index
    cCdInfo         mCdInfo;    // CD Information per audio track
    cCdIoRingBuffer mRingBuffer;
    bool           mTrackChange;  // Indication for external track change
    BUFCDIO_STATE_T mState;
    cMutex          mCdMutex;
    string mErrtxt;

    void GetCDText(const track_t track_no, CD_TEXT_T &cd_text);
    bool ReadTrack (TRACK_IDX_T trackidx);
#ifdef USE_PARANOIA
    bool ParanoiaLogMsg(void);
#endif

public:
    cBufferedCdio(void);
    ~cBufferedCdio(void);
    bool OpenDevice(const string &FileName);
    void CloseDevice(void);

    const string &GetErrorText(void) { return mErrtxt; };
    const TRACK_IDX_T GetCurrTrack(int *total = NULL, int *curr=NULL);
    static const char *GetCdTextField(const cdtext_field_t type);
    void GetCdInfo (CD_TEXT_T &txt) {
       mCdInfo.GetCdInfo(txt);
    }
    void GetCdTextFields(const TRACK_IDX_T track, CD_TEXT_T &txt) {
        mCdInfo.GetCdTextFields(track, txt);
    };
    lsn_t GetStartLsn (const TRACK_IDX_T track) {
        return mCdInfo.GetStartLsn(track);
    }
    lsn_t GetEndLsn (const TRACK_IDX_T track) {
        return mCdInfo.GetEndLsn(track);
    }
    void GetTrackTime (const TRACK_IDX_T track, int *min, int *sec) {
        return mCdInfo.GetTrackTime (track, min, sec);
    }
    const TRACK_IDX_T GetNumTracks (void) {
        return mCdInfo.GetNumTracks();
    }
    bool GetData (uint8_t *data); // Get a raw audio block
    BUFCDIO_STATE_T GetState(void)  {
        return mState;
    };
    void Action(void);
    void SetTrack (TRACK_IDX_T newtrack);
    void NextTrack(void) {
        cMutexLock MutexLock(&mCdMutex);
        SetTrack(mCurrTrackIdx+1);
    };
    void PrevTrack(void) {
        cMutexLock MutexLock(&mCdMutex);
        if (mCurrTrackIdx > 0) SetTrack(mCurrTrackIdx-1);
    };
    void Stop(void) {
        mState = BCDIO_STOP;
        Cancel(5);
        CloseDevice();

    }
    void Pause(void) {
        if (mState == BCDIO_PLAY) mState = BCDIO_PAUSE;
        else if (mState == BCDIO_PAUSE) mState = BCDIO_PLAY;
    }

    bool CDDBInfoAvailable(void) {
        return mCdInfo.CDDBInfoAvailable();
    }
    void SkipTime(int tm);
};

#endif
