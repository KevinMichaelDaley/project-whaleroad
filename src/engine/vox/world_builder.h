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
};

class worldgen_stream: public stream{
    std::string method;
    stream_mode mode;
    int offset_x, offset_y, offset_z;
    void* so;
    int offset_read_head;
    int i, j, k;
public:
    typedef block_t (*generator)(int, int, int, block_t*, int*);
private:
    generator generate;
public:
    void get_generated_run(int x, int y, int z, block_t* b, int* n){
        if(method.compare("default")==0){
            if(z<10){
                b[0]=SANDSTONE; n[0]=10-z;
            }
            else{
                b[0]=0;
                n[0]=constants::WORLD_HEIGHT-z;
            }
            return;
        }
        if(generate==nullptr){ b[0]=0; n[0]=constants::WORLD_HEIGHT-z; return;}
        generate(x,y,z,b,n);
    }
    worldgen_stream(std::string m, stream_mode md, int ox, int oy, int oz){
        method=m;
        mode=md;
        offset_x=ox; offset_y=oy; offset_z=oz;
        i=j=k=0;
        generate=nullptr;
        if(method.compare("default")){
            so=dlopen(("../../src/engine/proc/terrain/gen_"+method+".so").c_str(), RTLD_NOW);
            generate=(generator) dlsym(so, "generate_run");
        }
    }
    virtual size_t read(char* output, size_t size){
        assert(mode & STREAM_READ);
        for(int off=offset_read_head; off<size/sizeof(block_t)+offset_read_head; off+=2){
            block_t b; int n;
            get_generated_run(i+offset_x,j+offset_y,k+offset_z, &b, &n);
            n=std::min(constants::WORLD_HEIGHT-(k+offset_z),(unsigned) n);
            ((block_t*)output)[off-offset_read_head]=b;
            ((block_t*)output)[off-offset_read_head+sizeof(block_t)]=n;
            k+=n;
            if(k+offset_z>constants::WORLD_HEIGHT){
                j++;
                k=0;
            }
            if(j%1024==0){
                i++;
                j=0;
            }
        }
        offset_read_head+=size;
        return size;
    }
    virtual size_t write(const char* output, size_t size){
        assert(mode & STREAM_WRITE);
        return size;
    }
    
    virtual void close(){
        if(so) {dlclose(so); so=nullptr;}
    }
};
