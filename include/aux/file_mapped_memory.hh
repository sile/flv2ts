#ifndef FLV2TS_AUX_FILE_MAPPED_MEMORY_HH
#define FLV2TS_AUX_FILE_MAPPED_MEMORY_HH

#include <inttypes.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

namespace flv2ts {
  namespace aux {
    class FileMappedMemory {
    public:
      FileMappedMemory(const char* filepath)
        : _ptr(MAP_FAILED), _size(0) 
      {
        int fd = open(filepath, O_RDWR);
        if(fd == -1) {
          return;
        }

        struct stat statbuf;
        if(fstat(fd, &statbuf) == -1) {
          return;
        }
        _size = statbuf.st_size;
        _ptr = mmap(0, _size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);

	close(fd);
      }

      FileMappedMemory(const char* filepath, size_t start, size_t end)
        : _ptr(MAP_FAILED), _size(end - start) 
      {
        int fd = open(filepath, O_RDWR);
        if(fd == -1) {
          return;
        }

        _ptr = mmap(0, _size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, start);

	close(fd);
      }

      ~FileMappedMemory() {
        munmap(_ptr, _size);
      }
      
      operator bool() const { return _ptr != MAP_FAILED; } 
      
      template <class T>
      T* ptr() const { return *this ? reinterpret_cast<T*>(_ptr) : NULL; } 
      
      template <class T> 
      T* ptr(size_t offset) const { return reinterpret_cast<T*>(ptr<char>()+offset); } 
      
      size_t size() const { return _size; } 

    private:
      void* _ptr;
      size_t _size;
    };
  }
}

#endif
