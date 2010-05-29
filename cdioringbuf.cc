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
    mBlocks = 0;
    mPutIdx = 0;
    mGetIdx = 0;
};

cCdIoRingBuffer::cCdIoRingBuffer(int blocks)
{
    mData = (uint8_t *)malloc(CDIO_CD_FRAMESIZE_RAW * blocks);
    if (mData == NULL) {
        esyslog ("%s %d Out of memory", __FILE__, __LINE__);
        exit(-1);
    }
    mPutIdx = 0;
    mGetIdx = 0;
    mBlocks = blocks;
    mGetAllowed.Deny();  // No data available, so lock GetBlock
    mPutAllowed.Allow(); // Allow PutBlock
}

cCdIoRingBuffer::~cCdIoRingBuffer()
{
    free(mData);
}

/*
 * Get a block from the ringbuffer, wait if
 * currently no data is avalable
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
    mBufferMutex.Unlock();
    return true;
}

/*
 * Put a block to the ringbuffer, wait if
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
    mBufferMutex.Unlock();
    return true;
}

/*
 * Clear and reset ringbuffer
 */

void cCdIoRingBuffer::Clear(void)
{
    mBufferMutex.Lock();
    mPutIdx = 0;
    mGetIdx = 0;
    mGetAllowed.Deny(); // No data available, so lock get call
    mPutAllowed.Allow();
    mBufferMutex.Unlock();
}
