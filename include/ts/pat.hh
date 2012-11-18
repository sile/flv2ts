#ifndef FLV2TS_TS_PAT_HH
#define FLV2TS_TS_PAT_HH

#include "payload.hh"
#include <inttypes.h>
#include <vector>

namespace flv2ts {
  namespace ts {
    struct PMT_MAP_ENTRY {
      uint16_t program_num;
      uint8_t  reserved:3; // always set to 7
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
      uint8_t current_next_indicator:1;
      uint8_t section_number;
      uint8_t last_section_number;
      PMT_MAP pmt_map;
      uint32_t crc32;

      ssize_t dump(char* buf, size_t buf_size) const {
        if(buf_size < static_cast<size_t>(4 + section_length)) {
          return -1;
        }
      
        buf[0] = pointer_field;
        buf[1] = table_id;

        buf[2] = (((section_syntax_indicator ? 1 : 0) << 7) +
                  (zero << 6) +
                  (reserved1 << 4) + 
                  (section_length >> 8));
        buf[3] = section_length & 0xFF;
      
        buf[4] = transport_stream_id >> 8;
        buf[5] = transport_stream_id & 0xFF;

        buf[6] = ((reserved2 << 6) +
                  (version_number << 1) +
                  (current_next_indicator));
      
        buf[7] = section_number;
        buf[8] = last_section_number;

        size_t i=9;

        for(size_t j=0; j < pmt_map.size(); j++) {
          const PMT_MAP_ENTRY& e = pmt_map[j];
        
          buf[i++] = e.program_num >> 8;
          buf[i++] = e.program_num & 0xFF;
        
          buf[i++] = ((e.reserved << 5) +
                      (e.program_pid >> 8));
          buf[i++] = e.program_pid & 0xFF;
        }
      
        buf[i++] = crc32 >> 24;
        buf[i++] = crc32 >> 16;
        buf[i++] = crc32 >> 8;
        buf[i++] = crc32 & 0xFF;
      
        return i;
      }
    };
  }
}

#endif
