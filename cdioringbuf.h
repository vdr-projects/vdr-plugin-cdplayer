#ifndef __CDIORINGBUF_H__
#define __CDIORINGBUF_H__

#include <vdr/plugin.h>

class cAllowed : public cCondVar
{
private:
    bool mAllowed;
    cMutex mMutex;
public:
    cAllowed() : mAllowed(true) {};
    void Allow(void)
    {
        mMutex.Lock();
        mAllowed = true;
        Signal();
        mMutex.Unlock();
    }
    void Deny(void)
    {
        mMutex.Lock();
        mAllowed = false;
        Signal();
        mMutex.Unlock();
    }
    void WaitAllow(void)
    {
        while (!mAllowed) {
            Wait(0);
        }
    }
};

class cCdIoRingBuffer {
private:
    uint8_t *mData;
    int mBlocks;
    int mPutIdx;
    int mGetIdx;
    cMutex mBufferMutex;
    cAllowed mGetAllowed;
    cAllowed mPutAllowed;
public:
    cCdIoRingBuffer();
    cCdIoRingBuffer(int blocks);
    ~cCdIoRingBuffer();
    void GetBlock(uint8_t *block);
    void PutBlock(uint8_t *block);
    void Clear(void);
};

#endif
