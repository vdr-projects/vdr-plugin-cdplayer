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

#include "cdmenu.h"

static const char *MAXCDSPEED = "MaxCDSpeed";
static const char *ENABLEPARANOIA = "EnableParanoia";
static const char *ENABLEMAINMENU = "EnableMainMenu";
static const char *PLAYMODE = "PlayMode";
static const char *SHOWPERFORMER ="ShowArtist";
static const char *RESTART="Restart";
static const char *GRAPHTFT = "GraphTFT";
static const char *KEY_OK = "KeyOk";
static const char *KEY_BACK = "KeyBack";

int cMenuCDPlayer::mMaxSpeed = 8;
int cMenuCDPlayer::mShowMainMenu = true;
int cMenuCDPlayer::mPlayMode = false;
int cMenuCDPlayer::mShowArtist = true;
int cMenuCDPlayer::mRestart = false;
int cMenuCDPlayer::mGraphTFT = false;
cMenuCDPlayer::KEY_ASSIGNMENT cMenuCDPlayer::mOK_Key = KEY_EXIT;
cMenuCDPlayer::KEY_ASSIGNMENT cMenuCDPlayer::mBACK_Key = KEY_EXIT;

#ifdef USE_PARANOIA
int cMenuCDPlayer::mUseParanoia = true;
#else
int cMenuCDPlayer::mUseParanoia = false;
#endif

cMenuCDPlayer::cMenuCDPlayer(void) : cMenuSetupPage()
{
    static const char *playmode_entry[2] = {
            tr("Sorted"),
            tr("Random")
    };
    static const char *key_assignment[KEY_LAST] = {
            tr ("No Function"),
            tr ("Pause"),
            tr ("Exit")
    };
    SetSection (tr("CD-Player"));

    Add(new cMenuEditIntItem(tr("Max CD speed"), &mMaxSpeed));
    Add(new cMenuEditBoolItem(tr("Show in main menu"), &mShowMainMenu));
    Add(new cMenuEditStraItem(tr("Play mode"), &mPlayMode,
                                  2, playmode_entry));
    Add(new cMenuEditBoolItem(tr("Show artist"), &mShowArtist));
    Add(new cMenuEditBoolItem(tr("Restart playback"), &mRestart));
#ifdef USE_PARANOIA
    Add(new cMenuEditBoolItem(tr("Enable Paranoia"), &mUseParanoia));
#else
    mUseParanoia = false;
#endif
    Add(new cMenuEditBoolItem(tr("Use GraphTFT special characters"), &mGraphTFT));
    Add(new cMenuEditStraItem(tr("Back Key"), (int *)&mBACK_Key, KEY_LAST,
                              key_assignment));
    Add(new cMenuEditStraItem(tr("OK Key"), (int *)&mOK_Key, KEY_LAST,
                                  key_assignment));
}

bool cMenuCDPlayer::SetupParse(const char *Name, const char *Value)
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
  else if (strcasecmp(Name, PLAYMODE) == 0) {
      mPlayMode = atoi(Value);
  }
  else if (strcasecmp(Name, SHOWPERFORMER) == 0) {
      mShowArtist = atoi(Value);
  }
  else if (strcasecmp(Name, RESTART) == 0) {
      mRestart = atoi(Value);
  }
  else if (strcasecmp(Name, GRAPHTFT) == 0) {
      mGraphTFT  = atoi(Value);
  }
  else if (strcasecmp(Name, KEY_OK) == 0) {
      mOK_Key = (cMenuCDPlayer::KEY_ASSIGNMENT)atoi(Value);
  }
  else if (strcasecmp(Name, KEY_BACK) == 0) {
      mBACK_Key = (cMenuCDPlayer::KEY_ASSIGNMENT)atoi(Value);
  }
  else {
     return false;
  }
  return true;
}

void cMenuCDPlayer::Store(void)
{
    SetupStore(MAXCDSPEED, mMaxSpeed);
    SetupStore(ENABLEPARANOIA, mUseParanoia);
    SetupStore(ENABLEMAINMENU, mShowMainMenu);
    SetupStore(PLAYMODE, mPlayMode);
    SetupStore(SHOWPERFORMER, mShowArtist);
    SetupStore(RESTART, mRestart);
    SetupStore(GRAPHTFT, mGraphTFT);
    SetupStore(KEY_OK, (int)mOK_Key);
    SetupStore(KEY_BACK, (int)mBACK_Key);
}

eKeys cMenuCDPlayer::TranslateKey (KEY_ASSIGNMENT key) {
    if (key == KEY_PAUSE) {
        return (kPause);
    }
    else if (key == KEY_EXIT) {
        return (kStop);
    }
    return (kNone);
}
