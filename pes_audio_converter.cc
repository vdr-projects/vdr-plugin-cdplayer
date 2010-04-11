/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements a simple converter which converts the raw
 * data received from the CD-Rom to a PES stream.
 */

#include <vdr/tools.h>
#include "pes_audio_converter.h"

// Initialize PES Header suitable for data from raw CD output

void cPesAudioConverter::SetData(const uint8_t *payload, int length)
{
    int len = LPCM_HEADER_LEN + PES_HEADER_EXT_LEN + length;
    // Initialize unused elements with 0
    memset(&mPesPcmStream, 0x00, PES_MAX_PACKSIZE);
    // Check for length
    if (length > PES_MAX_PAYLOAD) {
        esyslog("%s %d: Oversized Packet len=%d", __FILE__, __LINE__, length);
        return;
    }
    // Setup PES Header
    mPesPcmStream.startcode2 = 1;
    mPesPcmStream.streamid = STREAM_ID_PRIVATE1;
    mPesPcmStream.pes_packet_len_low = len & 0xFF;
    mPesPcmStream.pes_packet_len_high = (len >> 8) & 0xFF;
    mPesPcmStream.ext1 = 0x80;
    mPesPcmStream.ext2 = 0;
    mPesPcmStream.pes_header_data_len = 0;
    // Setup PCM Header
    mPesPcmStream.sub_stream_id = SUBSTREAM_LPCM;
    mPesPcmStream.number_of_frame_headers = 0xFF;
    mPesPcmStream.sample = mFreq | PCM_CHAN2;
    mPesPcmStream.dynamic_range_control = PES_DYNAMIC_RANGE_OFF;
//    memcpy(m_pes_pcm_stream.payload, payload, length);
    mPeslen = length + PES_HEADER_LEN + LPCM_HEADER_LEN;

    // swap endianess
    const uint16_t *fromptr = (const uint16_t *)payload;
    uint16_t *toptr = (uint16_t *)mPesPcmStream.payload;
    uint8_t h,l;
    for (int n = 0; n < length; n += 2)
    {
        h = *fromptr >> 8;
        l = *fromptr & 0xFF;
        *toptr = h | (l << 8);
        fromptr++;
        toptr++;
    }
}

cPesAudioConverter::cPesAudioConverter() :
        mPeslen(0), mFreq(PCM_FREQ_44100)
{
    memset (&mPesPcmStream, 0, sizeof(PES_PCM_STREAM_T));
};
