#ifndef FLV2TS_TS_PES_HH
#define FLV2TS_TS_PES_HH

#include "payload.hh"
#include <inttypes.h>

namespace flv2ts {
  namespace ts {
    struct OptionalPESHeader {
      uint8_t marker_bits:2;  // always set to 2
      uint8_t scrambling_control:2;
      uint8_t priority:1;
      bool data_alignment_indicator;
      bool copyright;
      bool original_or_copy;
      bool pts_indicator;
      bool dts_indicator;
      bool escr_flag;
      bool es_rate_flag;
      bool dsm_trick_mode_flag;
      bool additional_copy_info_flag;
      bool crc_flag;
      bool extension_flag;
      uint8_t pes_header_length;

      // optional
      uint32_t pts;
      uint32_t dts;
      uint64_t escr:48;
      uint32_t es:24;
      uint8_t  dsm_trick_mode;
      uint8_t  additional_copy_info;
      uint16_t pes_crc;
    };

    struct PES : public Payload {
      uint32_t packet_start_prefix_code:24; // always set to 1
      uint8_t stream_id;
      uint16_t pes_packet_length;  // can be zero if PES packet paylaod is a video elementary stream
      OptionalPESHeader optional_header; //  not present in case of Padding stream & Private stream 2 (navigation data)

      size_t data_size;
      const uint8_t *data;
    };
  }
}

#endif
