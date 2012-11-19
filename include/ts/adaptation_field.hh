#ifndef FLV2TS_TS_ADAPTATION_FIELD_HH
#define FLV2TS_TS_ADAPTATION_FIELD_HH

#include <inttypes.h>
#include <cassert>

namespace flv2ts {
  namespace ts {
    struct AdaptationField {
      uint8_t adaptation_field_length;
      bool discontinuity_indicator;
      bool random_access_indicator;
      bool es_priority_indicator;
      bool pcr_flag;
      bool opcr_flag;
      bool splicing_point_flag;
      bool transport_private_data_flag;
      bool adaptation_field_extension_flag;
      uint64_t pcr:48;   // present if pcr_flag == true
      uint64_t opcr:48;  // present if opcr_flag == true
      int8_t splice_countdown; // present if splicing_point_flag == true
      const uint8_t* stuffing_bytes;

      ssize_t dump(char* buf, size_t buf_size) const {
        if(buf_size < static_cast<size_t>(1 + adaptation_field_length)) {
          return -1;
        }
        
        buf[0] = adaptation_field_length;
        buf[1] = (((discontinuity_indicator ? 1 : 0) << 7) +
                  ((random_access_indicator ? 1 : 0) << 6) +
                  ((es_priority_indicator   ? 1 : 0) << 5) +
                  ((pcr_flag                ? 1 : 0) << 4) +
                  ((opcr_flag               ? 1 : 0) << 3) +
                  ((splicing_point_flag     ? 1 : 0) << 2) +
                  ((transport_private_data_flag ? 1 : 0) << 1) +
                  ((adaptation_field_extension_flag ? 1 : 0) << 0));
        
        // unsupported
        assert(opcr_flag == false);
        assert(splicing_point_flag == false);
        assert(transport_private_data_flag == false);
        assert(adaptation_field_extension_flag == false);

        size_t i = 2;
        if(pcr_flag) {
          buf[i++] = pcr >> 32;
          buf[i++] = pcr >> 24;
          buf[i++] = pcr >> 16;
          buf[i++] = pcr >> 8;
          buf[i++] = pcr & 0xFF;
        }
        
        // stuffing bytes
        for(; i < static_cast<size_t>(adaptation_field_length + 1); i++) {
          buf[i] = 0xFF;
        }

        return i;
      }
    };
  }
}

#endif
