#ifndef FLV2TS_FLV_PARSER_HH
#define FLV2TS_FLV_PARSER_HH

#include "header.hh"
#include "tag.hh"
#include <aux/file_mapped_memory.hh>
#include <aux/byte_stream.hh>
#include <inttypes.h>
#include <string.h>

namespace flv2ts {
  namespace flv {
    class Parser {
    public:
      Parser(const char* filepath) 
        : _fmm(filepath),
          _in(_fmm.ptr<const uint8_t>(), _fmm.size())
      {
      }

      operator bool() const { return _fmm && _in; }

      bool parseHeader(Header& header) {
        if(_in.eos(9)) {
          return false;
        }
        
        header.signature[0] = _in.readUint8();
        header.signature[1] = _in.readUint8();
        header.signature[2] = _in.readUint8();
        if(strncmp("FLV", header.signature, 3) != 0) {
          return false;
        }
        
        header.version = _in.readUint8();
        
        const uint8_t flags = _in.readUint8();
        
        header.is_audio = flags & 0x04;
        header.is_video = flags & 0x01;
        
        header.data_offset = _in.readUint32Be();
        
        return true;
      }

      void parseTag(Tag& tag) {
      }

      bool seek(uint64_t pos) {
        return true;
      }
      
    private:
      aux::FileMappedMemory _fmm;
      aux::ByteStream _in;
    };
  }
}

#endif
