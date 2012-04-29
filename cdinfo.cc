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

#include "cdinfo.h"
#include "cdplayer.h"

/*
 * Trackinfo class holds information about a single track.
 */
cTrackInfo::cTrackInfo(track_t TrackNo, lsn_t StartLsn, lsn_t EndLsn, lba_t lba,
                          CD_TEXT_T CdTextFields)
                          : mTrackNo(TrackNo), mStartLsn(StartLsn), mEndLsn(EndLsn),
                            mLba(lba)
{
    SetCdTextFields (CdTextFields);
}

void cTrackInfo::SetCdTextFields (const CD_TEXT_T CdTextFields)
{
    int i;
    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        mCdTextFields[i] = CdTextFields[i];
    }
}

void cTrackInfo::GetCDDATime(int *min, int *sec)
{
    int s = GetTimeSecs();
    *min = (s / 60);
    *sec = s - (*min * 60);
}

/*
 * cCdInfo class holds information about the entire CD.
 */
void cCdInfo::Add(track_t TrackNo, lsn_t StartLsn, lsn_t EndLsn, lba_t lba,
                  CD_TEXT_T &CdTextFields)
{
    cTrackInfo ti(TrackNo, StartLsn, EndLsn, lba, CdTextFields);
    mTrackInfo.push_back(ti);
    mPlayList.push_back(mLastTrackIdx);
    mLastTrackIdx++;
    AddData(lba);
}

/* Add Data required for CDDB query */
void cCdInfo::AddData(lba_t lba)
{
    cCddbInfo ci(lba);
    mCddbInfo.push_back(ci);
}

void cCdInfo::SetCdInfo(const CD_TEXT_T CdTextFields) {
    cMutexLock MutexLock (&mInfoMutex);
    int i;
    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        mCdText[i] = CdTextFields[i];
    }
}

void cCdInfo::GetCdInfo(CD_TEXT_T &txt) {
    cMutexLock MutexLock (&mInfoMutex);
    int i;
    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        txt[i] = mCdText[i];
    }
}

void cCdInfo::GetCdTextFields(const TRACK_IDX_T track, CD_TEXT_T &CdTextFields)
{
    cMutexLock MutexLock(&mInfoMutex);
    int i;

    if (track < (TRACK_IDX_T)mTrackInfo.size()) {
        for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
            CdTextFields[i] = mTrackInfo[track].mCdTextFields[i];
        }
    }
}

void cCdInfo::Action(void) {
    while (!mCddbInfoAvail) {
        Query();
        cCondWait::SleepMs(10000);
    }
}

void cCdInfo::Query(void) {
    TRACK_IDX_T i;
    int nrfound;
    cddb_conn_t *cddb_conn;
    cddb_disc_t *cddb_disc;
    cddb_track_t *track;
    CD_TEXT_T txt;

    dsyslog("CDDB Query started");
    cddb_conn = cddb_new();
    if (cddb_conn == NULL) {
        dsyslog("%s %d Cddb Connection failed %s",
                __FILE__, __LINE__, cddb_error_str(cddb_errno(cddb_conn)));
        return;
    }

    cddb_set_server_name(cddb_conn, cPluginCdplayer::GetCDDBServer().c_str());
    if (!cPluginCdplayer::GetCDDBCacheDir().empty()) {
        cddb_cache_set_dir(cddb_conn, cPluginCdplayer::GetCDDBCacheDir().c_str());
    }
    if (!cPluginCdplayer::GetCDDBCacheEnabled()) {
        cddb_cache_disable(cddb_conn);
    }

    cddb_disc = cddb_disc_new();
    if (cddb_disc == NULL) {
       esyslog("%s %d unable to create CDDB disc structure %s",
               __FILE__, __LINE__, cddb_error_str(cddb_errno(cddb_conn)));
       cddb_destroy(cddb_conn);
       return;
     }

     for (i = 0; i < (TRACK_IDX_T)mCddbInfo.size(); i++) {
       cddb_track_t *t = cddb_track_new();
       if (t == NULL) {
           esyslog("%s %d cddb_track_new failed %s",
                   __FILE__, __LINE__, cddb_error_str(cddb_errno(cddb_conn)));
           cddb_destroy(cddb_conn);
           return;
       }
       cddb_track_set_frame_offset(t,mCddbInfo[i].GetCDDALba());
       cddb_disc_add_track(cddb_disc, t);
     }

     cddb_disc_set_length(cddb_disc, mLeadOut / CDIO_CD_FRAMES_PER_SEC);

     if (!cddb_disc_calc_discid(cddb_disc)) {
       dsyslog("%s %d libcddb calc discid failed %s.",
               __FILE__, __LINE__, cddb_error_str(cddb_errno(cddb_conn)));
       cddb_destroy(cddb_conn);
       return;
     }

     nrfound = cddb_query(cddb_conn, cddb_disc);
     if (nrfound <= 0) {
        dsyslog("%s %d CDDB no result.", __FILE__, __LINE__);
        cddb_destroy(cddb_conn);
        return;
     }
     dsyslog("CDDB %d matches found", nrfound);
     if (cddb_read(cddb_conn, cddb_disc) == 0) {
         dsyslog("%s %d CDDB read failed %s.",
                 __FILE__, __LINE__, cddb_error_str(cddb_errno(cddb_conn)));
         cddb_destroy(cddb_conn);
         return;
     }
     GetCdInfo (txt);

     txt[CDTEXT_TITLE] = NotNull(cddb_disc_get_title(cddb_disc));
     txt[CDTEXT_SONGWRITER] = NotNull(cddb_disc_get_artist(cddb_disc));
     txt[CDTEXT_PERFORMER] = NotNull(cddb_disc_get_artist(cddb_disc));
     txt[CDTEXT_GENRE] = NotNull( cddb_disc_get_category_str(cddb_disc));
     SetCdInfo (txt);
     dsyslog("category: %s (%d) %08x",
             cddb_disc_get_category_str(cddb_disc),
             cddb_disc_get_category(cddb_disc),
             cddb_disc_get_discid(cddb_disc));
     dsyslog("%s by %s",
             cddb_disc_get_title(cddb_disc),
             cddb_disc_get_artist(cddb_disc));

     for (i = 0; i <  GetNumTracks(); i++) {
        track = cddb_disc_get_track(cddb_disc, i);
        if (track != NULL) {
            GetCdTextFields(i, txt);
            txt[CDTEXT_TITLE] = NotNull(cddb_track_get_title(track));
            txt[CDTEXT_SONGWRITER] = NotNull(cddb_track_get_artist(track));
            txt[CDTEXT_PERFORMER] = NotNull(cddb_track_get_artist(track));
            SetCdTextFields(i, txt);
        }
     }
     cddb_destroy(cddb_conn);
     dsyslog("CDDB Query finished");
     mCddbInfoAvail = true;
}

