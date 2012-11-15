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
        if(! _in.can_read(9)) {
          return false;
        }
        
        header.signature[0] = _in.readUint8();
        header.signature[1] = _in.readUint8();
        header.signature[2] = _in.readUint8();
        if(strncmp("FLV", header.signature, 3) != 0) {
          return false;
        }
        
        header.version = _in.readUint8();
        
        uint8_t flags = _in.readUint8();
        
        header.is_audio = flags & 0x04;
        header.is_video = flags & 0x01;
        
        header.data_offset = _in.readUint32Be();

        return true;
      }

      bool parseTag(Tag& tag, uint32_t& prev_tag_size) {
        if(! _in.can_read(4)) {
          return false;
        }
        prev_tag_size = _in.readUint32Be();
        if(_in.eos()) {
          // last tag
          return true;
        } else {
          return parseTagImpl(tag);
        }
      }

      bool abs_seek(size_t pos) { return _in.abs_seek(pos); }
      bool rel_seek(ssize_t offset) { return _in.rel_seek(offset); }
      size_t position() const { return _in.position(); }
      bool eos() const { return _in.eos(); }
      
    private:
      bool parseTagImpl(Tag& tag) {
        if(! _in.can_read(11)) {
          return false;
        }
        
        uint8_t tmp = _in.readUint8();
        
        tag.filter = tmp & 0x20;
        if(tag.filter) {
          // encrypted file is unsupported
          return false;
        }

        tag.type = tmp & 0x1F;
        
        tag.data_size = _in.readUint24Be();
        
        uint32_t timestamp    = _in.readUint24Be();
        uint8_t timestamp_ext = _in.readUint8();
        tag.timestamp = static_cast<int32_t>((timestamp_ext << 24) + timestamp);
        
        tag.stream_id = _in.readUint24Be();
        
        if(! _in.can_read(tag.data_size)) {
          return false;
        }
        
        _in.rel_seek(tag.data_size);
                     
        return true;
      }

    private:
      aux::FileMappedMemory _fmm;
      aux::ByteStream _in;
    };
  }
}

#endif
