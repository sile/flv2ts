#ifndef FLV2TS_TS_PACKET_HH
#define FLV2TS_TS_PACKET_HH

#include "header.hh"
#include "adaptation_field.hh"
#include "payload.hh"
#include "pat.hh"
#include "pmt.hh"
#include "pes.hh"
#include "data.hh"
#include <inttypes.h>

namespace flv2ts {
  namespace ts {
    struct Packet {
      const static unsigned SIZE = 188;
      const static uint8_t SYNC_BYTE = 0x47;
      
      Header header;
      AdaptationField adaptation_field;
      Payload* payload;

      //
      Packet() : payload(NULL) {}
      ~Packet() { delete payload; }

      // 
      enum PAYLOAD_TYPE {
        PAYLOAD_TYPE_PAT,
        PAYLOAD_TYPE_PMT,
        PAYLOAD_TYPE_PES,
        PAYLOAD_TYPE_DATA,
        PAYLOAD_TYPE_NULL,
        PAYLOAD_TYPE_UNKNOWN
      };
    };
  }
}

#endif
