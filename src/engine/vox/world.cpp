#include <immintrin.h> 
#include "world.h"
#include "block_iterator.h"
#include "gfx/chunk_mesh.h"
#include "extern/libmorton/libmorton/include/morton.h"
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Range.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#ifndef __ANDROID__
    #include <jemalloc/jemalloc.h>
    
    #define MALLOC_PAGE je_malloc
    #define FREE_PAGE(x) if(x) je_free(x)
#else
    #define MALLOC_PAGE malloc
    #define FREE_PAGE(x) if(x) free(x)
#endif
using namespace libmorton;

  int nearest_multiple_down(int x, int base) {
    return (x / base - (x%base<0)) * base;
  }

void world_page::bounds(int &x, int &y, int &z, int &d) {            //
  x = x0+0;
  y = y0+0;
  z = 0;
  d = dim+0;
}

void rle_decompress(
    block_t *blocks, stream *f,
    int dim) { // run length decoding routine corresponding to rle_compress. the
               // run of blocks restarts at every column.
  for (int i = 0; i < (int)dim * (int) dim; ++i) {
    uint16_t rle[2] = {0, constants::WORLD_HEIGHT};
    int k = 0;
    while (k < (int)constants::WORLD_HEIGHT - 1) {
      size_t read_size=f->read((char*)rle, 2 * sizeof(uint16_t));
      if (rle[1] == 0) {
        break;
      }
      for (int z = k; z < k + rle[1]; ++z) {
        blocks[i * constants::WORLD_HEIGHT + z] = *(block_t *)&(rle[0]);
      }
      k += rle[1];
    }
    for (int z = k; z < (int) constants::WORLD_HEIGHT; ++z) {
        blocks[i*constants::WORLD_HEIGHT+k]=0;
    }
  }
}
void rle_compress(block_t *blocks,
                  stream *f,
                  int dim) {
               // run length encoding routine.  only single columns are
               // compressed, for easy implementation.  this results in massive
               // reduction of file size, especially when the level is first
               // generated (and especially especially if a heightmap is used).
  for (uint64_t i = 0; i < (int) dim * dim; ++i) {
    int ix = i * constants::WORLD_HEIGHT;
    block_t b0 = -BEDROCK;
    uint16_t rle[constants::WORLD_HEIGHT] = {0};
    int j = 0;
    for (int k = 0; k < (int) constants::WORLD_HEIGHT; ++k) {
      if (b0 != blocks[ix + k]) {
        b0 = blocks[ix + k];
        j++;
        rle[j * 2] = *(uint16_t *)&b0;
        rle[j * 2 + 1] = 1;
      } else {
        rle[j * 2 + 1]++;
      }
      if (!b0) {
        rle[j * 2 + 1] = constants::WORLD_HEIGHT - k;
        break;
      }
    }
    f->write((char*)rle, (j * 2 + 2) * sizeof(uint16_t));
  }
}
bool world_page::load() { // load the page into memory from the file (or network data) or generate
                          // it.  it's not going to have lighting yet, we have
                          // to generate that as the player moves around.
  if (loaded) {
    return true;
  }
  nulled=false;
  loaded=true;
  blocks = (block_t*) MALLOC_PAGE(sizeof(block_t)*(uint64_t)dim *(uint64_t) dim * (uint64_t)constants::WORLD_HEIGHT);
  invisible_blocks =
      (uint16_t*)MALLOC_PAGE(sizeof(uint16_t)*(uint64_t)dim * (uint64_t)dim * (uint64_t)constants::WORLD_HEIGHT);
      
  zmap =
      (uint16_t*)MALLOC_PAGE(sizeof(uint16_t)*(int64_t)dim * (int64_t)dim );
  std::memset(zmap,0,sizeof(uint16_t)*dim*dim);
  std::memset(blocks, 0,
              dim * dim * constants::WORLD_HEIGHT * sizeof(block_t));
  std::memset(zmap, 0,
              dim * dim * sizeof(block_t));
  stream *f = world_builder::get_file_with_offset_r(Vector3i{x0, y0, 0});
  light =
      (uint8_t *)MALLOC_PAGE((uint64_t)dim *(uint64_t) dim * (uint64_t)constants::WORLD_HEIGHT * constants::LIGHT_COMPONENTS);
  
  if (f) {
        rle_decompress(blocks, f, dim);
        delete f;
        f=nullptr;
  } else {
        //world_builder::generate(blocks, dim, {x0, y0, z0});
  }
  dirty_list = (uint64_t*) MALLOC_PAGE((uint64_t)(dim * dim) / 32 * sizeof(uint64_t)); 
  std::memset(dirty_list, 0xff, (uint64_t)(dim * dim) / 32 * sizeof(uint64_t));
  std::memset(invisible_blocks, 0x0,
              (uint64_t) (dim * dim) * constants::WORLD_HEIGHT *
                  sizeof(uint16_t));
  for (int x = 0; x < (int) dim; ++x) {
    for (int y = 0; y < (int) dim; ++y) {
      set(x + x0, y + y0, constants::WORLD_HEIGHT-1,
          blocks[x * dim * constants::WORLD_HEIGHT +
                 y * constants::WORLD_HEIGHT+constants::WORLD_HEIGHT-1],true,false);
      for(int k=0; k<constants::WORLD_HEIGHT-1; ++k){
          if(blocks[x * dim * constants::WORLD_HEIGHT +
                 y * constants::WORLD_HEIGHT+k]==0){
                    zmap[x*dim+y]=k;
                    break;
          }
      }
    }
  }
  std::memset(light, 0, (uint64_t)(dim * dim) * constants::WORLD_HEIGHT * constants::LIGHT_COMPONENTS);
  return true;
}

bool world_page::save() { // save the current page to a file.  we generate
                          // everything except the actual block array on the fly
                          // (for example, lighting) and grid-based physics
                          // simulations are restarted from generated initial
                          // conditions when the player comes back
  // so we can use rle compression and save a ton of space (I've seen on the
  // order of 100:1 ratios for certain very coherent data).
  if (blocks == nullptr)
    return false;
  stream *f = world_builder::get_file_with_offset_w({x0, y0, 0});
  if (f == nullptr) {
    return false;
  }
  rle_compress(blocks, f, dim);
  delete f;
  return true;
}
bool world_page::swap() { // save the current page and then get rid of it so we
                          // can load a different one.
  // we won't be able to access any data from the current page while it is only
  // on disk but we can store certain information relevant to gameplay
  // elsewhere.
 // bool success = save();
    bool success=true;
    FREE_PAGE(blocks);
    FREE_PAGE(light);
    FREE_PAGE(dirty_list);
    FREE_PAGE(zmap);
    FREE_PAGE(invisible_blocks);
    blocks = nullptr;
    zmap = nullptr;
    light = nullptr;
    invisible_blocks=nullptr;
    dirty_list = nullptr;
    wld=nullptr;
    loaded=false;
    nulled=true;
    dim=0;
    x0=0;
    y0=0;
    z0=0;
    return success;
}
bool world_page::contains(int x, int y, int z) {
  if(nulled){
      return false;
  }
  if (x >= x0 && y >= y0) {
    if (x < (x0 + dim) && y < (y0 + dim)) {
      return true;
    }
  }
  return false;
}
block_t world_page::get(
    int x, int y,
    int z) { // this version of get() returns the block at (x,y,z) by value and
             // should be used if you only want to read the current block.  you
             // can retrieve a reference to as much as an entire column of
             // blocks (safely) by using the version below.
  if (!contains(x, y, z)) {
    return sky_column[0];
  }
  if (!loaded) {
    load();
  };
  return blocks[(x - x0) * dim * constants::WORLD_HEIGHT +
                (y - y0) * constants::WORLD_HEIGHT + z ];
}

block_t trash[65536] = {0};
block_t & world_page::get(
    int x, int y, int z,
    bool &
        valid) { // get the block at (x,y,z) by value.  if valid comes back
                 // true, we are guaranteed to be able to write to z-values
                 // [0,65535] for that particular column if it returns false,
                 // you have received a similarly valid pointer, but to a
                 // column of invalid data (meaning that writing to it does
                 // nothing defined and reading it won't give you the actual
                 // level data). internally, this is sometimes used to
                 // reference entire multi-column regions of blocks (for
                 // example in the terrain generation code and the lighting) but
                 // you should never really do that anywhere except in
                 // performance-critical engine code because it is super unsafe.
  if (!contains(x, y, z)) {
    valid = false;
    return trash[0];
  }
  if (!loaded) { // if we haven't loaded this page yet, try loading it.  if it's
                 // new, allocate it and empty all the memory to default values.
    load();
  }
  valid = true;
  return blocks[(x - x0) * dim * constants::WORLD_HEIGHT +
                (y - y0) * constants::WORLD_HEIGHT + z ];
}

uint8_t *world_page::get_light(
    int x, int y,
     int z) { 
  if (!contains(x, y, z)) {
    return nullptr; // if this page doesn't contain this position we cannot
                    // return a useful value.
  }
  if(!loaded){
      load();
  }
  return &(light[((x - x0) * dim * constants::WORLD_HEIGHT +
                            (y - y0) * constants::WORLD_HEIGHT + z)*constants::LIGHT_COMPONENTS 
                 ]); // light values, like block values, are guaranteed to be
                        // contiguous within pages.
}
bool world_page::set(
    int x, int y, int z, block_t b,
    bool update_neighboring_pvs,
    bool update_neighborhood
                    ) { // set a block at absolute world coordinates
                                   // (x,y,z).  if b is negative, the block is
                                   // assumed invisible.
  if (!contains(x, y, z)) {
    return false;
  }
  if (!loaded){
      load();
  }
  int64_t ix = (x - x0) * (int64_t) dim * (int64_t) constants::WORLD_HEIGHT +
                (y - y0) * (int64_t) constants::WORLD_HEIGHT + z ;

  
  block_t bprev=blocks[ix];
    uint8_t rle_update_mask = 0x10u;
    if(update_neighborhood){
        
        bool all = true;
        int nm=0;
        for (int i = -1; i <= 1; i += 1) {
            for (int j = -1; j <= 1; j += 1) {
              for (int k = -1; k <= 1; k += 1) {
                if (i && j && k) {
                    continue;
                }
                if (!i && !j && !k) {
                    continue;
                }
                
                bool on_border = !contains(x+i, y+j, z+k);
                int64_t jx = i * (int64_t) constants::WORLD_HEIGHT * (int64_t)dim +
                            j *  (int64_t) constants::WORLD_HEIGHT + k;
                block_t b0 = (!on_border)? blocks[ix + jx] : wld->get_voxel(x+i,y+j,z+k);
                if (std::abs(b) <= WATER) {
                   
                    //update visibility of neighbor block
                    if(!on_border){
                        blocks[ix + jx] = std::abs(b0);
                        uint64_t mask = (uint64_t)1uL << (uint64_t)(
                                        ((x + i) % 8) * 4 +
                                        ((y + j) % 4));
                        dirty_list[(x - x0 + i) / 8 * (dim / 4) + (y-y0 + j) / 4] |= mask;
                        rle_update_mask |= 1u << ((i + 1) * 2 + (j + 1));
                    }
                    else{
                        wld->set_voxel(x+i,y+j,z+k,std::abs(b0),true,false);
                    }
                }
                if(std::abs(b0)<=WATER){//FIXME: replace all instances of <= WATER with proper opacity function.
                     //reflect light from current block if neighbor is transparent and we are adding a new solid block.
                    
                    //we do this in the update_occlusion dynamically instead.
                    /*if(block_is_opaque(std::abs(b)) && !block_is_opaque(std::abs(bprev))){
                        uint8_t* L2=wld->get_light(x+i,y+j,z+k);
                        uint8_t L0[constants::LIGHT_COMPONENTS];
                        for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                            L0[m]=L2[m];
                        }
                        for(int m=6; m<constants::LIGHT_COMPONENTS; ++m){
                            int N=0;
                            int L22=0;
                            for(int n=0; n<constants::LIGHT_COMPONENTS; ++n){
                                L22+=L0[n]*(constants::LPV_WEIGHT[(n%6)*27+nm]>0)*(constants::LPV_WEIGHT[(m%6)*27+nm]<0);
                            }
                            L2[m]=std::min((int)L2[m]+L22/4,255);
                        }
                    }*/
                        
                        
                }
                
                if(block_is_opaque(std::abs(bprev)) && !block_is_opaque(std::abs(b))>AIR){
                    uint8_t* L2=wld->get_light(x+i,y+j,z+k);
                    for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                        
                        L2[m]=0;
                    }
                }
                    
                ++nm;
                    
                all = all && (std::abs(b0) > WATER);
            }
        }
      }
      
        if (all && b > AIR) {
            b = -std::abs(b);
        }
    }
  
                
  blocks[ix] = b;
  if(b>1){
      for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
        light[ix*constants::LIGHT_COMPONENTS+m]=0u;
      }
  }
  ix = (x - x0) * dim + (y - y0);
  int k, j = 0;
  int run_length = 0;
  if(z>0){
    if ((bprev <=1 || blocks[ix * (int)constants::WORLD_HEIGHT + z] <=1)  && b != 0) {
        int k2 = z - 1;
        while (blocks[ix *  (int)constants::WORLD_HEIGHT + k2] == 0) {
            blocks[ix *  (int)constants::WORLD_HEIGHT + k2] = AIR;
            --k2;
        }
    }
  }
  if (update_neighboring_pvs) { // update the skippable list (list of blocks
                                // which are invisible or totally transparent
                                // from any angle); saves massive amounts of
                                // time during mesh generation and rendering
    int iy = ix; // note that we have to do this for every neighboring column,
                 // otherwise if we delete a block from a column it won't update
                 // the mesh of the adjacent block properly.
    for (int i = -1; i <= 1; ++i) {
      for (int j = -1; j <= 1; ++j) {
        if ((rle_update_mask & (1u << ((i + 1) * 2 + (j + 1)))) == 0)
          continue;
        if (ix + i < 0 || ix + i >= (int) dim || iy + j >= (int) dim || iy - j < 0)
          continue;
        int k0=0;
        invisible_blocks[ix*(int) constants::WORLD_HEIGHT+k0]=blocks[ix*(int) constants::WORLD_HEIGHT+k0]>=0?0:1;
        for(int k=1; k<(int) constants::WORLD_HEIGHT; ++k){
            int b1=blocks[ix*(int) constants::WORLD_HEIGHT+k];
            if(b1>0){
                invisible_blocks[ix*(int) constants::WORLD_HEIGHT+k0]=std::max(k-1-k0,0);
                invisible_blocks[ix*(int) constants::WORLD_HEIGHT+k]=0;
                k0=k+1;
            }
            else if(b1==0){
                invisible_blocks[ix*(int) constants::WORLD_HEIGHT+k]=(int)constants::WORLD_HEIGHT-k;
            }   
            else{
                invisible_blocks[ix*(int) constants::WORLD_HEIGHT+k]=1;
            }   
        }
      }
    }
  }
  
  if(b>1){
    zmap[(x-x0)*dim+(y-y0)]=std::max((int)zmap[(x-x0)*dim+(y-y0)],z+1);
    for(int i=0; i<=z; ++i){
        for(int m=0; m<constants::LIGHT_COMPONENTS; m+=1){
            light[(((x - x0) * dim + (y - y0))*constants::WORLD_HEIGHT+i)*constants::LIGHT_COMPONENTS+m]=0u;
        }
    }
  }
  if(std::abs(b)<=1){
      int z0=constants::WORLD_HEIGHT-1;
      bool empty=true;
      for(int i=z; i<constants::WORLD_HEIGHT; ++i){
          
          if(std::abs(blocks[((x - x0) * dim + (y - y0))*constants::WORLD_HEIGHT+i])>1){
              empty=false;
              break;
          }
              
      }
      if(empty){
          b=0;
            for(int i=z; i<constants::WORLD_HEIGHT; ++i){
                blocks[((x - x0) * dim + (y - y0))*constants::WORLD_HEIGHT+i]=0;
            }
      }
      if(b==0){
        
        z0=std::min(z+1, constants::WORLD_HEIGHT-1);
        while(std::abs(blocks[((x - x0) * dim + (y - y0))*constants::WORLD_HEIGHT+z0])<=1){
          blocks[((x - x0) * dim + (y - y0))*constants::WORLD_HEIGHT+z0]=0;
          z0--;
          if(z0==0){
              break;
          }
        }
        int zmax=z0+1;
        zmap[(x-x0)*dim+(y-y0)]=zmax;
        
      }
      
  }
  uint64_t mask = (uint64_t)1uL
                  << (uint64_t)(((x-x0) % 8) * 4 +
                                ((y-y0) % 4));
  dirty_list[(x-x0) / 8 * (int)(dim / 4) + (y-y0) / 4] |= mask; 
  return true;
}
void world_page::reset_diff() {
  std::memset(dirty_list, 0, (dim * dim / 32) * sizeof(uint64_t));
}
static uint8_t l[constants::LIGHT_COMPONENTS*constants::WORLD_HEIGHT] = {255u};
uint8_t *world::get_light(int x, int y, int z) {
  world_page* current=get_page(x,y,z);
  if (!current->nulled && z>=0 && z<constants::WORLD_HEIGHT) {
    return current->get_light(x, y, z);
  }
  return l;
}
block_t world::get_voxel(int x, int y, int z) {
  if(z>=(int) constants::WORLD_HEIGHT){
      return 0;
  }
  world_page* current=get_page(x,y,0);
  if (!current->nulled) {
    block_t b0 = current->get(x, y, z);
    if (b0 != 0) {
      return b0;
    }
  }
  return (z==0) ? SAND : (z < ocean_level) ? WATER : 0;
}
block_t &world::get_voxel(int x, int y, int z, bool &valid) {

  world_page* current=get_page(x,y,z);
  if (!current->nulled) {
    return current->get(x, y, z, valid);
  }
  valid = false;
  return trash[0];
}
bool world::set_voxel(int x, int y, int z, block_t b,
                      bool update_neighboring_pvs, bool update_neighborhood) {
    
  world_page* current=get_page(x,y,z);

  if (!current->nulled) {
    if (current->set(x, y, z, b, update_neighboring_pvs, update_neighborhood)) {
      return true;
    }
  }
  return false;
}
void world::save_all() {
  for(int i=0,e=std::min(num_pages, constants::MAX_RESIDENT_PAGES); i<e; ++i){
    current_[i].swap();
  }
}
bool world::cleanup(int px, int py, int pz, int vanish_dist) {
  bool success = true;
  int x, y, z, d;
  for(int i=0,e=std::min(num_pages, constants::MAX_RESIDENT_PAGES); i<e; ++i){
      
    world_page* current=&(current_[i]);
    current->bounds(x, y, z, d);
    int dist[4] = {0};
    dist[0] = (x + d - px) * (x + d - px) + (y + d - py) * (y + d - py);
    dist[1] = (x - px) * (x - px) + (y - py) * (y - py);
    dist[2] = (x + d - px) * (x + d - px) + (y - py) * (y - py);
    dist[3] = (x - px) * (x - px) + (y + d - py) * (y + d - py);
    if (dist[0] > vanish_dist && dist[1] > vanish_dist && dist[2] > vanish_dist &&
        dist[3] > vanish_dist) {
        success = success && current->swap();
    }
  }
  return success;
}
world_page* world::get_page(int x, int y, int z){
    for(int i=0; i<(int)std::min(num_pages,(int)constants::MAX_RESIDENT_PAGES); ++i){
        if(current_[i].contains(x,y,0)){
                return (&current_[i]);
        }
    }
    current_[num_pages%constants::MAX_RESIDENT_PAGES].swap();
    world_page* last_page=&(current_[num_pages%constants::MAX_RESIDENT_PAGES]);
    new (last_page)  world_page(nearest_multiple_down(x,constants::PAGE_DIM),nearest_multiple_down(y,constants::PAGE_DIM),0,constants::PAGE_DIM, this);
    num_pages++;
    return last_page;
}
int world::get_z(int i, int j, uint16_t *skip_invisible_array,
                 world_page *current) { 
  if (current == nullptr)
    current = get_page(i, j, 0);
  /* return the height of the highest block which is not air or empty,
   * optionally writing into skip_invisible_array the array A s.t., for every
   * integer height z in the column, A[z]=next(z)-z where next(z) is the index
   * of the next potentially-visible block.
   */
  if (current->nulled)
    return ocean_level;

  if (!current->contains(i, j, 0))
    current = get_page(i, j, 0);

  if (current == nullptr)
    return ocean_level;

  if (current->blocks ==
      nullptr) { // if we don't actually store anything here, return the max
                 // height of the default column.
    return ocean_level;
  }
  int k = 0;
  int ix = 0;
  int64_t ii =
      (i-current->x0) * (int64_t) constants::WORLD_HEIGHT * (int64_t) current->dim + (j-current->y0) * (int64_t)constants::WORLD_HEIGHT;

  /*
  int zmax_a = constants::WORLD_HEIGHT-1;
  int zmax_b = 0;
  int zmax=zmax_a;  
  for(int t=0; t<(int)constants::WORLD_HEIGHT; ++t){
      
      // iterate from the top of the world (max 255) down until we hit a solid
      // block and then break and that's our return z value
      //except we actually do it with a binary search for the highest block preceding a 0 block.
      if(zmax_a-zmax_b<=1){
          zmax=zmax_a;
          break;
      }     
      int zmax_c=(zmax_b+zmax_a)/2;
      if(b[zmax_c]==0){
          zmax_a=zmax_c;
      }
      else{
          zmax_b=zmax_c;
      }
  }
  */
  int zmax=current->zmap[(i-current->x0)*current->dim+(j-current->y0)];
  if (skip_invisible_array == nullptr) return zmax;
  
  block_t *b = &(current->blocks[ii]);
  for (int k=0; k <= std::min(zmax, constants::WORLD_HEIGHT-k); ) { // the skippable array is stored in the world_page and
                      // updated every time we call set_block()
    int rl = current->invisible_blocks[ii + k];
    skip_invisible_array[k] = std::max(rl,1);
    k+=std::max(rl,1);
    if(b[k]==0){
        skip_invisible_array[k]=constants::WORLD_HEIGHT-k;
        break;
    }
  }
  return zmax; 
}

uint64_t* world_page::has_changes_in_8x4_block(int x, int y) {
  if(dirty_list==nullptr){
      load();
  }
  int xi = (x - x0) / 8;
  int yi = (y - y0) / 4;
  return &(dirty_list[(uint64_t)(xi * (dim / 4) + yi)]);
}


  world * world_view::get_world() { return wld; }
  void world_view::update_center(Vector3 player_position) {
    updated_center=true;
    float xx = player_position.x();
    float yy = player_position.y();
    float zz = player_position.z();
    center_old[0] = center[0];
    center_old[1] = center[1];
    center_old[2] = center[2];
    center[0] = std::floor(xx) / constants::CHUNK_WIDTH;
    center[1] = std::floor(yy) / constants::CHUNK_WIDTH;
    center[2] = std::floor(zz);
  }
  
  void world_view::update_occlusion(int subradius){
      
        int c0=center[0]*constants::CHUNK_WIDTH;
        int c1=center[1]*constants::CHUNK_WIDTH;
        int x0=c0-(subradius);
        int y0=c1-(subradius);
        int x1=c0+(subradius);
        int y1=c1+(subradius);
        for(int ix=x0; ix<=nearest_multiple(x1,64); ix+=64){
            for(int iy=y0; iy<=nearest_multiple(y1,64); iy+=64){
                update_occlusion(ix,ix+64,iy,iy+64);
            }
        }
        
  }
  
int xy2d (unsigned n, unsigned x, unsigned y) {
    return morton2D_64_encode(x, y);
}


void d2xy(unsigned n, unsigned* d, unsigned *x, unsigned *y, bool bounds_check_increment=true) {
long unsigned x2,y2;
morton2D_64_decode(d[0], x2,y2);
if(bounds_check_increment && (x2>=n || y2<0)){ x[0]=0; y[0]++; d[0]=xy2d(n,x[0],y[0]);}
else if(bounds_check_increment && (y2>=n || x2<0)){ y[0]=0; x[0]++; d[0]=xy2d(n,x[0],y[0]);}
else{
    x[0]=x2; y[0]=y2; d[0]++;
}

}


  void world_view::update_occlusion(int x0,int x1, int y0, int y1, int D) {
        x0-=D;
        y0-=D;
        y1+=D-1 ;
        x1+=D-1;
        world_page* page=wld->get_page(x0,y0,0);
        int sx,sy,sz,dim;
        
         bool valid=false;
         page->bounds(sx,sy,sz,dim);
         block_t* barray=&page->get(sx,sy,0,valid);
         int N=std::max(x1+1-x0,y1+1-y0);
          int N3=N;
         std::memset(neighbor_mask,0,N3*N3*sizeof(bool));
         constexpr int N2=3+constants::LIGHT_COMPONENTS;
         uint64_t changed=page->has_changes_in_8x4_block(x0 , y0)[0];
         unsigned i=0,j=0;
        for(unsigned d=0,count=0; count<=N*N; ++count){
                
                int x=i+x0;
                int y=j+y0;
                if(!page->contains(x,y,0)){
                    page=wld->get_page(x,y,0);
                    page->bounds(sx,sy,sz,dim);
                    barray=&page->get(sx,sy,0,valid);
                }
                changed=page->has_changes_in_8x4_block(x , y)[0];
                int64_t bit_x=(x-sx)%8;
                int64_t bit_y=(y-sy)%4;
                uint64_t bit=uint64_t(bit_x*4+bit_y);
                if(((changed)&(uint64_t(1u)<<bit))!=0){
                    
                        for(int dx=-D; dx<=D; ++dx){
                            for(int dy=-D; dy<=D; ++dy){
                                if(i+dx>=D && j+dy>=D && i+dx<N3-D && j+dy<N3-D){
                                    neighbor_mask[(i+dx)*N3+j+dy]=true;
                                }
                            }
                        }
                }
                
                
                //printf("%i %i\n",i,j);
                d2xy(N,&d,&i,&j);
                sun_depth[i*N3+j]=page->zmap[(x-sx)*dim+y-sy];
        }
        i=0; j=0;
        
        for(unsigned d=0,count=0; count<=N*N; ++count){
                
                
                
                
                int x=i+x0;
                int y=j+y0;
                if(x>x1 || x<x0) continue;
                if(y>y1 || y<y0) continue;
                if(!page->contains(x,y,0)){
                    page=wld->get_page(x,y,0);
                    page->bounds(sx,sy,sz,dim);
                    barray=&page->get(sx,sy,0,valid);
                }
                bool dirty=(neighbor_mask[i*N3+j]);  
                if(!dirty){
                    d2xy(N,&d,&i,&j);
                    continue;
                }
                std::memset(&(input[((i*N3+j)*constants::WORLD_HEIGHT)*N2]),0,sizeof(int16_t)*N2*constants::WORLD_HEIGHT);
                
                uint8_t* Lbase=page->get_light(x,y,0);
                   
                uint16_t skip_invisible_array[constants::WORLD_HEIGHT]={0};
                int z=wld->get_z(x,y,skip_invisible_array,page);
                const block_t* b2=&(barray[(((x-sx)*(int)dim)+(y-sy))*(int) constants::WORLD_HEIGHT]);
                
                for(int k=0; k<=std::min(z-1,constants::WORLD_HEIGHT-1); k+=std::max((int)skip_invisible_array[k],1)){
                        
                        int blk=std::abs(b2[k]);     
                        for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                            input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+m]=255-(int)Lbase[k*constants::LIGHT_COMPONENTS+m];
                        }
                        input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+constants::LIGHT_COMPONENTS]=(block_is_opaque(blk))?255:0;
                        input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+1+constants::LIGHT_COMPONENTS]=(int)block_emissive_strength(blk);
                        input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+2+constants::LIGHT_COMPONENTS]=(b2[k]>0)?1:0;   
                      
                        if(!blk){   
                            break;
                        }
               
                }
        }
        page=wld->get_page(x0+1,y0+1,0);
        
        page->bounds(sx,sy,sz,dim);
        barray=&page->get(sx,sy,0,valid);
                    
        unsigned i2=0,j2=0;

        
        for(unsigned d=0,count=0; count<=(N-2*D)*(N-2*D); ++count){
                i=i2+D;
                j=j2+D;
                int x=i+x0;
                int y=j+y0;
                if(!page->contains(x,y,0)){
                    page=wld->get_page(x,y,0);
                    page->bounds(sx,sy,sz,dim);
                }
                
                bool dirty=neighbor_mask[i*N3+j];
                if(!dirty){
                    
                    d2xy(N,&d,&i2,&j2);
                    continue;
                }
                
                uint8_t* Lbase=nullptr;
                Lbase=page->get_light(x,y,0);
                int max_delta=0;
                uint16_t skip_invisible_array[constants::WORLD_HEIGHT]={0};
                int z=wld->get_z(x,y,skip_invisible_array,page);
                const block_t* b2=&(barray[(((x-sx)*(int)dim)+(y-sy))*(int) constants::WORLD_HEIGHT]);
                
                uint8_t occ[128*128]={0};
                int zmaxneighbor=z;
                bool occ_any=false;
                if(i>=D && i<N-D){
                    if(j>=D && j<N-D){
                        for(int dx=-D; dx<=D; ++dx){
                            for(int dy=-D; dy<=D; ++dy){
                                int z1=sun_depth[(i+dx)*N3+(j+dy)];
                                int dzmax=z1-z;
                                int zmaxneighbor=std::max(z1,zmaxneighbor);
                                if(dzmax<=0){
                                    continue;
                                 }
                                if(!dx && !dy) continue;
                                int16_t* ip=&input[(((i+dx)*N3+j+dy)*constants::WORLD_HEIGHT+z)*N2];   
                                for(int dz=1; dz<=std::min(dzmax, constants::WORLD_HEIGHT-1-z); ++dz){
                                    //if(!ip[(dz-1)*N2+constants::LIGHT_COMPONENTS]){
                                     
                                        //continue;
                                    //}
                                    //rasterize the block bottom face
                                    //using integer arithmetic at 128x128 res
                                    //add a fudge factor of 1 to the block width to account for the side faces.
                                    int x0=(146*64*(2*dx-1))/(2*dz), x1=(146*64*(2*dy+3))/(2*dz); 
                                    int y0=(146*64*(2*dy-1))/(2*dz), y1=(146*64*(2*dy+3))/(2*dz); 
                                   // printf("%i %i %i %i\n", x0,y0,x1,y1);
                                    x0=std::max(x0,-64);
                                    y0=std::max(y0,-64);
                                    y1=std::min(y1,63);
                                    x1=std::min(x1,63);
                                    for(int i3=x0+64; i3<=x1+64; ++i3){
                                        for(int j3=y0+64; j3<=y1+64; ++j3){
                                            occ[i3*128+j3]=std::max(occ[i3*128+j3],uint8_t(128-(100*dz-2)/dz));
                                            occ_any=true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                
                int ao=0;
                if(!occ_any){
                    
                            for(int m=1; m<constants::LIGHT_COMPONENTS; m+=6){
                            //if(FacesOffset[m%6][2]<0) continue;
                            int Lold=Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m];
                            max_delta=std::max(max_delta,std::abs(255-(int)Lold));
                            Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m]=255u;
                            
                            input[((i*N3+j)*constants::WORLD_HEIGHT+z)*N2+m]=0u;
                            }
                            ao=255;
                     
                    
                }
                else{
                    for(int bit=0; bit<128*128; ++bit){
                        int dz=128-occ[bit];
                        ao+=dz+dz;
                    }
                        
                    ao=(int)(std::min(((ao))/(128.0*128.0),255.0));
                    
                    //if(ao) std::cerr<<ao<<" ";  
                            
                            for(int m=1; m<constants::LIGHT_COMPONENTS; m+=6){
                            //if(FacesOffset[m%6][2]<0) continue;                            
                                int Lold=Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m];
                                max_delta=std::max(max_delta,std::abs((int)ao-(int)Lold));
                                Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m]=ao;  
                                input[((i*N3+j)*constants::WORLD_HEIGHT+z)*N2+m]=255-ao;
                            }
                        
                    
                }
                for(int k=0; k<=z; k+=std::max((int)skip_invisible_array[k],1)){
                    
                            float L1[constants::LIGHT_COMPONENTS];
                            float L0[constants::LIGHT_COMPONENTS];
                            for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                                L1[m]=0.0;
                            }
                            for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                                L0[m]=Lbase[k*constants::LIGHT_COMPONENTS+m];
                            }
                            if(b2[k]<0){
                                continue;
                            }   
                            if(block_is_opaque(std::abs(b2[k]))){
                                continue;
                            }
                            for(int nm=0; nm<6; ++nm){
                                        int dx=FacesNormal[nm%6][0];
                                        int dy=FacesNormal[nm%6][1];
                                        int dz=FacesNormal[nm%6][2];
                                        if(k+dz<0 || k+dz>=constants::WORLD_HEIGHT){
                                            continue;
                                        }
                                        int ix2=((i+dx)*N3+j+dy)*constants::WORLD_HEIGHT+k+dz;
                                        if(input[ix2*N2+constants::LIGHT_COMPONENTS]){
                                            L1[constants::INVERSE_COMPONENTS[nm%6]+6]=L1[nm%6+6]*0.05;
                                            L1[nm%6+6]=-L1[nm%6+6];
                                            L1[constants::INVERSE_COMPONENTS[nm%6]+6]+=L1[nm%6]*0.05;
                                            L1[nm%6]=-L1[nm%6];
                                        }       
                                        else{
                                                        float fac=constants::LPV_BIAS[nm];
                                                        int m=nm;
                                                        L1[nm+6]+=(float)(256.0-input[ix2*N2+(nm)])*fac/256.0f;
                                                        
                                                        L1[nm+6]+=(float)(256.0-input[ix2*N2+(m+6)])/256.0f*fac;
                                                
                                            
                                        }
                            }
                            
                            for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                                int Lold=(int)L0[m];
                                int Lnew=std::max(0.1,std::min(float(Lold)+(L1[m]*254.0),254.0));
                                Lbase[k*constants::LIGHT_COMPONENTS+m]=(uint8_t) Lnew;
                                int delta=(int)Lnew-(int)Lold;
                                max_delta=std::max(std::abs(delta),max_delta);
                            }
                            
                }
                ao=std::min(ao,255);
                for(int k=z+1; k<zmaxneighbor+1; ++k){
                    for(int m=1; m<constants::LIGHT_COMPONENTS; m+=6){
                        Lbase[k*constants::LIGHT_COMPONENTS+m]=(unsigned)ao;
                    }
                }
                
                int bit=((x-x0)%8)*4+(y-y0)%4;
                 
                if(max_delta<4){     
                 page->has_changes_in_8x4_block(x , y)[0] ^=  ((uint64_t(1u)<<uint64_t(bit)));
                }   
                else{
                  page->has_changes_in_8x4_block(x , y)[0] |= uint64_t(1u)<<uint64_t(32u);
                }
                d2xy(N-D*2,&d,&i2,&j2);
               
        }   
        

        updated_center=false;
  }

  int world_view::nearest_multiple(int x, int base) {
    return (x / base + (x % base != 0)) * base;
  }
  

  void world_view::update_visible_list(
      int dx,
      int dy) { //every time the player moves one chunk along x or y, we have to update the corresponding row/column of chunks in the distance to maintain the current draw distance.\
                                            //this function does that by moving the pointer to every chunk mesh to a different position in the visible array and changing the starting position of the ones that just became invisible and triggering a mesh update for those chunks.
    // this way, we don't actually have to reallocate anything, which is good.
    std::vector<chunk_mesh *> all_visible2;
    all_visible2.resize(all_visible.size());
    std::vector<chunk_mesh *> reuse;
    int Nx=(radius/constants::CHUNK_WIDTH*2+1);
    reuse.reserve(Nx);
    for (int i = -radius; i <= radius; i += constants::CHUNK_WIDTH) {
      int cx = (i + radius) / constants::CHUNK_WIDTH;
      if (dx > 0) {
        reuse.push_back(all_visible[0 * Nx + cx]);
      } else if (dx < 0) {
        reuse.push_back(all_visible[(Nx - 1) * Nx + cx]);
      } else if (dy > 0) {
        reuse.push_back(all_visible[(0 + cx * Nx)]);
      } else if (dy < 0) {
        reuse.push_back(all_visible[(Nx - 1) + Nx * cx]);
      }
    }
    for (int i = -radius + (dx > 0) * constants::CHUNK_WIDTH;
         i <= radius - (dx < 0) * constants::CHUNK_WIDTH; i += constants::CHUNK_WIDTH) {

      int cx = (i + radius) / constants::CHUNK_WIDTH;
      for (int j = -radius + (dy > 0) * constants::CHUNK_WIDTH;
           j <= radius - (dy < 0) * constants::CHUNK_WIDTH; j += constants::CHUNK_WIDTH) {
        int cy = (j + radius) / constants::CHUNK_WIDTH;
        int cx2 = ((i - dx * constants::CHUNK_WIDTH) + radius) / constants::CHUNK_WIDTH;
        int cy2 = ((j - dy * constants::CHUNK_WIDTH) + radius) / constants::CHUNK_WIDTH;
        all_visible2[cx2 * Nx + cy2] = all_visible[cx * Nx + cy];
      }
    }
    for (int i = 0; i < all_visible.size(); ++i) {
      all_visible[i] = all_visible2[i];
    }
    int j = 0;
    for (int i = -radius; i <= radius;
         i += constants::CHUNK_WIDTH) { // update the starting position of each chunk mesh
                             // we are no longer able to see at the new position
      // i.e., move it to the opposite side of the world view.
      int cx = 0, cy = 0;
      if (dy < 0) {
        cy = 0 / constants::CHUNK_WIDTH;
        cx = (i + radius) / constants::CHUNK_WIDTH;
      } else if (dy > 0) {
        cy = (radius + radius) / constants::CHUNK_WIDTH;
        cx = (i + radius) / constants::CHUNK_WIDTH;
      } else if (dx > 0) {
        cx = (radius + radius) / constants::CHUNK_WIDTH;
        cy = (i + radius) / constants::CHUNK_WIDTH;
      } else if (dx < 0) {
        cx = 0 / constants::CHUNK_WIDTH;
        cy = (i + radius) / constants::CHUNK_WIDTH;
      }
      all_visible[cx * Nx + cy] = reuse[j];
      reuse[j]->start_at(
          (cx * constants::CHUNK_WIDTH - radius) + (center[0]*constants::CHUNK_WIDTH),
          (cy * constants::CHUNK_WIDTH - radius) + (center[1]*constants::CHUNK_WIDTH));
      reuse[j]->force_change();
      
          update_occlusion(reuse[j]->x0,reuse[j]->x0+constants::CHUNK_WIDTH, reuse[j]->y0,reuse[j]->y0+constants::CHUNK_WIDTH);
      reuse[j]->update(wld);
      ++j;
    }
  }

  void world_view::initialize_meshes() {
    int Nx=(radius/constants::CHUNK_WIDTH*2+1);
    for (int i = -radius; i <= radius; i += constants::CHUNK_WIDTH) {
      ++Nx;
      for (int j = -radius; j <= radius; j += constants::CHUNK_WIDTH) {
        all_visible.push_back(new chunk_mesh());
        all_visible[all_visible.size() - 1]->start_at(i + center[0]*constants::CHUNK_WIDTH,
                                                      j + center[1]*constants::CHUNK_WIDTH);
        all_visible[all_visible.size() - 1]->update(wld);
        //all_visible[all_visible.size() - 1]->copy_to_gpu();
      }
    }
  }

  void world_view::queue_update_stale_meshes() { // add all meshes with stale light values
                                     // or block values to a queue for updating.
      
    int Nx=(radius/constants::CHUNK_WIDTH*2+1);
    mesh_update_queue.reserve(all_visible.size());
    if (std::abs(center[0] - center_old[0]) >= 1) {
      update_visible_list((center[0] - center_old[0]), 0);
      center_old[0] = center[0];
    }
    if (std::abs(center[1] - center_old[1]) >= 1) {
      update_visible_list(0, (center[1] - center_old[1]));
      center_old[1] = center[1];
    }
    
    for (int dx = -32; dx <= 32; dx += constants::CHUNK_WIDTH) {
      for (int dy = -32; dy <= 32; dy += constants::CHUNK_WIDTH) {
          
            int c1 = (dx + radius) * Nx / (int64_t) constants::CHUNK_WIDTH + (dy+radius) / (int64_t) constants::CHUNK_WIDTH;
            if(all_visible[c1]==nullptr) continue;
            if (all_visible[c1]->is_dirty(wld)) {
                fast_update_queue.push_back(all_visible[c1]);
            }
      }
    }
    for (int dx = 0; dx <= radius; dx += constants::CHUNK_WIDTH) {
      for (int dy = 0; dy <= radius; dy += constants::CHUNK_WIDTH) {
        int c1 = dx * Nx / constants::CHUNK_WIDTH + dy / constants::CHUNK_WIDTH;

        int c2 = (dx + radius) * Nx / constants::CHUNK_WIDTH + dy / constants::CHUNK_WIDTH;

        int c3 = (dx)*Nx / constants::CHUNK_WIDTH + (dy + radius) / constants::CHUNK_WIDTH;

        int c4 = (dx + radius) * Nx / constants::CHUNK_WIDTH +
                 (dy + radius) / constants::CHUNK_WIDTH;
        int c[4] = {c1, c2, c3, c4};
        for (int ix = 0; ix < 4; ++ix) {
          bool already =
              false; // don't add a mesh to the queue if it is already queued
                     // for an update, that's just wasteful.
          chunk_mesh *chunk = all_visible[c[ix]];
          if(chunk==nullptr) continue;
          for (int i = mesh_update_head; i < (int) mesh_update_queue.size(); ++i) {
            if (chunk == mesh_update_queue[i]) {
              already = true;
              break;
            }
          }

          if (chunk->is_dirty(wld) && !already) {
            mesh_update_queue.push_back(chunk);
          }
        }
      }
    }
  }

  void world_view:: remesh_from_queue() {

    if ((int)mesh_update_queue.size() >= (14)) {
      mesh_update_head += mesh_update_queue.size() / 2;
    }
    int num_to_update =
        first_frame ? mesh_update_queue.size()
                    : 2; // we don't need to update more than one chunk mesh per
                         // frame and it takes a little while anyway
    for (int j = 0; j < num_to_update; ++j) {
      if (mesh_update_head < mesh_update_queue.size()) {
        update_occlusion(mesh_update_queue[mesh_update_head]->x0,mesh_update_queue[mesh_update_head]->x0+constants::CHUNK_WIDTH, mesh_update_queue[mesh_update_head]->y0,mesh_update_queue[mesh_update_head]->y0+constants::CHUNK_WIDTH);
        mesh_update_queue[mesh_update_head]->update(wld);
        ++mesh_update_head;
      } else {
        mesh_update_queue.clear();
        mesh_update_head = 0;
      }
    }
      num_to_update =
        first_frame ? fast_update_queue.size()
                    : 2; // now do the fast update queue
      for (int j = 0; j < num_to_update; ++j) {
        if (fast_update_head < fast_update_queue.size()) {
            update_occlusion(fast_update_queue[fast_update_head]->x0,fast_update_queue[fast_update_head]->x0+constants::CHUNK_WIDTH, fast_update_queue[fast_update_head]->y0,fast_update_queue[fast_update_head]->y0+constants::CHUNK_WIDTH);
        fast_update_queue[fast_update_head]->update(wld);
        ++fast_update_head;
      } else {  
        fast_update_queue.clear();
        fast_update_head = 0;
      }
    }
  
    first_frame=false;
}

  const std::vector<chunk_mesh *> & world_view::get_all_visible() { return all_visible; }
  world_view::~world_view() {
    for (chunk_mesh *m : all_visible) {
      delete[] m;
    }
  }
