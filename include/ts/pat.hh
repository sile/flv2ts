#ifndef FLV2TS_TS_PAT_HH
#define FLV2TS_TS_PAT_HH

#include "payload.hh"
#include <inttypes.h>
#include <vector>

namespace flv2ts {
  namespace ts {
    struct PMT_MAP_ENTRY {
      uint16_t program_num;
      uint8_t  reserved:3; // always set to 3
      uint16_t program_pid:13;
    };
    typedef std::vector<PMT_MAP_ENTRY> PMT_MAP;

    struct PAT : public Payload {
      uint8_t pointer_field; // present if header.payload_unit_start_indicator is true
      uint8_t table_id;              // always set to 0
      bool section_syntax_indicator; // always set to true
      uint8_t zero:1;                // always set to 0
      uint8_t reserved1:2;           // always set to 3
      uint16_t section_length:12;  
      uint16_t transport_stream_id;
      uint8_t reserved2:2;           // always set to 3
      uint8_t version_number:5;
      bool current_next_indicator;
      uint8_t section_number;
      uint8_t last_section_number;
      PMT_MAP pmt_map;
      uint32_t crc32;
    };
  }
}

#endif
