#include "world_builder.h"
#include "block.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int world_builder::fd;
uint64_t world_builder::fsize;
std::string world_builder::wfilename;
void *world_builder::fptr;
bool world_builder::is_generate = false;
uint64_t world_builder::mortonEncode(unsigned int x, unsigned int y) {
  uint64_t answer = 0;
  for (uint64_t i = 0; i < (sizeof(uint64_t) * 8) / 2; ++i) {
    answer |=
        ((x & ((uint64_t)1 << i)) << i) | ((y & ((uint64_t)1 << i)) << (i + 1));
  }
  return answer;
}

void *world_builder::map_file() {
  fd = open(wfilename.c_str(), O_RDWR);
  fsize = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  fptr = mmap(nullptr, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  return fptr;
}

void world_builder::map_world_file(std::string filename, bool &new_file) {
  wfilename = filename;
  new_file = false;
  FILE *fp = fopen(filename.c_str(), "r");
  if (fp == nullptr) {
    fp = fopen(filename.c_str(), "r+");
    new_file = true;
    is_generate = true;
  } else {
    is_generate = false;
  }
  if (fp != nullptr) {
    fclose(fp);
  }
  map_file();
}

stream *world_builder::get_file_with_offset_r(Vector3i page_offset) {
  if (is_generate) {
    return new worldgen_stream("default", STREAM_READ, page_offset.x(),
                               page_offset.y(), 0);
  }
  uint64_t z_order_offset = mortonEncode(page_offset.x(), page_offset.y());
  ptrdiff_t offset_in_bytes = z_order_offset * sizeof(block_t);
  return new mmap_stream(((char *)fptr) + offset_in_bytes,
                         fsize - offset_in_bytes, STREAM_READ);
}

stream *world_builder::get_file_with_offset_w(Vector3i page_offset) {
  uint64_t z_order_offset = mortonEncode(page_offset.x(), page_offset.y());
  ptrdiff_t offset_in_bytes = z_order_offset * sizeof(block_t);
  return new mmap_stream(((char *)fptr) + offset_in_bytes,
                         fsize - offset_in_bytes, STREAM_WRITE);
}
void world_builder::unmap_world_file() {
  if (fptr != nullptr) {
    munmap(fptr, fsize);
    fptr = nullptr;
    fsize = 0;
    fd = -1;
  }
}
