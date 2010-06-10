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

#ifndef _CDPLAYER_H
#define _CDPLAYER_H

#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/player.h>
#include <cctype>
#include <string>
#include "cd_control.h"

static const char *VERSION        = "0.0.2";
static const char *DESCRIPTION    = trNOOP("CD-Player");

class cPluginCdplayer: public cPlugin {
private:
    static std::string mDevice;
    static std::string mStillPicture;
    static std::string mcfgDir;
public:
    cPluginCdplayer(void);
    virtual ~cPluginCdplayer() {};
    virtual const char *Version(void) { return VERSION; }
    virtual const char *Description(void) { return tr(DESCRIPTION); }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual void Housekeeping(void);
    virtual void MainThreadHook(void);
    virtual cString Active(void);
    virtual time_t WakeupTime(void);
    virtual const char *MainMenuEntry(void);
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    virtual bool Service(const char *Id, void *Data = NULL);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option,
            int &ReplyCode);
    static const std::string GetStillPicName(void) {
        const std::string cfdir = cPlugin::ConfigDirectory();
        return cfdir + "/" + mcfgDir + "/" + mStillPicture;
    }
    static const std::string GetDeviceName(void) {
        return mDevice;
    }
};

static inline const char *NotNull (const char *s) { return s ? s : ""; }
#endif
