#ifndef FLV2TS_FLV_TAG
#define FLV2TS_FLV_TAG

#include <inttypes.h>

namespace flv2ts {
  namespace flv {
    struct TagData {
    };

    struct Tag {
      bool     filter;
      uint8_t  type;
      uint32_t data_size;
      int32_t  timestamp; // timestamp + timestamp-extended. milli seconds
      uint32_t stream_id;
      TagData* data;

      enum TYPE {
        TYPE_AUDIO = 8,
        TYPE_VIDEO = 9,
        TYPE_SCRIPT_DATA = 18
      };
    };
  }
}

#endif
