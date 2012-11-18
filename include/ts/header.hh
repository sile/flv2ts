#ifndef FLV2TS_TS_HEADER_HH
#define FLV2TS_TS_HEADER_HH

#include <inttypes.h>

namespace flv2ts {
  namespace ts {
    struct Header {
      bool transport_error_indicator;
      bool payload_unit_start_indicator;
      uint8_t transport_priority:1;
      uint16_t pid:13;
      uint8_t scrambling_control:2;
      uint8_t adaptation_field_exist:2;
      uint8_t continuity_counter:4;

      bool does_adaptation_field_exist() const {
        return adaptation_field_exist & 0x02;
      }
      bool does_payload_exist() const {
        return adaptation_field_exist & 0x01;
      }
    };
  }
}

#endif
