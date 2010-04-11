/*
 * cdplayer.cc: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <getopt.h>
#include <stdlib.h>
#include "cdplayer.h"

static const char *MAINMENUENTRY  = trNOOP("CD-Player");

const char *cPluginCdplayer::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginCdplayer::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
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
  return true;
}

void cPluginCdplayer::Stop(void)
{
  // Stop any background activities the plugin is performing.
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
  cControl::Launch(new cCdControl());
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
