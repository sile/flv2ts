#ifndef FLV2TS_H264_AVC_SAMPLE_HH
#define FLV2TS_H264_AVC_SAMPLE_HH

#include "avc_decoder_configuration_record.hh"
#include <aux/byte_stream.hh>
#include <inttypes.h>

namespace flv2ts {
  namespace h264 {
    struct AVCSample {
      uint32_t nal_unit_length;
      const uint8_t* nal_unit;

      bool parse(aux::ByteStream& in, const AVCDecoderConfigurationRecord& conf) {
        if(conf.length_size_minus_one >= 4) {
          return false; // 仕様ではなく実装的に無理
        }

        if(! in.can_read(conf.length_size_minus_one + 1)) {
          return false;
        }

        nal_unit_length = 0;
        for(uint8_t i=0; i <= conf.length_size_minus_one; i++) {
          nal_unit_length = (nal_unit_length << 8) + static_cast<uint32_t>(in.readUint8());
        }
        
        if(! in.can_read(nal_unit_length)) {
          return false;
        }
        
        nal_unit = in.read(nal_unit_length);
        return true;
      }
    };
  }
}

#endif
