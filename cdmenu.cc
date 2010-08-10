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

#include "cdmenu.h"

static const char *MAXCDSPEED = "MaxCDSpeed";
static const char *ENABLEPARANOIA = "EnableParanoia";
static const char *ENABLEMAINMENU = "EnableMainMenu";

int cMenuCDPlayer::mMaxSpeed = 8;
int cMenuCDPlayer::mShowMainMenu = true;

#ifdef USE_PARANOIA
int cMenuCDPlayer::mUseParanoia = true;
#else
int cMenuCDPlayer::mUseParanoia = false;
#endif

cMenuCDPlayer::cMenuCDPlayer(void) : cMenuSetupPage()
{
    SetSection (tr("CD-Player"));

    Add(new cMenuEditIntItem( tr("Max CD Speed"), &mMaxSpeed));
    Add(new cMenuEditBoolItem(tr("Show in main menu"), &mShowMainMenu));
#ifdef USE_PARANOIA
    Add(new cMenuEditBoolItem(tr("Enable Paranoia"), &mUseParanoia));
#else
    mUseParanoia = false;
#endif
}

const bool cMenuCDPlayer::SetupParse(const char *Name, const char *Value)
{
  // Parse setup parameters and store their values.
  if (strcasecmp(Name, MAXCDSPEED) == 0) {
      mMaxSpeed = atoi(Value);
      if (mMaxSpeed < 1) {
          mMaxSpeed = 1;
      }
  }
  else if (strcasecmp(Name, ENABLEPARANOIA) == 0) {
      mUseParanoia = atoi(Value);
  }
  else if (strcasecmp(Name, ENABLEMAINMENU) == 0) {
      mShowMainMenu = atoi(Value);
  }
  else {
     return false;
  }
  return true;
}

void cMenuCDPlayer::Store(void)
{
    SetupStore(MAXCDSPEED, mMaxSpeed);
    SetupStore(ENABLEPARANOIA, (int)mUseParanoia);
    SetupStore(ENABLEMAINMENU, (int)mShowMainMenu);
}