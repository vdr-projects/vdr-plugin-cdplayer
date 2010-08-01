/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the main plugin class
 *
 */

#include <getopt.h>
#include <stdlib.h>
#include "cdplayer.h"
#include "cdmenu.h"
#include <vdr/remote.h>

static const char *MAINMENUENTRY  = trNOOP("CD-Player");

// Defaults for command line arguments

std::string cPluginCdplayer::mDevice ="/dev/cdrom";
std::string cPluginCdplayer::mStillPicture = "cd.mpg";
std::string cPluginCdplayer::mcfgDir = "cdplayer";
std::string cPluginCdplayer::mCDDBServer = "freedb.freedb.org";
std::string cPluginCdplayer::mCDDBCacheDir = "";
bool cPluginCdplayer::mEnableCDDB = true;
bool cPluginCdplayer::mEnableCDDBCache = true;

cPluginCdplayer::cPluginCdplayer(void) : mCdControl(NULL)
{

}

const char *cPluginCdplayer::CommandLineHelp(void)
{
    return "-d  --device  <device>    CD-Rom device : /dev/cdrom\n"
            "-s  --stillpic <file>     Still-Picture : cd.mpg\n"
            "-c  --configdir <dir>     Directory for config files : cdplayer\n"
            "-S  --cddbserver <server> CDDB server name : freedb.freedb.org\n"
            "-C  --cddbcache <dir>     CDDB cache directory\n"
            "-N  --disablecddbcache    Disable CDDB cache\n"
            "-n  --disablecddb         Disable CDDB query\n";
}

bool cPluginCdplayer::ProcessArgs(int argc, char *argv[])
{
    cMutexLock MutexLock(&mCdMutex);
    static struct option long_options[] =
    {
        { "device",         required_argument, NULL, 'd' },
        { "stillpic",       required_argument, NULL, 's' },
        { "configdir",      required_argument, NULL, 'c' },
        { "cddbserver",     required_argument, NULL, 'S' },
        { "cddbcache",      required_argument, NULL, 'C' },
        { "disablecddb",        no_argument, NULL, 'n' },
        { "disablecddbcache",   no_argument, NULL, 'N' },
        { NULL }
    };
    int c, option_index = 0;

    while ((c = getopt_long(argc, argv, "d:s:c:S:C:nN",
                            long_options, &option_index)) != -1) {
        switch (c) {
        case 'd':
            mDevice.assign(optarg);
            break;
        case 's':
            mStillPicture.assign(optarg);
            break;
        case 'c':
            mcfgDir.assign(optarg);
            break;
        case 'S':
            mCDDBServer.assign(optarg);
            break;
        case 'C':
            mCDDBCacheDir.assign(optarg);
            break;
        case 'n':
            mEnableCDDB = false;
            break;
        case 'N':
            mEnableCDDBCache = false;
            break;
        default:
            esyslog("CD Player unknown option %c", c);
            return false;
        }
    }

    // Disable CDDB when no home and no CDDB cache dir is given.
    if ((getenv("HOME") == NULL) && (mEnableCDDB)) {
        if (mCDDBCacheDir.empty()) {
            esyslog("No HOME set and no cache dir for CDDB, disable CDDB");
            mEnableCDDB = false;
        }
        else {
            esyslog("Define HOME to %s", mCDDBCacheDir.c_str());
            if (setenv("HOME", mCDDBCacheDir.c_str(), 0) != 0) {
                esyslog ("setenv failed %d", errno);
                mEnableCDDB = false;
            }
        }
    }
    if (!mEnableCDDB) {
        dsyslog ("CDDB disabled");
    }
    return true;
}

bool cPluginCdplayer::Initialize(void)
{
    return true;
}

bool cPluginCdplayer::Start(void)
{
    return true;
}

void cPluginCdplayer::Stop(void)
{
  // Stop any background activities the plugin is performing.
    cMutexLock MutexLock(&mCdMutex);
    if (mCdControl != NULL) {
        mCdControl->ProcessKey(kStop);
    }
}

void cPluginCdplayer::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginCdplayer::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginCdplayer::Active(void)
{
    // Return a message string if shutdown should be postponed
    return NULL;
}

time_t cPluginCdplayer::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

const char *cPluginCdplayer::MainMenuEntry(void)
{
    if (cMenuCDPlayer::GetShowMainMenu()) {
        return tr(MAINMENUENTRY);
    }
    return NULL;
}

cOsdObject *cPluginCdplayer::MainMenuAction(void)
{
    cMutexLock MutexLock(&mCdMutex);
    mCdControl = new cCdControl();
    cControl::Launch(mCdControl);
    return NULL;
}

cMenuSetupPage *cPluginCdplayer::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
    return new cMenuCDPlayer();
}

bool cPluginCdplayer::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return cMenuCDPlayer::SetupParse(Name, Value);
}

bool cPluginCdplayer::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginCdplayer::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {
            "PLAY:  Play a disk\n",
            "PAUSE: Pause\n",
            "STOP:  Stop\n",
            "NEXT:  Next title\n",
            "PREV:  Previous title\n",
            NULL
    };
    return HelpPages;
}

cString cPluginCdplayer::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    cMutexLock MutexLock(&mCdMutex);
    if ((strcasecmp(Command, "PLAY") == 0) && (mCdControl == NULL)) {
        cRemote::CallPlugin(Name());
        return "OK";
    }
    if (mCdControl != NULL) {
        if (strcasecmp(Command, "PLAY") == 0) {
            mCdControl->ProcessKey(kPlay);
        }
        else if (strcasecmp(Command, "STOP") == 0) {
            mCdControl->ProcessKey(kStop);
        }
        else if (strcasecmp(Command, "PAUSE") == 0) {
            mCdControl->ProcessKey(kPause);
        }
        else if (strcasecmp(Command, "NEXT") == 0) {
            mCdControl->ProcessKey(kNext);
        }
        else if (strcasecmp(Command, "PREV") == 0) {
            mCdControl->ProcessKey(kPrev);
        }
        else {
            return NULL;
        }
        return "OK";
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginCdplayer); // Don't touch this!
