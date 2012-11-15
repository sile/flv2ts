#ifndef FLV2TS_FLV_PARSER_HH
#define FLV2TS_FLV_PARSER_HH

#include "header.hh"
#include "tag.hh"
#include <aux/file_mapped_memory.hh>
#include <inttypes.h>

namespace flv2ts {
  namespace flv {
    class Parser {
    public:
      Parser(const char* filepath) 
        : _fmm(filepath)
      {
      }

      operator bool() const { return _fmm; }

      void parseHeader(Header& header) {
      }

      void parseTag(Tag& tag) {
      }

      bool seek(uint64_t pos) {
        return true;
      }
      
    private:
      aux::FileMappedMemory _fmm;
    };
  }
}

#endif
