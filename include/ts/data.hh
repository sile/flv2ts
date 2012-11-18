#ifndef FLV2TS_TS_DATA_HH
#define FLV2TS_TS_DATA_HH

#include "payload.hh"
#include <inttypes.h>

namespace flv2ts {
  namespace ts {
    struct Data : public Payload {
      uint8_t  data_size;
      const uint8_t* data;
    };
  }
}

#endif
