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

static const int CCDIO_MAX_BLOCKS=50;

typedef map<int, string> StringMap;
typedef int TRACK_IDX_T;

class cTrackInfo {
private:
    friend class cBufferedCdio;
    track_t mTrackNo;
    lsn_t mStartLsn;
    lsn_t mEndLsn;
    StringMap mCdTextFields;
public:
    cTrackInfo(void) : mTrackNo(0), mStartLsn(0), mEndLsn(0) {};
    cTrackInfo(lsn_t StartLsn, lsn_t EndLsn)
                : mTrackNo(0), mStartLsn(StartLsn), mEndLsn(EndLsn) {};
    track_t GetCDDATrack(void) { return mTrackNo; };
    lsn_t GetCDDAStartLsn(void) { return mStartLsn; };
    lsn_t GetCDDAEndLsn(void) { return mEndLsn; };
};

typedef vector<cTrackInfo> TrackInfoVector;

class cBufferedCdio: public cThread {
private:
    static const char *cd_text_field[MAX_CDTEXT_FIELDS+1];
    CdIo_t          *pCdio;
    track_t         mFirstTrackNum;  // CDIO first track
    track_t         mNumOfTracks;    // CDIO number of tracks

    TRACK_IDX_T     mCurrTrackIdx; // Audio Track index
    StringMap       mCdText;       // CD-Text for entire CD
    TrackInfoVector mTrackInfo;    // CD Information per audio track
    cCdIoRingBuffer mRingBuffer;

    void GetCDText(const track_t track_no, StringMap &cd_text);
    bool ReadTrack (TRACK_IDX_T trackidx);

public:
    cBufferedCdio(void);
    ~cBufferedCdio(void);
    bool OpenDevice(const string &FileName);
    void CloseDevice(void);

    const char *GetCdTextField(const cdtext_field_t type);
    const cTrackInfo &GetTrackInfo (const TRACK_IDX_T track) {
        return mTrackInfo[track];
    }
    const TRACK_IDX_T GetNumTracks (void) {
        return mTrackInfo.size();
    }
    bool GetData (uint8_t *data);
    void Action(void);
};

#endif
