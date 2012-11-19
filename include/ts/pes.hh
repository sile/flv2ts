#ifndef FLV2TS_TS_PES_HH
#define FLV2TS_TS_PES_HH

#include "payload.hh"
#include <inttypes.h>
#include <cassert>

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
      uint64_t pts:33;
      uint64_t dts:33;
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

      // dataは除く
      ssize_t dump(char* buf, size_t buf_size) const {
        if(buf_size < static_cast<size_t>(6 + 2 + optional_header.pes_header_length)) {
          return -1;
        }

        buf[0] = packet_start_prefix_code >> 16;
        buf[1] = packet_start_prefix_code >> 8;
        buf[2] = packet_start_prefix_code & 0xFF;
        buf[3] = stream_id;
        buf[4] = pes_packet_length >> 8;
        buf[5] = pes_packet_length & 0xFF;
        
        const OptionalPESHeader& h = optional_header;
        buf[6] = ((h.marker_bits << 6) +
                  (h.scrambling_control << 4) +
                  (h.priority << 3) +
                  ((h.data_alignment_indicator ? 1 : 0) << 2) +
                  ((h.copyright ? 1 : 0) << 1) +
                  ((h.original_or_copy ? 1 : 0)));
        buf[7] = (((h.pts_indicator ? 1 : 0) << 7) +
                  ((h.dts_indicator ? 1 : 0) << 6) +
                  ((h.escr_flag ? 1 : 0) << 5) +
                  ((h.es_rate_flag ? 1 : 0) << 4) +
                  ((h.dsm_trick_mode ? 1 : 0) << 3) +
                  ((h.additional_copy_info ? 1 : 0) << 2) +
                  ((h.crc_flag ? 1 : 0) << 1) +
                  ((h.extension_flag ? 1 : 0)));
        buf[8] = h.pes_header_length;

        // unsupported
        assert(h.data_alignment_indicator == false);
        assert(h.escr_flag == false);
        assert(h.es_rate_flag == false);
        assert(h.dsm_trick_mode == false);
        assert(h.additional_copy_info == false);
        assert(h.crc_flag == false);
        assert(h.extension_flag == false);
        
        size_t i=9;
        if(h.pts_indicator) {
          buf[i++] = ((h.dts_indicator ? 0x30 : 0x20) + // check-bits
                      ((h.pts >> 29) & 0x0E) +
                      0x01); // marker
          buf[i++] = h.pts >> 22;
          buf[i++] = (((h.pts >> 14) & 0xFFFE) +
                      0x01); // marker
          buf[i++] = h.pts >> 7;
          buf[i++] = ((h.pts << 1) +
                      0x01); // marker
        }
        if(h.dts_indicator) {
          buf[i++] = (0x10 + // check-bits
                      ((h.dts >> 29) & 0x0E) +
                      0x01); // marker
          buf[i++] = h.dts >> 22;
          buf[i++] = (((h.dts >> 14) & 0xFFFE) +
                      0x01); // marker
          buf[i++] = h.dts >> 7;
          buf[i++] = ((h.dts << 1) +
                      0x01); // marker
        }
        return i;
      }
    };
  }
}

#endif
