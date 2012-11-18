#include <iostream>
#include <ts/parser.hh>
#include <ts/packet.hh>

using namespace flv2ts;

const char* bool2str(bool b) {
  return b ? "true" : "false";
}

int main(int argc, char** argv) {
  if(argc != 2) {
    std::cerr << "Usage: parse-ts TS_FILE" << std::endl;
    return 1;
  }

  const char* filepath = argv[1];
  ts::Parser parser(filepath);
  if(! parser) {
    std::cerr << "Can't open file: " << filepath << std::endl;
    return 1;
  }

  std::cout << "[file]" << std::endl
            << "  path: " << filepath << std::endl
            << std::endl;
  
  for(int i=0; ! parser.eos(); i++) {
    ts::Packet packet;
    bool ret = parser.parse(packet);
    
    std::cout << "[packet:" << i << "]" << std::endl;
    std::cout << "  [header]" << std::endl
              << "    transport_error_indicator:    " << bool2str(packet.header.transport_error_indicator) << std::endl
              << "    payload_unit_start_indicator: " << bool2str(packet.header.payload_unit_start_indicator) << std::endl
              << "    transport_priority:           " << (int)packet.header.transport_priority << std::endl
              << "    pid:                          " << packet.header.pid << std::endl
              << "    scrambling_control:           " << (int)packet.header.scrambling_control << std::endl
              << "    adaptation_field_exist:       " << (int)packet.header.adaptation_field_exist << std::endl
              << "    continuity_counter:           " << (int)packet.header.continuity_counter << std::endl;

    if(packet.header.does_adaptation_field_exist()) {
      const ts::AdaptationField& af = packet.adaptation_field;
      std::cout << "  [adaptation-field]" << std::endl
                << "    adaptation_field_length: " << (int)af.adaptation_field_length << std::endl
                << "    discontinuity_indicator: " << bool2str(af.discontinuity_indicator) << std::endl
                << "    random_access_indicator: " << bool2str(af.random_access_indicator) << std::endl
                << "    es_priority_indicator:   " << bool2str(af.es_priority_indicator) << std::endl
                << "    pcr_flag:                " << bool2str(af.pcr_flag) << std::endl
                << "    opcr_flag:               " << bool2str(af.opcr_flag) << std::endl
                << "    splicing_point_flag:     " << bool2str(af.splicing_point_flag) << std::endl
                << "    transport_private_data_flag:     " << bool2str(af.transport_private_data_flag) << std::endl
                << "    adaptation_field_extension_flag: " << bool2str(af.adaptation_field_extension_flag) << std::endl
                << "    pcr:              " << af.pcr << std::endl
                << "    opcr:             " << af.opcr << std::endl
                << "    splice_countdown: " << (int)af.splice_countdown << std::endl;
    }

    switch(parser.get_payload_type(packet)) {
    case ts::Packet::PAYLOAD_TYPE_PAT: {
      const ts::PAT& pat = *reinterpret_cast<ts::PAT*>(packet.payload);
      std::cout << "  [paylaod:PAT]" << std::endl
                << "    pointer_field:            " << (int)pat.pointer_field << std::endl
                << "    table_id:                 " << (int)pat.table_id << std::endl
                << "    section_syntax_indicator: " << bool2str(pat.section_syntax_indicator) << std::endl
                << "    section_length:           " << pat.section_length << std::endl
                << "    transport_stream_id:      " << pat.transport_stream_id << std::endl
                << "    version_number:           " << (int)pat.version_number << std::endl
                << "    current/next_indicator:   " << bool2str(pat.current_next_indicator) << std::endl
                << "    section_number:           " << (int)pat.section_number << std::endl
                << "    last_section_number:      " << (int)pat.last_section_number << std::endl;
      std::cout << "    [PMT_MAP]" << std::endl;
      for(size_t i=0; i < pat.pmt_map.size(); i++) {
        std::cout << "      " << i << ": " 
                  << "program_num=" << pat.pmt_map[i].program_num << ", "
                  << "program_pid=" << pat.pmt_map[i].program_pid << std::endl;
      }
      std::cout << "    crc32: " << pat.crc32 << std::endl;
      break;
      }
    case ts::Packet::PAYLOAD_TYPE_PMT: {
      const ts::PMT& pmt = *reinterpret_cast<ts::PMT*>(packet.payload); 
      std::cout << "  [paylaod:PMT]" << std::endl
                << "    pointer_field:            " << (int)pmt.pointer_field << std::endl
                << "    table_id:                 " << (int)pmt.table_id << std::endl
                << "    section_syntax_indicator: " << bool2str(pmt.section_syntax_indicator) << std::endl
                << "    section_length:           " << pmt.section_length << std::endl
                << "    program_num:              " << pmt.program_num << std::endl
                << "    version_number:           " << (int)pmt.version_number << std::endl
                << "    current/next_indicator:   " << bool2str(pmt.current_next_indicator) << std::endl
                << "    section_number:           " << (int)pmt.section_number << std::endl
                << "    last_section_number:      " << (int)pmt.last_section_number << std::endl
                << "    pcr_pid:                  " << (int)pmt.pcr_pid << std::endl
                << "    program_info_length:      " << pmt.program_info_length << std::endl
                << "    program_descriptor_list:  [";
      for(size_t i=0; i < pmt.program_descriptor_list.size(); i++) {
        if(i != 0) {
          std::cout << ", ";
        }
        std::cout << (int) pmt.program_descriptor_list[i];
      }
      std::cout << "]" << std::endl;

      if(pmt.stream_info_list.size() > 0) {
        std::cout << "    [STREAM_INFO_LIST]" << std::endl;
        for(size_t i=0; i < pmt.stream_info_list.size(); i++) {
          const ts::STREAM_INFO& info = pmt.stream_info_list[i];
          std::cout << "      [" << i << "]" << std::endl
                    << "        stream_type:    " << (int)info.stream_type << std::endl
                    << "        elementary_pid: " << info.elementary_pid << std::endl
                    << "        es_info_length: " << info.es_info_length << std::endl
                    << "        es_info_list:   [";
          for(size_t j=0; j < info.es_descriptor_list.size(); j++) {
            if(j != 0) {
              std::cout << ", ";
            }
            std::cout << (int)info.es_descriptor_list[j];
          }
          std::cout << "]" << std::endl;
        }
      }
      std::cout << "    crc32: " << pmt.crc32 << std::endl;
      
      break;
    }
    case ts::Packet::PAYLOAD_TYPE_PES: {
      const ts::PES& pes = *reinterpret_cast<ts::PES*>(packet.payload); 
      std::cout << "  [paylaod:PES]" << std::endl
                << "    packet_start_prefix_code: " << pes.packet_start_prefix_code << std::endl
                << "    stream_id:                " << (int)pes.stream_id << std::endl
                << "    pes_packet_length:        " << pes.pes_packet_length << std::endl;
      
      const ts::OptionalPESHeader& header = pes.optional_header;
      std::cout << "    [optional_header]" << std::endl
                << "      scrambling_control:       " << (int)header.scrambling_control << std::endl
                << "      priority:                 " << (int)header.priority << std::endl
                << "      data_alignment_indicator: " << bool2str(header.data_alignment_indicator) << std::endl
                << "      copyright:                " << bool2str(header.copyright) << std::endl
                << "      original_or_copy:         " << bool2str(header.original_or_copy) << std::endl
                << "      pts_indicator:            " << bool2str(header.pts_indicator) << std::endl
                << "      dts_indicator:            " << bool2str(header.dts_indicator) << std::endl
                << "      escr_flag:                " << bool2str(header.escr_flag) << std::endl
                << "      es_rate_flag:             " << bool2str(header.es_rate_flag) << std::endl
                << "      dsm_trick_mode_flag:      " << bool2str(header.dsm_trick_mode_flag) << std::endl
                << "      additional_copy_info_flag:" << bool2str(header.additional_copy_info_flag) << std::endl
                << "      crc_flag:                 " << bool2str(header.crc_flag) << std::endl
                << "      extension_flag:           " << bool2str(header.extension_flag) << std::endl
                << "      pes_header_length:        " << (int)header.pes_header_length << std::endl
                << "      pts:  " << header.pts << std::endl
                << "      dts:  " << header.dts << std::endl
                << "      escr: " << header.escr << std::endl
                << "      es:   " << header.es << std::endl
                << "      dsm_trick_mode:       " << (int)header.dsm_trick_mode << std::endl
                << "      additional_copy_info: " << (int)header.additional_copy_info << std::endl
                << "      pes_crc:              " << header.pes_crc << std::endl;
      std::cout << "    data_size: " << pes.data_size << std::endl;
      break;
    }
    case ts::Packet::PAYLOAD_TYPE_DATA: {
      const ts::Data& data = *reinterpret_cast<ts::Data*>(packet.payload);
      std::cout << "  [payload:DATA]" << std::endl
                << "    data_size: " << (int)data.data_size << std::endl;
      break;
    }
    case ts::Packet::PAYLOAD_TYPE_NULL:
      std::cout << "  [paylaod:NULL]" << std::endl;
      break;
    case ts::Packet::PAYLOAD_TYPE_UNKNOWN:
      std::cout << "  [paylaod:UNKNOWN]" << std::endl;
      break;
    default:
      return 1;
    }
    std::cout << std::endl;

    if(! ret) {
      std::cerr << "parse packet failed" << std::endl;
      return 1;
    }
  }

  return 0;
}
