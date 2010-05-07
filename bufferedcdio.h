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

#include <string>
#include <map>
#include <vector>

#include <stdio.h>
#include <stdint.h>
#include <vdr/plugin.h>
#include <cdio/cdio.h>
#include <cdio/cd_types.h>

#include "cdioringbuf.h"

using namespace std;

// Maximum number of raw blocks to buffer
static const int CCDIO_MAX_BLOCKS=32;

typedef string CD_TEXT_T[MAX_CDTEXT_FIELDS];
typedef unsigned int TRACK_IDX_T;

// Track information for each track
class cTrackInfo {
private:
    friend class cBufferedCdio;
    track_t mTrackNo;
    lsn_t mStartLsn;
    lsn_t mEndLsn;
    CD_TEXT_T mCdTextFields;
public:
    cTrackInfo(void) : mTrackNo(0), mStartLsn(0), mEndLsn(0) {};
    cTrackInfo(lsn_t StartLsn, lsn_t EndLsn)
                : mTrackNo(0), mStartLsn(StartLsn), mEndLsn(EndLsn) {};
    track_t GetCDDATrack(void) { return mTrackNo; };
    lsn_t GetCDDAStartLsn(void) { return mStartLsn; };
    lsn_t GetCDDAEndLsn(void) { return mEndLsn; };
};

// Vector (array) containing all track information
typedef vector<cTrackInfo> TrackInfoVector;

typedef enum _bufcdio_state {
    BCDIO_STOP = 0,
    BCDIO_OPEN_DEVICE,
    BCDIO_PAUSE,
    BCDIO_PLAY,
    BCDIO_FAILED
} BUFCDIO_STATE_T;

// Class for accessing the audio cd
class cBufferedCdio: public cThread {
private:
    static const char *cd_text_field[MAX_CDTEXT_FIELDS+1];
    CdIo_t          *pCdio;
    track_t         mFirstTrackNum;  // CDIO first track
    track_t         mNumOfTracks;    // CDIO number of tracks

    TRACK_IDX_T     mCurrTrackIdx; // Audio Track index
    CD_TEXT_T       mCdText;       // CD-Text for entire CD
    TrackInfoVector mTrackInfo;    // CD Information per audio track
    cCdIoRingBuffer mRingBuffer;
    bool           mTrackChange;  // Indication for external track change
    BUFCDIO_STATE_T mState;
    cMutex          mCdMutex;
    void GetCDText(const track_t track_no, CD_TEXT_T &cd_text);
    bool ReadTrack (TRACK_IDX_T trackidx);

public:
    cBufferedCdio(void);
    ~cBufferedCdio(void);
    bool OpenDevice(const string &FileName);
    void CloseDevice(void);

    const TRACK_IDX_T GetCurrTrack(void) { return mCurrTrackIdx; };
    const char *GetCdTextField(const cdtext_field_t type);
    const CD_TEXT_T& GetCDInfo (void) {
        return mCdText;
    }
    const CD_TEXT_T& GetCdTextFields(const TRACK_IDX_T track) {
        return mTrackInfo[track].mCdTextFields;
    };
    const cTrackInfo &GetTrackInfo (const TRACK_IDX_T track) {
        return mTrackInfo[track];
    }
    const TRACK_IDX_T GetNumTracks (void) {
        return mTrackInfo.size();
    }
    bool GetData (uint8_t *data); // Get a raw audio block
    BUFCDIO_STATE_T GetState(void)  {return mState; };
    void Action(void);
    void SetTrack (TRACK_IDX_T newtrack);
    void NextTrack(void) {
        mCdMutex.Lock();
        if (mCurrTrackIdx < GetNumTracks()) SetTrack(mCurrTrackIdx+1);
        mCdMutex.Unlock();
    };
    void PrevTrack(void) {
        mCdMutex.Lock();
        if (mCurrTrackIdx > 0) SetTrack(mCurrTrackIdx-1);
        mCdMutex.Unlock();
    };
    void Stop(void) {
        mCdMutex.Lock();
        Cancel(5);
        CloseDevice();
        mCdMutex.Unlock();
    }
    void Pause(void) {
        if (mState == BCDIO_PLAY) mState = BCDIO_PAUSE;
        if (mState == BCDIO_PAUSE) mState = BCDIO_PLAY;
    }
};

#endif
