
#ifndef _CD_CONTROL_H
#define _CD_CONTROL_H

#include <vdr/player.h>

// The maximum size of a single frame (up to HDTV 1920x1080):
#define TS_SIZE 188
#define MAXFRAMESIZE  (KILOBYTE(1024) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE to avoid breaking up TS packets

class cCdPlayer: public cPlayer, public cThread {
protected:
    uchar *pStillBuf;
    int mStillBufLen;

    virtual void Activate(bool On);
    void Action(void);

public:
    cCdPlayer(void);
    virtual ~cCdPlayer();
    void LoadStillPicture (char *FileName);
    void DisplayStillPicture (void);
};

class cCdControl: public cControl {
private:
    cCdPlayer *player;
public:
    cCdControl(void);
    virtual ~cCdControl() { };
    virtual void Hide(void);
    virtual cOsdObject *GetInfo(void);
    virtual eOSState ProcessKey(eKeys Key);
};

#endif
