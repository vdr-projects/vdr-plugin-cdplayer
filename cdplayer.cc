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
            dsyslog("CD Player unknown option %c", c);
            return false;
        }
    }
    return true;
}

bool cPluginCdplayer::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}

bool cPluginCdplayer::Start(void)
{
    // Start any background activities the plugin shall perform.
    dsyslog("cPluginCdplayer Start");
    return true;
}

void cPluginCdplayer::Stop(void)
{
    dsyslog("cPluginCdplayer Stop");
  // Stop any background activities the plugin is performing.
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
    return tr(MAINMENUENTRY);
}

cOsdObject *cPluginCdplayer::MainMenuAction(void)
{
    mCdControl = new cCdControl();
    cControl::Launch(mCdControl);
    return NULL;
}

cMenuSetupPage *cPluginCdplayer::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginCdplayer::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginCdplayer::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginCdplayer::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginCdplayer::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginCdplayer); // Don't touch this!
