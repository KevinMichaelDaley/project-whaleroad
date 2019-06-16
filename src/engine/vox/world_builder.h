#pragma once
#include "block.h"
#include "common/constants.h"
#include "common/stream.h"
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <sys/mman.h>
#include <dlfcn.h>
using namespace Magnum;
class world_builder {
private:
  static int fd;
  static uint64_t fsize;
  static std::string wfilename;
  static void* fptr;
  static bool is_generate;
public:
  enum map_mode{
      MAP_READ,
      MAP_WRITE
  };
private:
   static uint64_t mortonEncode(unsigned int x, unsigned int y, unsigned int z);
   static void* map_file();
public:
  static void map_world_file(std::string filename, bool& new_file);
  
  static stream * get_file_with_offset_r(Vector3i page_offset);

  static stream * get_file_with_offset_w(Vector3i page_offset);
  
  static void unmap_world_file();
  ~world_builder(){
      unmap_world_file();
  }
};

class worldgen_stream: public stream{
    std::string method;
    stream_mode mode;
    void* so;
    uint64_t ij;
    int16_t k;
public:
    typedef block_t (*generator)(int, int, block_t*, int*);
private:
    generator generate;
public:
    void get_generated_run(int xy, int z, block_t* b, int* n){
        if(method.compare("default")==0){
            if(z<10){
                b[0]=-SAND; n[0]=10-z;
            }
            else if(z==10){
                b[0]=SAND; n[0]=1;
            }
            else{
                b[0]=0;
                n[0]=constants::WORLD_HEIGHT-z;
            }
            return;
        }
        if(generate==nullptr){ b[0]=0; n[0]=constants::WORLD_HEIGHT-z; return;}
        generate(xy,z,b,n);
    }
    worldgen_stream(std::string m, stream_mode md, int ox, int oy, int oz){
        method=m;
        mode=md;
        so=nullptr;
        ij=0;
        for(unsigned i=0; i<32; ++i){
            ij+=((uint64_t)((ox>>i)&1u))<<(uint64_t)(2u*i);
            ij+=((uint64_t)((oy>>i)&1u))<<(uint64_t)(2u*i+1u);
        }
        k=0;
        generate=nullptr;
        if(method.compare("default")){
            so=dlopen(("../../src/engine/proc/terrain/gen_"+method+".so").c_str(), RTLD_NOW);
            generate=(generator) dlsym(so, "generate_run");
        }
    }
    virtual size_t read(char* output, size_t size){
        assert(mode & STREAM_READ);
        for(size_t off=0; off<size/sizeof(block_t); off+=2u){
            block_t b; int n;
            get_generated_run(ij, k, &b, &n);
            n=std::min(constants::WORLD_HEIGHT-(k), n);
            ((block_t*)output)[off]=b;
            ((block_t*)output)[off+1]=n;
            k+=n;
            if(k>=constants::WORLD_HEIGHT){
                ij++;
                k=0;
            }
        }
        return size;
    }
    virtual size_t write(const char* output, size_t size){
        assert(mode & STREAM_WRITE);
        return size;
    }
    
    virtual void close(){
        if(so) {dlclose(so); so=nullptr;}
    }
    ~worldgen_stream(){
        close();
    }
};
