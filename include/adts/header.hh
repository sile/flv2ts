#ifndef FLV2TS_ADTS_HEADER_HH
#define FLV2TS_ADTS_HEADER_HH

#include <inttypes.h>

namespace flv2ts {
  namespace adts {
    struct Header {
      uint16_t syncword:12; // always set to '0b11111111'
      uint8_t  id:1;        // 0: MPEG-4, 1: MPEG-2
      uint8_t  layer:2;     // always set to '0b00'
      uint8_t  protection_absent:1;
      uint8_t  profile:2; // 00:MAIN, 01:LC, 10:SSR, 11:reserved
      uint8_t  sampling_frequency_index:4;
      uint8_t  private_bit:1;
      uint8_t  channel_configuration:3; 
      uint8_t  original_copy:1;  // 0:original, 1:copy
      uint8_t  home:1;
      uint8_t  copyright_identification_bit:1;
      uint8_t  copyright_identification_start:1;
      uint16_t aac_frame_length:13; // length of the frame incl. header
      uint16_t adts_buffer_fullness:11; // 0x7FF indicates VBR
      uint8_t  no_raw_data_blocks_in_frame:2;
      uint16_t crc_check;  // only if protetion_absent == 0

      static Header make_default(size_t aac_payload_length) {
        Header h;
        h.syncword = 0xFFF;
        h.id = 0; // MPEG-4
        h.layer = 0;
        h.protection_absent = 1;
        h.profile = 1; // LC
        h.sampling_frequency_index = 4; // 44100 XXX:
        h.private_bit = 0;
        h.channel_configuration = 2;
        h.original_copy = 0;
        h.home = 0;
        h.copyright_identification_bit = 0;
        h.copyright_identification_start = 0;
        h.aac_frame_length = aac_payload_length + 7;
        h.adts_buffer_fullness = 0x7FF;
        h.no_raw_data_blocks_in_frame = 0;
        
        return h;
      }

      bool dump(char* buf, size_t buf_size) const {
        if(buf_size < 7) {
          return false;
        }

        buf[0] = (syncword & 0xFF0) >> 4;

        buf[1] = (((syncword & 0x00F) << 4) +
                  (id << 3) +
                  (layer << 1) +
                  (protection_absent));
        
        buf[2] = ((profile << 6) +
                  (sampling_frequency_index << 2) +
                  (private_bit << 1) +
                  (channel_configuration >> 2));
        
        buf[3] = ((channel_configuration << 6) +
                  (original_copy << 5) +
                  (home << 4) +
                  (copyright_identification_bit << 3) +
                  (copyright_identification_start << 2) +
                  (aac_frame_length >> 11));
        
        buf[4] = (aac_frame_length >> 3);
        
        buf[5] = ((aac_frame_length << 5) +
                  (adts_buffer_fullness >> 6));
        
        buf[6] = ((adts_buffer_fullness << 2) + 
                  no_raw_data_blocks_in_frame);
        
        return true;
      }
    };
  }
}

/*
  sampling_frequency_index:
   0: 96000
   1: 88200
   2: 64000
   3: 48000
   4: 44100
   5: 32000
   6: 24000
   7: 22050
   8: 16000
   9: 12000
  10: 11025
  11: 8000
 */

#endif
