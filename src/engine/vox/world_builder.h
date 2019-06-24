#pragma once
#include "block.h"
#include "res/hmap.h"
#include "common/constants.h"
#include "common/stream.h"
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <sys/mman.h>
#include <dlfcn.h>
using namespace Magnum;
#define WORLD_SCALE 20
#define WORLD_OFFSET 20
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
public:
    float gen_pattern(int x, int y){
        float height=0.0;
        int ix=0;
        //for each power of 2, generate a random 4x4 pattern.
        //interpolate bilinearly and add octaves together.
        for(int r=29; r>0; r-=1){
            int xd=x>>r;
            int yd=y>>r;
            float s=(x%(1<<(r+1)))/float((1<<(r+1)));
            float t=(y%(1<<(r+1)))/float((1<<(r+1)));
            //printf("%f %f\n", s,t);
            int bit=(xd&1)*2+(yd&1);
            int pattern[3][3]={0};
            for(int dx=0; dx<=1; dx+=1){
                for(int dy=0; dy<=1; dy+=1){
                    int x2=xd+dx;
                    int y2=yd+dy;
                    int bit=(x2&1)*2+(y2&1);
                    //seed with position hash (to preserve spatial coherence) and do lcg once
                    pattern[dx][dy]=!!((((x2/2)*65539+(y2/2))*48271)&(1<<bit));
                }
            }
            float off_interp0=(1-s)*pattern[0][0]+s*pattern[1][0];
            float off_interp2=(1-s)*pattern[0][1]+s*pattern[1][1];
            float off_interp = 0.5*((1-t)*(off_interp0)+(t)*(off_interp2));
            height+=(off_interp)/exp(0.18*(r-6.5)*(r-6.5))  ;
            ++ix;       
        }
       // printf("%f ", 20.0*height+20.0);
        return height;
        
    }
    float gen_height(int x, int y){
        return gen_pattern(x+x0,y+y0);
    } 
       
    
    void get_generated_run(int x, int y, int z, block_t* b, int* n){
        
        if(method.compare("default")==0){
            
            int h=std::max(0,std::min(constants::WORLD_HEIGHT-64,(int)(gen_height(x,y)*WORLD_SCALE+WORLD_OFFSET)));
            
            if(z<(int)(h)){
                b[0]=STONE; n[0]=h-1-z;
                int min_neighbor_height=h;
                for(int i=-1; i<=1; ++i){ 
                    for(int j=-1; j<=1; ++j){ 
                       if(!i && !j) continue; 
            
                        int h2=std::max(0,std::min(constants::WORLD_HEIGHT-100,(int)(gen_height(x+i,y+j)*WORLD_SCALE+WORLD_OFFSET)));
                        min_neighbor_height=std::min(min_neighbor_height,h2);
                    }
                }
                if(min_neighbor_height>z){
                    b[0]=-STONE;
                    n[0]=std::max(1,min_neighbor_height-z-1);
                }
                else{
                    b[0]=STONE; n[0]=h-z;
                }
            }
                
            else if(z==(int)h){
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
        method=m;
        mode=md;
        so=nullptr;
        ij=0;
        x0=ox;
        y0=oy;
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
