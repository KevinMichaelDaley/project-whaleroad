#pragma once
#include "block.h"
#include "common/constants.h"
#include "common/stream.h"
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <sys/mman.h>
#include <dlfcn.h>
using namespace Magnum;
#define WORLD_SCALE 80
#define WORLD_OFFSET 10
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
    uint64_t ij, x0, y0;
    int16_t k;
public:
    typedef block_t (*generator)(int, int, int,     block_t*, int*);
private:
    generator generate;
    float height[(constants::PAGE_DIM+20)*(constants::PAGE_DIM+20)];
    int voronoi_x[1024];
public:

    float gen_height(int x, int y, int r=50){
        
        if(r==0){
            float hmin=100000;
            for(int i=0; i<20; ++i){
                int x2=(voronoi_x[i]%(155*155))%155+1024;
                int y2=(voronoi_x[i]%(155*155))/155+1024;
                hmin=std::min(hmin,(float) ((x2-x)*(x2-x)+(y2-y)*(y2-y))/(355.0f*355.0f));
            }   
            return 0.4-std::min(0.3f,hmin);
        }       
        else{
            float havg=height[(x-x0+10)*(constants::PAGE_DIM+20)+y-y0+10];
            float hdiff=0.0;
            int ix1=(x+10-x0)*(constants::PAGE_DIM+20)+y+10-y0;
            for(int i=-1; i<=1; ++i){
                
                if(x-x0+i<-10 || x-x0+i >= constants::PAGE_DIM+10) continue;
                for(int j=-1; j<=1; ++j){
                    if(!i && !j) continue;
                    if(y-y0+j<-10 || y-y0+j >= constants::PAGE_DIM+10) continue;
                    int ix2=(i)*(constants::PAGE_DIM+20)+j;
                    hdiff+=height[ix1+ix2]-height[ix1];
                }
            }
            return havg+(hdiff)/20.0;
        } 
       
    }
    void get_generated_run(int x, int y, int z, block_t* b, int* n){
        if(method.compare("default")==0){
            if(z<(int)(height[(x+10)*(constants::PAGE_DIM+20)+y+10]*WORLD_SCALE+WORLD_OFFSET)-1){
                b[0]=STONE; n[0]=(int)(height[(x+10)*(constants::PAGE_DIM+20)+y+10]*WORLD_SCALE+WORLD_OFFSET)-1-z;
                bool neg=true;
                for(int i=-1; i<=1; ++i){ 
                    for(int j=-1; j<=1; ++j){ 
                    if(int(height[(x+10+i)*(constants::PAGE_DIM+20)+y+10+j]*WORLD_SCALE+WORLD_OFFSET)<z+1){
                        neg=false;
                    }
                    if(int(height[(x+10+i)*(constants::PAGE_DIM+20)+y+10+j]*WORLD_SCALE+WORLD_OFFSET)<z+1){
                        neg=false;
                     }
                    }
                }
                if(neg){
                    b[0]=-STONE;
                }
            }
            else if(z<(int)(height[(x+10)*(constants::PAGE_DIM+20)+y+10]*WORLD_SCALE+WORLD_OFFSET)){
                
                b[0]=STONE; n[0]=1;
            }
                
            else if(z==(int)(height[(x+10)*(constants::PAGE_DIM+20)+y+10]*WORLD_SCALE+WORLD_OFFSET)){
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
        generate(x,y,z,b,n);
        }
    }
    worldgen_stream(std::string m, stream_mode md, int ox, int oy, int oz){
        srand(0);
        for(int i=0; i<1024; ++i){
            voronoi_x[i]=rand();
        }
        method=m;
        mode=md;
        so=nullptr;
        ij=0;
        x0=ox;
        y0=oy;
        k=0;
        generate=nullptr;
        
                for(int t=0; t<3; ++t){
        for(int i=-10; i<constants::PAGE_DIM+10; ++i){
            
            for(int j=-10; j<constants::PAGE_DIM+10; ++j){
                    height[(i+10)*(constants::PAGE_DIM+20)+j+10]=gen_height(i+(int)x0,j+(int)y0,t);
             
            }
        }   
                }
        if(method.compare("default")){
            so=dlopen(("../../src/engine/proc/terrain/gen_"+method+".so").c_str(), RTLD_NOW);
            generate=(generator) dlsym(so, "generate_run");
        }
    }
    virtual size_t read(char* output, size_t size){
        assert(mode & STREAM_READ);
        for(size_t off=0; off<size/sizeof(block_t); off+=2u){
            block_t b; int n;
            
            
            int xz=0;
            get_generated_run(ij/constants::PAGE_DIM,ij%constants::PAGE_DIM, k, &b, &n);
            n=std::min(constants::WORLD_HEIGHT-(k), n);
            ((block_t*)output)[off]=b;
            ((block_t*)output)[off+1]=n;    
            k+=n;
            if(k>=constants::WORLD_HEIGHT){
                ij+=1;
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
