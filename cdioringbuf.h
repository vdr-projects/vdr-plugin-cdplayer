/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements a simple ringbuffer which stores blocks
 * of size CDIO_CD_FRAMESIZE_RAW for buffering the output of the
 * CD-Rom device
 */

#ifndef __CDIORINGBUF_H__
#define __CDIORINGBUF_H__

#include <vdr/plugin.h>
#include <cdio/cdio.h>

// Class to wait until access is allowed
class cAllowed : public cCondWait
{
private:
    volatile bool mAllowed;  // Access allowed ?
    cMutex mMutex;   // Mutex for waiting
public:

    cAllowed() : mAllowed(true) {};

    // Allow Access
    void Allow(void)
    {
        mMutex.Lock();
        mAllowed = true;
        Signal();
        mMutex.Unlock();
    }

    // Deny access
    void Deny(void)
    {
        mMutex.Lock();
        mAllowed = false;
        Signal();
        mMutex.Unlock();
    }

    // Wait until access is allowed or time out after 2 seconds (returns
    // false in this case)
    bool WaitAllow(void)
    {
        while (!mAllowed) {
            if (!Wait(2000)) {
                return false;
            }
        }
        return true;
    }
};

// Ringbuffer implementation

class cCdIoRingBuffer {
private:
    uint8_t *mData;
    volatile int mPutIdx;
    volatile int mGetIdx;
    int mBlocks;
    volatile int mNumBlocks;
    cMutex mBufferMutex;
    cAllowed mGetAllowed;
    cAllowed mPutAllowed;
    cCdIoRingBuffer();
public:
    cCdIoRingBuffer(int blocks);
    ~cCdIoRingBuffer();
    bool GetBlock(uint8_t *block);
    bool PutBlock(const uint8_t *block);
    void Clear(void);
    // Wait until number of blocks are available in the ring buffer.
    void WaitBlocksAvail (int numblocks);
    // Return average usage for debugging purposes
    int GetFreePercent(void) {
        return ((100*mNumBlocks)/mBlocks);
    }
};

#endif
