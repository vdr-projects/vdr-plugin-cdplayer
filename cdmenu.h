/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
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

protected:
    virtual void Store(void);

public:
    cMenuCDPlayer(void);
    static const int GetMaxSpeed(void) { return mMaxSpeed; }
    static const bool GetUseParanoia(void) { return mUseParanoia; }
    static const bool GetShowMainMenu(void) { return mShowMainMenu; }
    static const bool SetupParse(const char *Name, const char *Value);
};
