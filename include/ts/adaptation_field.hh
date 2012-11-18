#ifndef FLV2TS_TS_ADAPTATION_FIELD_HH
#define FLV2TS_TS_ADAPTATION_FIELD_HH

#include <inttypes.h>

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
    };
  }
}

#endif
