#ifndef FLV2TS_H264_AVC_DECODER_CONFIGURATION_RECORD_HH
#define FLV2TS_H264_AVC_DECODER_CONFIGURATION_RECORD_HH

#include <inttypes.h>
#include <vector>
#include <string>

#include <aux/byte_stream.hh>

namespace flv2ts {
  namespace h264 {
    typedef std::vector<std::string> SequenceParameterSetBytesList;
    typedef std::vector<std::string> SequenceParameterSetExtBytesList;
    typedef std::vector<std::string> PictureParameterSetBytesList;
    
    // ISO/IEC 14496-15 (2010-06-01) 
    struct AVCDecoderConfigurationRecord {
      uint8_t configuration_version;  // always set to 1
      uint8_t avc_profile_indication;
      uint8_t profile_compatibility;
      uint8_t avc_level_indication;
      uint8_t reserved1:6; // alywas set to binary '111111'
      uint8_t length_size_minus_one:2;
      uint8_t reserved2:3; // always set to binary '111'

      uint8_t num_of_sequence_parameter_sets:5;
      SequenceParameterSetBytesList sequence_parameter_set_list;

      uint8_t num_of_picture_parameter_sets;
      PictureParameterSetBytesList picture_parameter_set_list;
      
      // below fields present if profile_idc == 100|110|122|144
      uint8_t reserved3:6; // always set to binary '111111'
      uint8_t chroma_format:2;
      uint8_t reserved4:5; // always set to binary '11111'
      uint8_t bit_depth_luma_minus8:3;
      uint8_t reserved5:5; // always set to binary '11111'
      uint8_t bit_depth_chroma_minus8:3;
      uint8_t num_of_sequence_parameter_set_ext;
      SequenceParameterSetExtBytesList sequence_parameter_set_ext_list;

      bool is_high_profile() const {
        const uint8_t profile_idc =  avc_profile_indication;
        return (profile_idc == 100 || profile_idc == 110 ||
                profile_idc == 122 || profile_idc == 144);
      }
    
      bool parse(aux::ByteStream& in) {
        if(! in.can_read(6)) {
          return false;
        }
        
        configuration_version = in.readUint8();
        if(configuration_version != 1) {
          std::cerr << (int)configuration_version << std::endl;
          return false;
        }

        avc_profile_indication = in.readUint8();
        profile_compatibility = in.readUint8();
        avc_level_indication = in.readUint8();

        const uint8_t tmp1 = in.readUint8();
        reserved1 = tmp1 >> 2;
        length_size_minus_one = tmp1 & 3;
        if(reserved1 != 0x3F) {
          return false;
        }

        const uint8_t tmp2 = in.readUint8();
        reserved2 = tmp2 >> 5;
        num_of_sequence_parameter_sets = tmp2 & 0x1F;
        if(reserved2 != 7) {
          return false;
        }

        for(uint8_t i=0; i < num_of_sequence_parameter_sets; i++) {
          if(! in.can_read(2)) {
            return false;
          }
          const uint16_t length = in.readUint16Be();
          if(! in.can_read(length)) {
            return false;
          }
          std::string buf;
          
          buf.assign(reinterpret_cast<const char*>(in.read(length)), length);
          sequence_parameter_set_list.push_back(buf);
        }
        
        if(! in.can_read(1)) {
          return false;
        }
        num_of_picture_parameter_sets = in.readUint8();

        for(uint8_t i=0; i < num_of_picture_parameter_sets; i++) {
          if(! in.can_read(2)) {
            return false;
          }
          const uint16_t length = in.readUint16Be();
          if(! in.can_read(length)) {
            return false;
          }
          std::string buf;
          
          buf.assign(reinterpret_cast<const char*>(in.read(length)), length);
          picture_parameter_set_list.push_back(buf);
        }
        
        if(is_high_profile()) {
          if(! in.can_read(4)) {
            return false;
          }

          const uint8_t tmp3 = in.readUint8();
          reserved3 = tmp3 >> 2;
          chroma_format = tmp3 & 3;
          if(reserved3 != 0x3F) {
            return false;
          }

          const uint8_t tmp4 = in.readUint8();
          reserved4 = tmp4 >> 3;
          bit_depth_luma_minus8 = tmp4 & 7;
          if(reserved4 != 0x1F) {
            return false;
          }

          const uint8_t tmp5 = in.readUint8();
          reserved5 = tmp5 >> 3;
          bit_depth_chroma_minus8 = tmp4 & 7;
          if(reserved5 != 0x1F) {
            return false;
          }

          num_of_sequence_parameter_set_ext = in.readUint8();
          for(uint8_t i=0; i < num_of_sequence_parameter_set_ext; i++) {
            if(! in.can_read(2)) {
              return false;
            }
            const uint16_t length = in.readUint16Be();
            if(! in.can_read(length)) {
              return false;
            }
            std::string buf;
            
            buf.assign(reinterpret_cast<const char*>(in.read(length)), length);
            sequence_parameter_set_ext_list.push_back(buf);
          }
        }

        return true;
      }
    };
  }
}

#endif
