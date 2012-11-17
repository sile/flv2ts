#ifndef FLV2TS_AUX_BYTE_STREAM_HH
#define FLV2TS_AUX_BYTE_STREAM_HH

#include <inttypes.h>
#include <string.h>

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
      bool can_read(size_t size) const { return _cur + size <= _end; }

      uint8_t readUint8() {
        return *(_cur++);
      }
      int8_t readInt8() { return static_cast<int8_t>(readUint8()); }

      uint16_t readUint16Be() {
        uint16_t v = (_cur[0]<<8) + _cur[1];
        _cur += 2;
        return v;
      }
      int16_t readInt16Be() { return static_cast<int16_t>(readUint16Be()); }

      uint32_t readUint24Be() {
        uint32_t v = (_cur[0]<<16) + (_cur[1]<<8) + _cur[2];
        _cur += 3;
        return v;
      }

      int32_t readInt24Be() {
        uint32_t v = readUint24Be();
        return (v < 0x800000) ? v : v - 0x1000000;
      }

      uint32_t readUint32Be() {
        uint32_t v = (_cur[0]<<24) + (_cur[1]<<16) + (_cur[2]<<8) + _cur[3];
        _cur += 4;
        return v;
      }
      int32_t readInt32Be() { return static_cast<int32_t>(readUint32Be()); }

      const uint8_t* read(size_t size) {
        const uint8_t* tmp = _cur;
        _cur += size;
        return tmp;
      }

      bool abs_seek(size_t pos) {
        if(_start + pos > _end) {
          return false;
        }
        _cur = _start + pos;
        return true;
      }

      bool rel_seek(ssize_t offset) {
        if(_cur + offset < _start) {
          return false;
        }
        if(_cur + offset > _end) {
          return false;
        }
        _cur += offset;
        return true;
      }

      size_t position() const { return _cur - _start; }

    private:
      const uint8_t* const _start;
      const uint8_t* const _end;
      const uint8_t* _cur;
    };
  }
}

#endif
