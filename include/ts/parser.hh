#ifndef FLV2TS_TS_PARSER_HH
#define FLV2TS_TS_PARSER_HH

#include "packet.hh"
#include <aux/file_mapped_memory.hh>
#include <aux/byte_stream.hh>
#include <inttypes.h>
#include <algorithm>
#include <cassert> // XXX:

namespace flv2ts {
  namespace ts {
    class Parser {
    public:
      Parser(const char* filepath) 
        : _fmm(filepath),
          _in(_fmm.ptr<const uint8_t>(), _fmm.size())
      {
      }

      operator bool() const { return _fmm && _in; }

      bool parse(Packet& packet) {
        const size_t start_pos = _in.position();

        if(! _in.can_read(Packet::SIZE)) {
          return false;
        }
        
        const uint8_t sync_byte = _in.readUint8();
        if(sync_byte != Packet::SYNC_BYTE) {
          return false;
        }

        if(! parseHeader(packet.header)) {
          return false;
        }

        if(packet.header.does_adaptation_field_exist()) {
          if(! parseAdaptationField(packet.adaptation_field)) {
            return false;
          }
        }

        if(packet.header.does_payload_exist()) {
          if(! parsePayload(start_pos, packet, packet.payload)) {
            return false;
          }
        }

        if(_in.position() > start_pos + Packet::SIZE) {
          return false;
        }

        _in.abs_seek(start_pos + Packet::SIZE);
        return true;
      }

      bool abs_seek(size_t pos) { return _in.abs_seek(pos); }
      bool rel_seek(ssize_t offset) { return _in.rel_seek(offset); }
      size_t position() const { return _in.position(); }
      bool eos() const { return _in.eos(); }

      Packet::PAYLOAD_TYPE get_payload_type(const Packet& packet) const {
        if(! packet.header.payload_unit_start_indicator) {
          return Packet::PAYLOAD_TYPE_DATA;
        }

        switch (packet.header.pid) {
        case 0x0000: return Packet::PAYLOAD_TYPE_PAT;
        case 0x1FFF: return Packet::PAYLOAD_TYPE_NULL;
        default: 
          for(size_t i=0; i < pmt_map.size(); i++) {
            if(packet.header.pid == pmt_map[i].program_pid) {
              return Packet::PAYLOAD_TYPE_PMT;
            }
          }
          
          for(size_t i=0; i < stream_info_list.size(); i++) {
            if(packet.header.pid == stream_info_list[i].elementary_pid) {
              return Packet::PAYLOAD_TYPE_PES;
            }
          }
          
          return Packet::PAYLOAD_TYPE_UNKNOWN;
        }
      }

      bool is_audio_packet(const Packet& packet) const {
        Packet::PAYLOAD_TYPE type = get_payload_type(packet);
        if(type != Packet::PAYLOAD_TYPE_PES && type != Packet::PAYLOAD_TYPE_DATA) {
          return false;
        }

        for(size_t i=0; i < stream_info_list.size(); i++) {
          if(packet.header.pid == stream_info_list[i].elementary_pid) {
            return stream_info_list[i].stream_type == 15;
          }
        }
        
        return false;
      }

      bool is_video_packet(const Packet& packet) const {
        Packet::PAYLOAD_TYPE type = get_payload_type(packet);
        if(type != Packet::PAYLOAD_TYPE_PES && type != Packet::PAYLOAD_TYPE_DATA) {
          return false;
        }
        
        for(size_t i=0; i < stream_info_list.size(); i++) {
          if(packet.header.pid == stream_info_list[i].elementary_pid) {
            return stream_info_list[i].stream_type == 27;
          }
        }
        
        return false;
      }

    private:
      bool parseHeader(Header& header) {
        const uint16_t tmp1 = _in.readUint16Be();
        header.transport_error_indicator    = tmp1 & 0x8000;
        header.payload_unit_start_indicator = tmp1 & 0x4000;
        header.transport_priority           = (tmp1 & 0x2000) ? 1 : 0;
        header.pid                          = tmp1 & 0x1FFF;

        const uint8_t tmp2 = _in.readUint8();
        header.scrambling_control     = (tmp2 & 0xC0) >> 6;
        header.adaptation_field_exist = (tmp2 & 0x30) >> 4;
        header.continuity_counter     = (tmp2 & 0x0F);

        return true;
      }

      bool parseAdaptationField(AdaptationField& field) {
        memset(&field, 0, sizeof(AdaptationField));
        field.adaptation_field_length = _in.readUint8();
        const size_t start_pos = _in.position();
        
        const uint8_t tmp1 = _in.readUint8();
        field.discontinuity_indicator = tmp1 & 0x80;
        field.random_access_indicator = tmp1 & 0x40;
        field.es_priority_indicator   = tmp1 & 0x20;
        field.pcr_flag                = tmp1 & 0x10;
        field.opcr_flag               = tmp1 & 0x08;
        field.splicing_point_flag     = tmp1 & 0x04;
        field.transport_private_data_flag     = tmp1 & 0x02;
        field.adaptation_field_extension_flag = tmp1 & 0x01;
        
        if(field.pcr_flag) {
          field.pcr = (_in.readUint16Be() << 16) + _in.readUint32Be();
        }
        if(field.opcr_flag) {
          field.opcr = (_in.readUint16Be() << 16) + _in.readUint32Be();
        }
        if(field.splicing_point_flag) {
          field.splice_countdown = _in.readUint8();
        }
        
        field.stuffing_bytes = _in.read(field.adaptation_field_length - (_in.position() - start_pos));
                                        
        return true;
      }

      bool parsePayload(const size_t start_pos, const Packet& packet, Payload* & payload) {
        switch(get_payload_type(packet)) {
        case Packet::PAYLOAD_TYPE_PAT: {
          payload = new PAT;
          return parsePayloadPAT(packet, *reinterpret_cast<PAT*>(payload));
        }
        case Packet::PAYLOAD_TYPE_PMT: {
          payload = new PMT;
          return parsePayloadPMT(packet, *reinterpret_cast<PMT*>(payload));
        }
        case Packet::PAYLOAD_TYPE_PES: {
          payload = new PES;
          return parsePayloadPES(start_pos, packet, *reinterpret_cast<PES*>(payload));
        }
        case Packet::PAYLOAD_TYPE_DATA: {
          payload = new Data;
          return parsePayloadData(start_pos, packet, *reinterpret_cast<Data*>(payload));
        }
        case Packet::PAYLOAD_TYPE_NULL:
          return true;
        case Packet::PAYLOAD_TYPE_UNKNOWN:
          return true;
          
        default:
          assert(false);
          return true;
        }
      }

      bool parsePayloadData(const size_t start_pos, const Packet& packet, Data& data) {
        data.data_size = (start_pos + Packet::SIZE) - _in.position();
        data.data = _in.read(data.data_size);
        return true;
      }

      bool parsePayloadPAT(const Packet& packet, PAT& pat) {
        if(packet.header.payload_unit_start_indicator) {
          pat.pointer_field = _in.readUint8();
        } else {
          pat.pointer_field = 0;
        }
        
        pat.table_id = _in.readUint8();
        if(pat.table_id != 0) {
          return false;
        }

        const uint16_t tmp1 = _in.readUint16Be();
        pat.section_syntax_indicator = (tmp1 & 0x8000);
        pat.zero                     = (tmp1 & 0x4000) ? 1 : 0;
        pat.reserved1                = (tmp1 & 0x3000) >> 12;
        pat.section_length           = (tmp1 & 0x0FFF);
        if(pat.section_syntax_indicator == false ||
           pat.zero == 1 ||
           pat.reserved1 != 3) {
          return false;
        }

        pat.transport_stream_id = _in.readUint16Be();
        
        const uint8_t tmp2 = _in.readUint8();
        pat.reserved2              = (tmp2 & 0xC0) >> 6;
        pat.version_number         = (tmp2 & 0x3E) >> 1;
        pat.current_next_indicator = (tmp2 & 0x01);
        if(pat.reserved2 != 3) {
          return false;
        }
        
        pat.section_number = _in.readUint8();
        pat.last_section_number = _in.readUint8();

        const int pmt_map_entry_count = (pat.section_length - 9) / ((16+3+13)/8);
        for(int i=0; i < pmt_map_entry_count; i++) {
          PMT_MAP_ENTRY entry;
          entry.program_num = _in.readUint16Be();
          
          const uint16_t tmp3 = _in.readUint16Be();
          entry.reserved    = (tmp3 & 0xE000) >> 13;
          entry.program_pid = (tmp3 & 0x1FFF);
          if(entry.reserved != 7) {
            return false;
          }

          pat.pmt_map.push_back(entry);
        }
        pmt_map = pat.pmt_map;
        
        pat.crc32 = _in.readUint32Be(); // TODO: verify CRC
        return true;
      }

      bool parsePayloadPMT(const Packet& packet, PMT& pmt) {
        if(packet.header.payload_unit_start_indicator) {
          pmt.pointer_field = _in.readUint8();
        } else {
          pmt.pointer_field = 0;
        }
        
        pmt.table_id = _in.readUint8();
        if(pmt.table_id != 2) {
          return false;
        }

        const uint16_t tmp1 = _in.readUint16Be();
        pmt.section_syntax_indicator = (tmp1 & 0x8000);
        pmt.zero                     = (tmp1 & 0x4000) ? 1 : 0;
        pmt.reserved1                = (tmp1 & 0x3000) >> 12;
        pmt.section_length           = (tmp1 & 0x0FFF);
        if(pmt.zero == 1 ||
           pmt.reserved1 != 3) {
          return false;
        }
        const size_t section_end_pos = _in.position() + pmt.section_length;

        pmt.program_num = _in.readUint16Be();
        
        const uint8_t tmp2 = _in.readUint8();
        pmt.reserved2              = (tmp2 & 0xC0) >> 6;
        pmt.version_number         = (tmp2 & 0x3E) >> 1;
        pmt.current_next_indicator = (tmp2 & 0x01);
        
        pmt.section_number = _in.readUint8();
        pmt.last_section_number = _in.readUint8();
        if(pmt.section_number != 0 ||
           pmt.last_section_number != 0) {
          return false;
        }

        const uint16_t tmp3 = _in.readUint16Be();
        pmt.reserved3 = (tmp3 & 0xE000) >> 13;
        pmt.pcr_pid   = (tmp3 & 0x1FFF);
        
        const uint16_t tmp4 = _in.readUint16Be();
        pmt.reserved4           = (tmp4 & 0xF000) >> 12;
        pmt.program_info_length = (tmp4 & 0x0FFF);
        
        const int program_descriptor_count = pmt.program_info_length / 8;
        for(int i=0; i < program_descriptor_count; i++) {
          pmt.program_descriptor_list.push_back(_in.readUint8());
        }
        
        while(_in.position() < section_end_pos - 4) {
          STREAM_INFO info;
          info.stream_type = _in.readUint8();
          
          const uint16_t tmp5 = _in.readUint16Be();
          info.reserved1      = (tmp5 & 0xE000) >> 13;
          info.elementary_pid = (tmp5 & 0x1FFF);
          if(info.reserved1 != 7) {
            return false;
          }
          
          const uint16_t tmp6 = _in.readUint16Be();
          info.reserved2      = (tmp6 & 0xF000) >> 12;
          info.es_info_length = (tmp6 & 0x0FFF);

          for(int i=0; i < info.es_info_length; i++) {
            info.es_descriptor_list.push_back(_in.readUint8());
          }
          pmt.stream_info_list.push_back(info);
        }
        stream_info_list = pmt.stream_info_list;

        pmt.crc32 = _in.readUint32Be(); // TODO: verify CRC
        return true;
      }

      bool parsePayloadPES(const size_t start_pos, const Packet& packet, PES& pes) {
        // memset(&pes, 0, sizeof(PES));
        pes.packet_start_prefix_code = _in.readUint24Be();
        if(pes.packet_start_prefix_code != 0x000001) {
          return false;
        }

        pes.stream_id = _in.readUint8();
        pes.pes_packet_length = _in.readUint16Be();
        
        // TODO: 分岐
        OptionalPESHeader& header = pes.optional_header;

        const uint8_t tmp1 = _in.readUint8();
        header.marker_bits              = (tmp1 & 0xC0) >> 6;
        header.scrambling_control       = (tmp1 & 0x30) >> 4;
        header.priority                 = (tmp1 & 0x08) >> 3;
        header.data_alignment_indicator = (tmp1 & 0x04);
        header.copyright                = (tmp1 & 0x02);
        header.original_or_copy         = (tmp1 & 0x01);
        
        const uint8_t tmp2 = _in.readUint8();
        header.pts_indicator = (tmp2 & 0x80);
        header.dts_indicator = (tmp2 & 0x40);
        header.escr_flag     = (tmp2 & 0x20);
        header.es_rate_flag  = (tmp2 & 0x10);
        header.dsm_trick_mode_flag       = (tmp2 & 0x08);
        header.additional_copy_info_flag = (tmp2 & 0x04);
        header.crc_flag      = (tmp2 & 0x02);
        header.extension_flag= (tmp2 & 0x01);
        header.pes_header_length = _in.readUint8();

        const size_t header_end = _in.position() + header.pes_header_length;
        
        if(header.pts_indicator) {
          header.pts = readPtsDts(header.dts_indicator ? 3 : 2);
        }
        if(header.dts_indicator) {
          header.dts = readPtsDts(1);
        }

        if(header.escr_flag) {
          header.escr = (_in.readUint16Be() << 16) + _in.readUint32Be();
        }
        if(header.es_rate_flag) {
          header.es = _in.readUint24Be();
        }
        if(header.dsm_trick_mode_flag) {
          header.dsm_trick_mode_flag = _in.readUint8();
        }
        if(header.additional_copy_info_flag) {
          header.additional_copy_info = _in.readUint8();
        }
        if(header.crc_flag) {
          header.pes_crc = _in.readUint16Be();
        }
        
        // TODO: 他のオプショナルなフィールドのパース
        
        _in.abs_seek(header_end);

        // data
        pes.data_size = (start_pos + Packet::SIZE) - _in.position();
        pes.data = _in.read(pes.data_size);
        
        return true;
      }
      
      uint64_t readPtsDts(uint8_t expect_check_bits) {
        uint8_t b1 = _in.readUint8();
        uint8_t b2 = _in.readUint16Be();
        uint8_t b3 = _in.readUint16Be();
        
        uint8_t  check_bits = (b1 & 0xF0) >> 4;
        uint8_t  v32_30     = (b1 & 0x0E) >> 1;
        uint8_t  marker1    = (b1 & 0x01);
        uint16_t v29_15     = (b2 >> 1);
        uint8_t  marker2    = (b2 & 0x0001);
        uint16_t v14_0      = (b3 >> 1);
        uint8_t  marker3    = (b3 & 0x0001);
        
        assert(check_bits == expect_check_bits && marker1 == 1 && marker2 == 1 && marker3 == 1);

        return (v32_30 << 30) + (v29_15 << 15) + v14_0;
      }

    private:
      aux::FileMappedMemory _fmm;
      aux::ByteStream _in;

      PMT_MAP pmt_map;
      STREAM_INFO_LIST stream_info_list;
    };
  }
}

#endif
