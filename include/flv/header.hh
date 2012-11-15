#ifndef FLV2TS_FLV_HEADER_HH
#define FLV2TS_FLV_HEADER_HH

#include <inttypes.h>

namespace flv2ts {
  namespace flv {
    struct Header {
      char     signature[3];
      uint8_t  version;
      uint8_t  is_audio;
      uint8_t  is_video;
      uint32_t data_offset;
    };
  }
}

#endif
