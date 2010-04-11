
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
    mData = malloc(CDIO_CD_FRAMESIZE_RAW * blocks);
    if (mData == NULL) {
        esyslog ("%s %d Can not open %s", __FILE__, __LINE__, FileName.c_str());
        exit(-1);
    }
    mPutIdx = 0;
    mGetIdx = 0;
    mBlocks = blocks;
    mGetAllowed.Deny(); // No data available, so lock get call
    mPutAllowed.Allow(); // Allow Put
}

cCdIoRingBuffer::~cCdIoRingBuffer()
{
    free(mData);
}

void cCdIoRingBuffer::GetBlock(uint8_t *block)
{
    int idx = mGetIdx * CDIO_CD_FRAMESIZE_RAW;
    mGetAllowed.WaitAllow();
    mBufferMutex.Lock();
    (void)memcpy (block, &mData[idx], CDIO_CD_FRAMESIZE_RAW);
    mGetIdx++;
    if (mGetIdx > mBlocks) {
        mGetIdx = 0;
    }
    if (mGetIdx == mPutIdx) { // All data in buffer fetched
        mGetAllowed.Deny();
    }
    mPutAllowed.Allow();
    mBufferMutex.Unlock();
}

void cCdIoRingBuffer::PutBlock(uint8_t *block)
{
    int idx = mPutIdx * CDIO_CD_FRAMESIZE_RAW;
    mPutAllowed.WaitAllow();
    mBufferMutex.Lock();
    (void)memcpy (&mData[idx], block, CDIO_CD_FRAMESIZE_RAW);
    mPutIdx++;
    if (mPutIdx > mBlocks) {
        mPutIdx = 0;
    }
    if (mGetIdx == mPutIdx) { // Buffer is full
        mPutAllowed.Deny();
    }
    mGetAllowed.Allow();
    mBufferMutex.Unlock();
}

void cCdIoRingBuffer::Clear(void)
{
    mBufferMutex.Lock();
    mPutIdx = 0;
    mGetIdx = 0;
    mGetAllowed.Deny(); // No data available, so lock get call
    mPutAllowed.Allow();
    mBufferMutex.Unlock();
}
