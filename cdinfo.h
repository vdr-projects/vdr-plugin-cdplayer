/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010-2012 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
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

#include <cdio/cdio.h>
#include <cdio/cd_types.h>
#include <cdio/version.h>

#ifdef VERSION
#undef VERSION
#endif

#include <vdr/plugin.h>
#include <cddb/cddb.h>
#include <vector>
#include <string>

#if LIBCDIO_VERSION_NUM > 83

#define CDTEXT_COMPOSER     CDTEXT_FIELD_COMPOSER
#define CDTEXT_DISCID       CDTEXT_FIELD_DISCID
#define CDTEXT_GENRE        CDTEXT_FIELD_GENRE
#define CDTEXT_MESSAGE      CDTEXT_FIELD_MESSAGE
#define CDTEXT_ISRC         CDTEXT_FIELD_ISRC
#define CDTEXT_PERFORMER    CDTEXT_FIELD_PERFORMER
#define CDTEXT_SONGWRITER   CDTEXT_FIELD_SONGWRITER
#define CDTEXT_TITLE        CDTEXT_FIELD_TITLE
#define CDTEXT_UPC_EAN      CDTEXT_FIELD_UPC_EAN
#define CDTEXT_ARRANGER     CDTEXT_FIELD_ARRANGER
#define CDTEXT_INVALID      CDTEXT_FIELD_INVALID
#endif

typedef std::string CD_TEXT_T[MAX_CDTEXT_FIELDS];
typedef int TRACK_IDX_T;

#define INVALID_TRACK_IDX ((TRACK_IDX_T)-1)

// Track information for each track
class cTrackInfo {
private:
    friend class cCdInfo;
    track_t mTrackNo;
    lsn_t mStartLsn;
    lsn_t mEndLsn;
    lba_t mLba;
    CD_TEXT_T mCdTextFields;
public:
    cTrackInfo(void) : mTrackNo(0), mStartLsn(0), mEndLsn(0), mLba(0) {}
    cTrackInfo(track_t TrackNo, lsn_t StartLsn, lsn_t EndLsn, lba_t lba,
               CD_TEXT_T CdTextFields);
    ~cTrackInfo() {}
    void SetCdTextFields (const CD_TEXT_T CdTextFields);
    track_t GetCDDATrack(void) { return mTrackNo; }
    lsn_t GetCDDAStartLsn(void) { return mStartLsn; }
    lsn_t GetCDDAEndLsn(void) { return mEndLsn; }
    lba_t GetCDDALba(void) { return mLba; }
    void GetCDDATime(int *min, int *sec);
    int GetTimeSecs(void) {return (mEndLsn - mStartLsn) / CDIO_CD_FRAMES_PER_SEC;}
};

// Track information for each track for cddb query (includes also data tracks)
class cCddbInfo {
private:
    friend class cCdInfo;
    lba_t mLba;
public:
    cCddbInfo(void) : mLba(0) {}
    cCddbInfo(lba_t lba) { mLba = lba; }
    lba_t GetCDDALba(void) { return mLba; }
};

// Vector (array) containing all track information
typedef std::vector<cTrackInfo> TrackInfoVector;
// Vector containing all track information required for CDDB query
typedef std::vector<cCddbInfo> CddbInfoVector;
// List containing the PlayList
typedef std::vector<track_t> PlayList;

class cCdInfo: public cThread {
private:
    TRACK_IDX_T mLastTrackIdx;
    TrackInfoVector mTrackInfo;
    CddbInfoVector mCddbInfo;
    PlayList mPlayList;

    lba_t mLeadOut;
    cMutex mInfoMutex;
    CD_TEXT_T mCdText;
    bool mCddbInfoAvail;
    void Query(void);
    void SetCdTextFields(const TRACK_IDX_T track, CD_TEXT_T CdTextFields) {
          cMutexLock MutexLock (&mInfoMutex);
          mTrackInfo[track].SetCdTextFields(CdTextFields);
     }
public:
    cCdInfo(void) {mCddbInfoAvail = false; mLastTrackIdx = 0; mLeadOut = 0;}
    ~cCdInfo(void) {if (Active()) Cancel(3);}

    void Clear(void) {
        mTrackInfo.clear();
        mPlayList.clear();
        mLastTrackIdx = 0;
    }

    void Add(track_t TrackNo, lsn_t StartLsn, lsn_t EndLsn, lba_t lba, CD_TEXT_T &CdTextFields);
    void AddData(lba_t lba);

    void SetLeadOut (lba_t leadout) { mLeadOut = leadout; }

    void GetCdTextFields(const TRACK_IDX_T track, CD_TEXT_T &CdTextFields);

    void SetCdInfo(const CD_TEXT_T CdTextFields);
    void GetCdInfo(CD_TEXT_T &txt);

    TRACK_IDX_T GetNumTracks(void) {
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
    bool CDDBInfoAvailable(void) {
        return mCddbInfoAvail;
    }
    PlayList GetDefaultPlayList(void) {
        return mPlayList;
    }
    void Action(void); // Start cddb query
};


#endif /* CDINFO_H_ */
