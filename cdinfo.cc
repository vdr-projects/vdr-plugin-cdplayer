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

/*
 * Trackinfo class holds information about a single track.
 */
cTrackInfo::cTrackInfo(lsn_t StartLsn, lsn_t EndLsn, CD_TEXT_T CdTextFields)
                           : mTrackNo(0), mStartLsn(StartLsn), mEndLsn(EndLsn)
{
    int i;
    for (i = 0; i < MAX_CDTEXT_FIELDS; i++) {
        mCdTextFields[i] = CdTextFields[i];
    }
}

void cTrackInfo::GetCDDATime(int *min, int *sec)
{
    int s = (mEndLsn - mStartLsn) / CDIO_CD_FRAMES_PER_SEC;
    *min = (s / 60);
    *sec = s - (*min * 60);
}

/*
 * cCdInfo class holds information about the entire CD.
 */
void cCdInfo::Add(lsn_t StartLsn, lsn_t EndLsn, CD_TEXT_T &CdTextFields)
{
    cTrackInfo ti(StartLsn, EndLsn, CdTextFields);
    mTrackInfo.push_back(ti);
}

void cCdInfo::Action(void) {
    printf("Start CDDB quiery");
}
