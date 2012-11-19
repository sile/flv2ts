#ifndef FLV2TS_FLV_VIDEO_TAG_HH
#define FLV2TS_FLV_VIDEO_TAG_HH

#include <inttypes.h>

namespace flv2ts {
  namespace flv {
    struct VideoTag {
      uint8_t frame_type:4;
      uint8_t codec_id:4;
      uint8_t avc_packet_type; // present if codec_id == 7
      int32_t composition_time:24; // present if codec_id == 7
      
      size_t payload_size;
      const uint8_t* payload;
      
      size_t headerSize() const {
        if(codec_id == 7) {
          return 5;
        } else {
          return 1;
        }
      }

      enum FRAME_TYE {
        FRAME_TYPE_KEY = 1,
        FRAME_TYPE_INTER = 2,
        FRAME_TYPE_DISPOSABLE_INTER = 3, // H.263 only
        FRAME_TYPE_GENERATED_KEY = 4, // reserved for server use only
        FRAME_TYPE_VIDEO_INFO_COMMAND = 5
      };
    };
  }
}

#endif
