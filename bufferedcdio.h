/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010-2012 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
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
#include <cdio/mmc.h>
#include "cdioringbuf.h"
#include "cdinfo.h"

using namespace std;

// Maximum number of raw blocks to buffer
static const int CCDIO_MAX_BLOCKS=128;

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
    PlayList mPlayList;
    track_t mFirstTrackNum;  // CDIO first track
    track_t mNumOfTracks;    // CDIO number of tracks

    volatile lsn_t             mStartLsn;
    volatile lsn_t             mCurrLsn;
    volatile TRACK_IDX_T       mCurrTrackIdx; // Audio Track index
    volatile bool mTrackChange;  // Indication for external track change

    cCdInfo         mCdInfo;    // CD Information per audio track
    cCdIoRingBuffer mRingBuffer;
    BUFCDIO_STATE_T mState;
    cMutex          mCdMutex;
    volatile int  mSpeed;
    string mErrtxt;
// Buffer statistics
    int mBufferStat;
    int mBufferCnt;

    void GetCDText(const track_t track_no, CD_TEXT_T &cd_text);
    bool ReadTrack (TRACK_IDX_T trackidx);
    void SetSpeed (int speed);
    void SkipTimeFwd(lsn_t lsncnt);
    void SkipTimeBack(lsn_t lsncnt);
    TRACK_IDX_T GetTrackPlaylist (const TRACK_IDX_T track) {
        return mPlayList[track];
    }
#ifdef USE_PARANOIA
    bool ParanoiaLogMsg(void);
#endif

    // Span Plugin
    void SendToSpanPlugin(const uchar *data, int len, int frame);
    cPlugin *mSpanPlugin;

public:
    cBufferedCdio(void);
    ~cBufferedCdio(void);
    bool OpenDevice(const string &FileName);
    void CloseDevice(void);

    PlayList GetDefaultPlayList (void) {
        return mCdInfo.GetDefaultPlayList();
    }
    void SetPlayList (const PlayList li) {
        mPlayList = li;
    }

    const string &GetErrorText(void) { return mErrtxt; };

    TRACK_IDX_T GetCurrTrack(int *total = NULL, int *curr=NULL);
    static const char *GetCdTextField(const cdtext_field_t type);
    void GetCdInfo (CD_TEXT_T &txt) {
       mCdInfo.GetCdInfo(txt);
    }
    void GetCdTextFields(const TRACK_IDX_T track, CD_TEXT_T &txt) {
        mCdInfo.GetCdTextFields(GetTrackPlaylist(track), txt);
    };
    lsn_t GetStartLsn (const TRACK_IDX_T track) {
        return mCdInfo.GetStartLsn(GetTrackPlaylist(track));
    }
    lsn_t GetEndLsn (const TRACK_IDX_T track) {
        return mCdInfo.GetEndLsn(GetTrackPlaylist(track));
    }
    lsn_t GetLengthLsn (const TRACK_IDX_T track) {
        TRACK_IDX_T t = GetTrackPlaylist(track);
        return (mCdInfo.GetEndLsn(t)-mCdInfo.GetStartLsn(t));
    }
    void GetTrackTime (const TRACK_IDX_T track, int *min, int *sec) {
        return mCdInfo.GetTrackTime (GetTrackPlaylist(track), min, sec);
    }
    TRACK_IDX_T GetNumTracks (void) {
        return mCdInfo.GetNumTracks();
    }
    bool GetData (uint8_t *data, lsn_t *lsn, int *frame); // Get a raw audio block
    BUFCDIO_STATE_T GetState(void) {
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
        cMutexLock MutexLock(&mCdMutex);
        mState = BCDIO_STOP;
        Cancel(5);
        CloseDevice();
    }

    void Play(void) {
         if (mState == BCDIO_PAUSE) mState = BCDIO_PLAY;
    }
    void Pause(void) {
        if (mState == BCDIO_PLAY) mState = BCDIO_PAUSE;
        else if (mState == BCDIO_PAUSE) mState = BCDIO_PLAY;
    }

    bool CDDBInfoAvailable(void) {
        return mCdInfo.CDDBInfoAvailable();
    }
    void SkipTime(int tm);

    // Wait until some buffers are available on first play.
    void WaitBuffer (void) { mRingBuffer.WaitBlocksAvail (CCDIO_MAX_BLOCKS/4); }
};

#endif
