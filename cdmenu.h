/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010-2012 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the configuration menu
 *
 */

#include <vdr/plugin.h>
#include "cdplayer.h"

class cMenuCDPlayer: public cMenuSetupPage {
private:
    static int mMaxSpeed;
    static int mUseParanoia;
    static int mShowMainMenu;
    static int mPlayMode;
    static int mShowArtist;
    static int mRestart;
protected:
    virtual void Store(void);

public:
    cMenuCDPlayer(void);
    static int GetMaxSpeed(void) {return mMaxSpeed;}
    static bool GetUseParanoia(void) {return mUseParanoia;}
    static bool GetShowMainMenu(void) {return mShowMainMenu;}
    static bool GetPlayMode(void) {return mPlayMode; }
    static bool GetShowArtist(void) {return mShowArtist;}
    static bool GetRestart(void) {return mRestart;}
    static bool SetupParse(const char *Name, const char *Value);
};
