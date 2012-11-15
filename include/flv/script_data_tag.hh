#ifndef FLV2TS_FLV_SCRIPT_DATA_TAG_HH
#define FLV2TS_FLV_SCRIPT_DATA_TAG_HH

#include "tag.hh"

namespace flv2ts {
  namespace flv {
    struct ScriptDataTag : public TagData {
      uint8_t amf0_payload[0];
    };
  }
}

#endif
