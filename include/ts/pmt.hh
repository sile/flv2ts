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
      uint8_t table_id;              
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
    };
  }
}

#endif
