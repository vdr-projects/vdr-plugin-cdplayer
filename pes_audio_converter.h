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


#ifndef _PES_AUDIO_CONVERTER_H
#define _PES_AUDIO_CONVERTER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const int PES_HEADER_LEN = 9;
const int PES_HEADER_EXT_LEN = 3;
const int LPCM_HEADER_LEN = 7;
const int PES_MAX_PACKSIZE = 4096;
const int PES_MAX_PAYLOAD = (PES_MAX_PACKSIZE - PES_HEADER_LEN - LPCM_HEADER_LEN);

const int PES_DYNAMIC_RANGE_OFF = 0x80;
const int SUBSTREAM_LPCM=0xA0;


const int STREAM_ID_PRIVATE1=0xBD;
const int STREAM_ID_AUDIO1 = 0xC0;

// Bits for ext1;
const int PES_EXT1=0x80;
const int PES_ORIGINAL=0x01;
const int PES_COPYRIGHT=0x02;
const int PES_DATA_ALIGNMENT_INDICATOR=0x04;

typedef enum _pcm_freq {
    PCM_FREQ_48000 = 0x00,
    PCM_FREQ_96000 = 0x10,
    PCM_FREQ_44100 = 0x20,
    PCM_FREQ_32000 = 0x30
} PCM_FREQ_T;

typedef enum _pcm_channel {
    PCM_CHAN1 = 0,
    PCM_CHAN2 = 1
} PCM_CHANNEL_T;


#pragma pack(push,1)
// PES Stream see http://dvd.sourceforge.net/dvdinfo/pes-hdr.html
typedef struct _pes_pcm_stream {
    uint8_t startcode0;  // 0
    uint8_t startcode1;  // 0
    uint8_t startcode2;  // 1
    uint8_t streamid;    // 0xBD Private Stream 1
    uint8_t pes_packet_len_high;
    uint8_t pes_packet_len_low;

    uint8_t ext1;
    uint8_t ext2;
    uint8_t pes_header_data_len; // PES_HEADER_LEN

    // PCM Stream header starts here

    // Byte 7
    uint8_t sub_stream_id;
    // Byte 6
    uint8_t number_of_frame_headers;
    // 5
//    uint16_t lpcm_header_len;
    uint8_t start_of_first_audio_frame_high;
    uint8_t start_of_first_audio_frame_low;
    // Byte 3
    // audio_emphasis :1;
    // audio_mute :1;
    // reserved :1;
    // audio_frame_number :5;
    uint8_t audio;
    // Byte 2
    // quantization_word_length :2;
    // sampling_frequency :2; // (48khz = 0, 96khz = 1)
    // reserved1 :1;
    // number_of_audio_channels : 3; // (e.g. stereo = 1 )
    uint8_t sample;
    // Byte 1
    uint8_t dynamic_range_control; // 0x80 if off
    uint8_t payload[PES_MAX_PACKSIZE - PES_HEADER_LEN - LPCM_HEADER_LEN];
} PES_PCM_STREAM_T;
#pragma pack(pop)

class cPesAudioConverter {
  private:
    PES_PCM_STREAM_T mPesPcmStream;
    int mPeslen;
    PCM_FREQ_T mFreq;
  public:
    // Default initialization suitable for CD
    cPesAudioConverter() ;
    void SetData(const uint8_t *payload, int length);
    void SetFreq(PCM_FREQ_T newfreq) { mFreq = newfreq; };
    int GetPesLength(void) { return mPeslen; };
    uint8_t *GetPesData(void) {return (uint8_t *)&mPesPcmStream; };

};

#endif
