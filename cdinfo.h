/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements access to the audio track information via
 * CD-Text or cddb.
 *
 */

#ifndef CDINFO_H_
#define CDINFO_H_

#include <vdr/plugin.h>
#include <cdio/cdio.h>
#include <cdio/cd_types.h>
#include <cddb/cddb.h>
#include <vector>
#include <string>

typedef std::string CD_TEXT_T[MAX_CDTEXT_FIELDS];
typedef unsigned int TRACK_IDX_T;

// Track information for each track
class cTrackInfo {
private:
    friend class cCdInfo;
    track_t mTrackNo;
    lsn_t mStartLsn;
    lsn_t mEndLsn;
    CD_TEXT_T mCdTextFields;
public:
    cTrackInfo(void) : mTrackNo(0), mStartLsn(0), mEndLsn(0) {};
    cTrackInfo(lsn_t StartLsn, lsn_t EndLsn, CD_TEXT_T CdTextFields);
    track_t GetCDDATrack(void) { return mTrackNo; };
    lsn_t GetCDDAStartLsn(void) { return mStartLsn; };
    lsn_t GetCDDAEndLsn(void) { return mEndLsn; };
    void GetCDDATime(int *min, int *sec);
};

// Vector (array) containing all track information
typedef std::vector<cTrackInfo> TrackInfoVector;

class cCdInfo: public cThread {
private:
    TrackInfoVector mTrackInfo;
public:
    cCdInfo(void) {};
    ~cCdInfo(void) {Cancel(3);};

    void Clear(void) {
        mTrackInfo.clear();
    }
    void Add(lsn_t StartLsn, lsn_t EndLsn, CD_TEXT_T &CdTextFields);
    const CD_TEXT_T& GetCdTextFields(const TRACK_IDX_T track) {
        return mTrackInfo[track].mCdTextFields;
    }
    const TRACK_IDX_T GetNumTracks (void) {
        return mTrackInfo.size();
    }
    lsn_t GetStartLsn(const TRACK_IDX_T track) {
        return mTrackInfo[track].GetCDDAStartLsn();
    }
    lsn_t GetEndLsn(const TRACK_IDX_T track) {
        return mTrackInfo[track].GetCDDAEndLsn();
    }
    void GetTrackTime (const TRACK_IDX_T track, int *min, int *sec) {
        mTrackInfo[track].GetCDDATime(min, sec);
    }
    void Action(void); // Start cddb query
};


#endif /* CDINFO_H_ */
