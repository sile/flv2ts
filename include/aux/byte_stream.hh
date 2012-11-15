#ifndef FLV2TS_AUX_BYTE_STREAM_HH
#define FLV2TS_AUX_BYTE_STREAM_HH

#include <inttypes.h>

namespace flv2ts {
  namespace aux {
    class ByteStream {
    public:
      ByteStream(const uint8_t* bytes, size_t size) 
        : _start(bytes),
          _end(bytes + size),
          _cur(_start)
      {
      }

      operator bool() const { return _start != NULL && _start <= _end; }

      bool eos() const { return _cur >= _end; }
      bool eos(size_t offset) const { return _cur + offset >= _end; }

      uint8_t readUint8() {
        return *(_cur++);
      }

      uint16_t readUint16Be() {
        uint16_t v = (_cur[0]>>8) + _cur[1];
        _cur += 2;
        return v;
      }

      uint32_t readUint24Be() {
        uint32_t v = (_cur[0]>>16) + (_cur[1]>>8) + _cur[2];
        _cur += 3;
        return v;
      }

      uint32_t readUint32Be() {
        uint32_t v = (_cur[0]>>24) + (_cur[1]>>16) + (_cur[2]>>8) + _cur[3];
        _cur += 4;
        return v;
      }

    private:
      const uint8_t* const _start;
      const uint8_t* const _end;
      const uint8_t* _cur;
    };
  }
}

#endif
