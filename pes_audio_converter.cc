
#include <vdr/tools.h>
#include "pes_audio_converter.h"

// Initialize PES Header suitable for data from raw CD output

void cPesAudioConverter::SetData(const uint8_t *payload, int length)
{
    int len = LPCM_HEADER_LEN + PES_HEADER_EXT_LEN + length;
    // Initialize unused elements with 0
    memset(&m_pes_pcm_stream, 0x00, PES_MAX_PACKSIZE);
    // Check for length
    if (length > PES_MAX_PAYLOAD) {
        esyslog("%s %d: Oversized Packet len=%d", __FILE__, __LINE__, length);
        return;
    }
    // Setup PES Header
    m_pes_pcm_stream.startcode2 = 1;
    m_pes_pcm_stream.streamid = STREAM_ID_PRIVATE1;
    m_pes_pcm_stream.pes_packet_len_low = len & 0xFF;
    m_pes_pcm_stream.pes_packet_len_high = (len >> 8) & 0xFF;
    m_pes_pcm_stream.ext1 = 0x80;
    m_pes_pcm_stream.ext2 = 0;
    m_pes_pcm_stream.pes_header_data_len = 0;
    // Setup PCM Header
    m_pes_pcm_stream.sub_stream_id = SUBSTREAM_LPCM;
    m_pes_pcm_stream.number_of_frame_headers = 0xFF;
    m_pes_pcm_stream.sample = PCM_FREQ_44100 | PCM_CHAN2;
    m_pes_pcm_stream.dynamic_range_control = PES_DYNAMIC_RANGE_OFF;
    memcpy(m_pes_pcm_stream.payload, payload, length);
    m_peslen = length + PES_HEADER_LEN + LPCM_HEADER_LEN;
}

cPesAudioConverter::cPesAudioConverter() :
        m_peslen(0)
{
    memset (&m_pes_pcm_stream, 0, sizeof(PES_PCM_STREAM_T));
};
