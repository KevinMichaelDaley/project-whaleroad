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
   static void* map_file();
public:
    
  static uint64_t mortonEncode(unsigned int x, unsigned int y);
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
    uint64_t ij, xz;
    int16_t k;
public:
    typedef block_t (*generator)(int, int, block_t*, int*);
private:
    generator generate;
    float height[8388608];
public:
    uint64_t zadd(uint64_t z, uint64_t w)
    {
        uint64_t xsum = (z | 0xAAAAAAAAAAAAAAAA) + (w & 0x5555555555555555);
        uint64_t ysum = (z | 0x5555555555555555) + (w & 0xAAAAAAAAAAAAAAAA);
        return (xsum & 0x5555555555555555) | (ysum & 0xAAAAAAAAAAAAAAAA);
    }
    uint64_t zsub(uint64_t z, uint64_t w)
    {
        uint64_t xdiff = (z & 0x5555555555555555) - (w & 0x5555555555555555);
        uint64_t ydiff = (z & 0xAAAAAAAAAAAAAAAA) - (w & 0xAAAAAAAAAAAAAAAA);
        return (xdiff & 0x5555555555555555) | (ydiff & 0xAAAAAAAAAAAAAAAA);
    }

    float gen_height(uint64_t xyc, int r=50){
        int xy=xyc%8388608;
        if(r==0){
                int x=0;
                int y=0;
                for(int i=0; i<32; ++i){
                    x|=(xyc>>i)&(1<<i);
                    y|=(xyc>>(i+1))&(1<<i);
                }
                srand(x/18+(y/18)*100000);
                return (rand()%1000)/1000.0;
        }   
        else{
            float havg=height[xy];
            float hdiff=0.0, hdiff2=0.0;
            for(int i=1; i<17; ++i){
                hdiff=std::max(hdiff,std::max(height[zadd(xy,i)%8388608]-havg,0.0f));
                hdiff2=std::min(hdiff,std::min(height[zadd(xy,i)%8388608]-havg,0.0f));
                hdiff=std::max(hdiff,std::max(height[zsub(xy,i)%8388608]-havg,0.0f));
                hdiff2=std::min(hdiff2,std::min(height[zsub(xy,i)%8388608]-havg,0.0f));
            }
            return havg+(hdiff+hdiff2)*0.25f;
        }
    }
    void get_generated_run(uint64_t xy, int z, block_t* b, int* n){
        if(method.compare("default")==0){
            if(z<(int)(height[xy%8388608]*300.0)-1){
                b[0]=STONE; n[0]=(int)(height[xy%8388608]*300.0)-1-z;
                bool neg=true;
                for(int i=0; i<4; ++i){ 
                    if(int(height[zadd(xy,i)%8388608]*300.0)<z){
                        neg=false;
                    }
                    if(int(height[zsub(xy,i)%8388608]*300.0)<z){
                        neg=false;
                     }
                }
                if(neg){
                    b[0]=-STONE;
                }
            }
            else if(z<(int)(height[xy%8388608]*300.0)){
                
                b[0]=STONE; n[0]=1;
            }
                
            else if(z==(int)(height[xy%8388608]*300.0)){
                b[0]=GRASS; n[0]=1;
            }
            else{
                b[0]=0;
                n[0]=constants::WORLD_HEIGHT-z;
            }
            return;
        }
        else{
        if(generate==nullptr){ b[0]=0; n[0]=constants::WORLD_HEIGHT-z; return;}
        generate(xy,z,b,n);
        }
    }
    worldgen_stream(std::string m, stream_mode md, int ox, int oy, int oz){
        
        method=m;
        mode=md;
        so=nullptr;
        ij=0;
        xz=0;
        for(unsigned i=0; i<32; ++i){
            xz+=((uint64_t)((ox>>i)&1u))<<(uint64_t)(2u*i);
            xz+=((uint64_t)((oy>>i)&1u))<<(uint64_t)(2u*i+1u);
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
            
            for(int t=0; t<10; ++t){
                height[ij%8388608]=gen_height(ij,t);
            }
             height[ij%8388608]=std::min(0.8,std::max(0.01,height[ij%8388608]+0.5))/4.0;
            int xz=0;
            get_generated_run(ij, k, &b, &n);
            n=std::min(constants::WORLD_HEIGHT-(k), n);
            ((block_t*)output)[off]=b;
            ((block_t*)output)[off+1]=n;    
            k+=n;
            if(k>=constants::WORLD_HEIGHT){
                ij+=1;
                if(ij%constants::PAGE_DIM==0){
                    xz=zadd(xz,3);
                }
                else{
                    xz=zadd(xz,1);
                }   
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
