/*
 * Plugin for VDR to act as CD-Player
 *
 * Copyright (C) 2010-2012 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This module implements the service for the Spectrum Analyzer plugin
 */

#ifndef _SERVICE_H
#define _SERVICE_H


#define SPAN_PROVIDER_CHECK_ID  "Span-ProviderCheck-v1.0"
#define SPAN_CLIENT_CHECK_ID    "Span-ClientCheck-v1.0"
#define SPAN_SET_PCM_DATA_ID    "Span-SetPcmData-v1.1"
#define SPAN_SET_PLAYINDEX_ID   "Span-SetPlayindex-v1.0"
#define SPAN_GET_BAR_HEIGHTS_ID "Span-GetBarHeights-v1.0"

//Span requests to collect possible providers / clients
struct Span_Provider_Check_1_0 {
    bool *isActive;
    bool *isRunning;
};

struct Span_Client_Check_1_0 {
    bool *isActive;
    bool *isRunning;
};

// SpanData
struct Span_SetPcmData_1_0 {
    unsigned int length;        // the length of the PCM-data
    const unsigned char *data;  // the PCM-Data
    int index;                  // the timestamp (ms) of the frame(s) to be visualized
    unsigned int bufferSize;    // for span-internal bookkeeping of the data to be visualized
    bool bigEndian;             // are the pcm16-data coded bigEndian?
};

struct Span_SetPlayindex_1_0 {
    int index;                  // the timestamp (ms) of the frame(s) being currently played
};

#endif
