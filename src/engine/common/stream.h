#pragma once
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <string>
#include <cstring>

    
  enum stream_mode{
      STREAM_READ=1,
      STREAM_WRITE=2
  };
class stream {
public:
  virtual size_t read(char *output, size_t size) = 0;
  virtual size_t write(const char *output, size_t size) = 0;
  virtual void close()=0;
};

class mmap_stream : public stream{
public:
  char* base_ptr;
  size_t base_size;
  stream_mode mode;
  mmap_stream(char* p, size_t s, stream_mode m){
      base_ptr=p;
      base_size=s;
      mode=m;
  }
  virtual size_t read(char* output, size_t size){
    assert(mode & STREAM_READ);
    std::memcpy(output, base_ptr, std::min(size, base_size));
    return std::min(size,base_size);
  }
  virtual size_t write(const char* output, size_t size){
    assert(mode & STREAM_WRITE);
    std::memcpy(base_ptr, output, std::min(size,base_size));
    return std::min(size,base_size);
  }
  virtual void close(){}
};
