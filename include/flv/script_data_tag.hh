#ifndef FLV2TS_FLV_SCRIPT_DATA_TAG_HH
#define FLV2TS_FLV_SCRIPT_DATA_TAG_HH

namespace flv2ts {
  namespace flv {
    struct ScriptDataTag {
      size_t payload_size;
      const uint8_t* amf0_payload;
    };
  }
}

#endif
