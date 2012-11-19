#ifndef FLV2TS_TS_PMT_HH
#define FLV2TS_TS_PMT_HH

#include "payload.hh"
#include <inttypes.h>
#include <vector>

namespace flv2ts {
  namespace ts {
    typedef std::vector<uint8_t> PROGRAM_DESCRIPTOR_LIST;
    
    typedef std::vector<uint8_t> ES_DESCRIPTOR_LIST;

    struct STREAM_INFO {
      uint8_t stream_type;
      uint8_t reserved1:3;
      uint16_t elementary_pid:13;
      uint8_t reserved2:4;
      uint16_t es_info_length:12;
      ES_DESCRIPTOR_LIST es_descriptor_list;
    };
    typedef std::vector<STREAM_INFO> STREAM_INFO_LIST;

    struct PMT : public Payload {
      uint8_t pointer_field; // present if header.payload_unit_start_indicator is true
      uint8_t table_id;      // always set to 2
      bool section_syntax_indicator; 
      uint8_t zero:1;                // always set to 0
      uint8_t reserved1:2;           // always set to 3
      uint16_t section_length:12;  
      uint16_t program_num;
      uint8_t reserved2:2;           
      uint8_t version_number:5;
      uint8_t current_next_indicator:1;
      uint8_t section_number;        // always set to 0
      uint8_t last_section_number;   // always set to 0
      uint8_t reserved3:3;
      uint16_t pcr_pid:13; 
      uint8_t reserved4:4;
      uint16_t program_info_length:12;
      PROGRAM_DESCRIPTOR_LIST program_descriptor_list;
      STREAM_INFO_LIST stream_info_list;
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
      
        buf[4] = program_num >> 8;
        buf[5] = program_num & 0xFF;

        buf[6] = ((reserved2 << 6) +
                  (version_number << 1) +
                  (current_next_indicator));
      
        buf[7] = section_number;
        buf[8] = last_section_number;
        
        buf[9] = ((reserved3 << 5) +
                  (pcr_pid >> 8));
        buf[10]= pcr_pid & 0xFF;

        buf[11]= ((reserved4 << 4) +
                  (program_info_length >> 8));
        buf[12]= program_info_length & 0xFF;
        
        size_t i = 13;
        for(size_t j=0; j < program_descriptor_list.size(); j++) {
          buf[i++] = program_descriptor_list[j];
        }

        for(size_t j=0; j < stream_info_list.size(); j++) {
          const STREAM_INFO& info = stream_info_list[j];
          buf[i++] = info.stream_type;
          buf[i++] = ((info.reserved1 << 5) +
                      (info.elementary_pid >> 8));
          buf[i++] = info.elementary_pid & 0xFF;
          buf[i++] = ((info.reserved2 << 4) +
                      (info.es_info_length >> 8));
          buf[i++] = info.es_info_length & 0xFF;
          for(size_t k=0; k < info.es_descriptor_list.size(); k++) {
            buf[i++] = info.es_descriptor_list[k];
          }
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
