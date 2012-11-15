#ifndef FLV2TS_FLV_TAG
#define FLV2TS_FLV_TAG

#include "audio_tag.hh"
#include "video_tag.hh"
#include "script_data_tag.hh"
#include <inttypes.h>

namespace flv2ts {
  namespace flv {
    struct Tag {
      bool     filter;
      uint8_t  type;
      uint32_t data_size;
      int32_t  timestamp; // timestamp + timestamp-extended. milli seconds
      uint32_t stream_id;

      union {
        AudioTag audio;
        VideoTag video;
        ScriptDataTag script_data;
      };

      enum TYPE {
        TYPE_AUDIO = 8,
        TYPE_VIDEO = 9,
        TYPE_SCRIPT_DATA = 18
      };
    };
  }
}

#endif
