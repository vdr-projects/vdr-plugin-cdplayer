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

#include <string.h>
#include "cdioringbuf.h"

cCdIoRingBuffer::cCdIoRingBuffer()
{
    mData = NULL;
    Clear();
};

cCdIoRingBuffer::cCdIoRingBuffer(int blocks)
{
    mData = (uint8_t *)malloc(CDIO_CD_FRAMESIZE_RAW * blocks);
    if (mData == NULL) {
        esyslog ("%s %d Out of memory", __FILE__, __LINE__);
        exit(-1);
    }
    mBlocks = blocks;
    Clear();
}

cCdIoRingBuffer::~cCdIoRingBuffer()
{
    free(mData);
}

/*
 * Wait until at least numblocks blocks are available in the ringbuffer
 */
void cCdIoRingBuffer::WaitBlocksAvail (int numblocks)
{
    while (mNumBlocks < numblocks) {
        cCondWait::SleepMs(250);
    }
}
/*
 * Get a block from the ring buffer, wait if
 * currently no data is available
 */

bool cCdIoRingBuffer::GetBlock(uint8_t *block)
{
    int idx = mGetIdx * CDIO_CD_FRAMESIZE_RAW;

    if (!mGetAllowed.WaitAllow()) {
        return false;
    }
    mBufferMutex.Lock();
    memcpy (block, &mData[idx], CDIO_CD_FRAMESIZE_RAW);
    mGetIdx++;
    if (mGetIdx >= mBlocks) {
        mGetIdx = 0;
    }
    if (mGetIdx == mPutIdx) { // All data in buffer fetched
        mGetAllowed.Deny();
    }
    mPutAllowed.Allow();
    mNumBlocks--;
    mBufferMutex.Unlock();
    return true;
}

/*
 * Put a block to the ring buffer, wait if
 * no space is left on the buffer
 */

bool cCdIoRingBuffer::PutBlock(const uint8_t *block)
{
    int idx = mPutIdx * CDIO_CD_FRAMESIZE_RAW;

    if (!mPutAllowed.WaitAllow()) {
        return false;
    }
    mBufferMutex.Lock();
    memcpy (&mData[idx], block, CDIO_CD_FRAMESIZE_RAW);
    mPutIdx++;
    if (mPutIdx >= mBlocks) {
        mPutIdx = 0;
    }
    if (mGetIdx == mPutIdx) { // Buffer is full
        mPutAllowed.Deny();
    }
    mGetAllowed.Allow();
    mNumBlocks++;
    mBufferMutex.Unlock();
    return true;
}

/*
 * Clear and reset ringbuffer
 */

void cCdIoRingBuffer::Clear(void)
{
    mBufferMutex.Lock();
    mNumBlocks = 0;
    mPutIdx = 0;
    mGetIdx = 0;
    mGetAllowed.Deny(); // No data available, so lock get call
    mPutAllowed.Allow();
    mBufferMutex.Unlock();
}
