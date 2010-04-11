
#ifndef _CD_CONTROL_H
#define _CD_CONTROL_H

#include <vdr/player.h>
#include "bufferedcdio.h"
#include "pes_audio_converter.h"

// The maximum size of a single frame (up to HDTV 1920x1080):
#define TS_SIZE 188
#define MAXFRAMESIZE  (KILOBYTE(1024) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE to avoid breaking up TS packets
#define MAX_SPEED 2

class cCdPlayer: public cPlayer, public cThread {
protected:
    cBufferedCdio cdio;
    uchar *pStillBuf;
    int mStillBufLen;
    int mSpeed;
    static const PCM_FREQ_T mSpeedTypes[MAX_SPEED+1];
    virtual void Activate(bool On);
    void Action(void);

public:
    cCdPlayer(void);
    virtual ~cCdPlayer();
    void LoadStillPicture (char *FileName);
    void DisplayStillPicture (void);
    void NextTrack(void) {cdio.NextTrack();};
    void PrevTrack(void) {cdio.PrevTrack();};
    void Stop(void) {cdio.Stop();};
    void SpeedNormal(void) {mSpeed = 0;};
    void SpeedFaster(void) {if (mSpeed < MAX_SPEED) mSpeed++;};
    void SpeedSlower(void) {if (mSpeed > 0) mSpeed--;};
};

class cCdControl: public cControl {
private:
    cCdPlayer *mCdPlayer;
public:
    cCdControl(void);
    virtual ~cCdControl() { };
    virtual void Hide(void);
    virtual cOsdObject *GetInfo(void);
    virtual eOSState ProcessKey(eKeys Key);
};

#endif
