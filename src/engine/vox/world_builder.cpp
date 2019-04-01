#include "world_builder.h"
#include "block.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h>

  int world_builder::fd;
  uint64_t world_builder::fsize;
  std::string world_builder::wfilename;
  void* world_builder::fptr;
   uint64_t world_builder::mortonEncode(unsigned int x, unsigned int y, unsigned int z){
        uint64_t answer = 0;
        for (uint64_t i = 0; i < (sizeof(uint64_t)* 8)/3; ++i) {
            answer |= ((x & ((uint64_t)1 << i)) << 2*i) | ((y & ((uint64_t)1 << i)) << (2*i + 1)) | ((z & ((uint64_t)1 << i)) << (2*i + 2));
        }
        return answer;
    }

   void* world_builder::map_file(){
    fd=open(wfilename.c_str(),O_RDWR);
    fsize=lseek(fd, 0, SEEK_END);
    lseek(fd,0,SEEK_SET);
    fptr=mmap(0, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return fptr;
  }

  void  world_builder::map_world_file(std::string filename, bool& new_file){
    wfilename=filename;
    new_file=false;
    FILE* fp=fopen(filename.c_str(), "r");
    if(fp==nullptr){
        fp=fopen(filename.c_str(), "r+");
        new_file=true;
    }
    if(fp!=nullptr){
        fclose(fp);
    }
    map_file();
  }
  
  stream *  world_builder::get_file_with_offset_r(Vector3i page_offset){
    uint64_t z_order_offset=mortonEncode(page_offset.x(), page_offset.y(), page_offset.z());
    ptrdiff_t offset_in_bytes=z_order_offset*sizeof(block_t);
    return new mmap_stream((char*)(fptr+offset_in_bytes), fsize-offset_in_bytes, STREAM_READ);
  }

   stream *  world_builder::get_file_with_offset_w(Vector3i page_offset){
    uint64_t z_order_offset=mortonEncode(page_offset.x(), page_offset.y(), page_offset.z());
    ptrdiff_t offset_in_bytes=z_order_offset*sizeof(block_t);
    return new mmap_stream((char*)(fptr+offset_in_bytes), fsize-offset_in_bytes, STREAM_WRITE);
  }
  void world_builder::unmap_world_file(){
      if(fptr!=nullptr){
        munmap(fptr, fsize);
        fptr=nullptr;
        fsize=0;
        fd=-1;
      }
  }
