#include "world.h"
#include "block_iterator.h"
#include "gfx/chunk_mesh.h"
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Range.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#ifdef JEMALLOC_PAGE
    #include <jemalloc/jemalloc.h>
    
    #define MALLOC_PAGE je_malloc
    #define FREE_PAGE(x) if(x) je_free(x)
#else
    #define MALLOC_PAGE malloc
    #define FREE_PAGE(x) if(x) free(x)
#endif


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
      (uint16_t*)MALLOC_PAGE(sizeof(uint16_t)*(uint64_t)dim * (uint64_t)dim );
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
                    if(std::abs(b)>AIR && std::abs(bprev)<=AIR){
                        uint8_t* L2=wld->get_light(x+i,y+j,z+k);
                        uint8_t L0[constants::LIGHT_COMPONENTS];
                        for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                            L0[m]=L2[m];
                        }
                        for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                            int N=0;
                            int L22=0;
                            for(int n=0; n<constants::LIGHT_COMPONENTS; ++n){
                                L22+=L2[n]*(constants::LPV_WEIGHT[n*27+nm]>0)*(constants::LPV_WEIGHT[m*27+nm]<0);
                            }
                            L2[m]=std::min((int)L2[m]+L22/4,255);
                        }
                    }
                        
                        
                }
                else if(std::abs(b)<=AIR && std::abs(bprev)>AIR){
                    for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                        uint8_t* L2=wld->get_light(x+i,y+j,z+k);
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
        for(int m=0; m<constants::LIGHT_COMPONENTS; m+=6){
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
  int zmax=current->zmap[(i-current->x0)*constants::WORLD_HEIGHT+(j-current->y0)];
  if (skip_invisible_array == nullptr) return zmax;
  
  block_t *b = &(current->blocks[ii]);
  for (int k=0; k <= zmax; ) { // the skippable array is stored in the world_page and
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

/*
        
void world_light::calculate_block_shadow(block_t *column, uint16_t *skip_neighbors,
                            uint16_t *z_neighbors, int ix, int iy, int x, int y,
                            world_page *page, world *wld) {
     page->dirty_list[(x - page->x0)/8 * page->dim / 4 + (y - page->y0)/4] =
      (uint64_t)1uL << (uint64_t)32uL;
  uint16_t *skip_invisible_array =
      &skip_neighbors[0];
  int z = z_neighbors[0];
  int z0 = 0;
  bool valid = false;
  block_t *b = &(page->get(x, y, 0, valid));
  if(!valid){
      return;
  }
  uint8_t *Lbase = page->get_light(x, y, 0);
  uint8_t max_delta=0;
  for (int i = 0; i <= std::min(z, (int)constants::WORLD_HEIGHT);
       i += std::max(int(skip_invisible_array[i]), 1)) {
    
    block_t b2 = b[i];
    bool solid = block_is_opaque(b2);
    bool visible = block_is_visible(b2);
    
    if (!b[i-1] && i > 8)
      break;
    if (!solid || !visible) {
      continue;
    }
    uint8_t ao=0;

    for (int dy = -31; dy < 31; ++dy) { // slice index
      uint64_t occlusion_slice[2] = {0x0, 0x0};
      for (int dx = -31; dx < 31; ++dx) { // x offset

        uint16_t *skip2 = &skip_neighbors[((dy) * (63+4) + dx) *
                                          (int)constants::WORLD_HEIGHT];
        int zcol = z_neighbors[((dy) * (63+4) + dx)];

        for (int dz = 32 - abs(dx); dz < 32; dz += std::max((int)skip2[i + dz],1)) { // z offset

          if (i + dz > zcol)
            break;

          bool solid = skip2[i + dz] <= 1uL;
          bool solid2 = skip2[i + abs(dx)] <= 1uL;
          int index1 = (abs(dx) << 5) + dz;
          int index2 = (dz << 5) + abs(dx);

          occlusion_slice[dx > 0] |=
              ((uint64_t)lut_projected_area[index1] * solid) *
                  ((1u << (32 / dz)) - 1)
              << abs(dy);

          occlusion_slice[dx > 0] |=
              ((uint64_t)lut_projected_area[index2] * solid2) *
                  ((1u << (32 / dz)) - 1)
              << abs(dy);
        }
      }

      for (uint64_t b = 0; b < 64; ++b) {
          for (int c = 0; c <= 1; ++c) {
            ao += (!!(occlusion_slice[c] & (uint64_t(1) << b)));
          }
      }
    }

    
      uint8_t L_old = Lbase[i* constants::LIGHT_COMPONENTS], L_new = (ao)*(256/128);
      Lbase[i* constants::LIGHT_COMPONENTS] = L_new;
      uint8_t delta=std::abs(L_new - L_old);
      max_delta = std::max(delta, max_delta);
      //     }
      // above, explained:
      // we compute ambient occlusion for a block using a sort of aliased cone
      // tracing, as follows:
      //  *****************
      //   ***************
      //    *************
      //     ***********
      //      *********
      //       *******
      //        *****
      //         ***
      //          x
      //   consider the region bounded by a frustum facing upward with angle
      //   from center to exterior set to 45* and height set to 32 blocks. a
      //   single slice of this cone appears as above, though it is probably
      //   stretched vertically due to the spacing of your text editor, and of
      //   course this drawing is not 32 blocks high.  but the idea is there.
      //   here, asterisks represent unoccluded empty space, spaces represent
      //   set voxels, and hyphens represent occluded space; which is backwards
      //   but it's so we can clearly see the entire cone. now, suppose we have
      //   a vertical column of blocks set at some arbitrary position inside
      //   this cone slice which is capable of occluding light.  then, imagine
      //   casting rays using bresenham's line algorithm until we've projected
      //   the entire solid volumetric slice onto the hemicube plane, centered
      //   at our ray origin, with angle 90*, thereby determining how much of
      //   the far hemisphere is obscured.
      //    **-----********
      //     **----*******
      //      **-  ******
      //       *********
      //        *******
      //         *****
      //          ***
      //           x
      //
      // we get something like this.  Intuitively speaking, we just need to know
      // the number of asterisks and hyphens in the top row for any
      // configuration of spaces, which will determine how much light is let
      // through. but there is a much simpler way to do this than casting rays,
      // which is to use an integer LUT over the entire frustum slice.  Every
      // element is a bitmask describing the top row after a particular voxel is
      // set, and we & them together to get the final value. in this case, for
      // instance, we would have
      //
      //
      //    110000011111111
      //    **-----********
      //     **----*******
      //      **-  ******
      //       *********
      //        *******
      //         *****
      //          ***
      //           x                  = 0x60FF
      //
      //  as a final bitmask, and the number of bits set would be the ambient
      //  occlusion value for that slice. Except it would be a 32-bit value.
      //  (we could also convolve the bitmask with a spherical harmonic basis to
      //   get directional light, in which case the method is generally the same).
      //
      //  we obviously have to do many slices, but the good news is that our
      //  rays are radially symmetric.  what this means is that moving a block
      //  to the next slice is equivalent to moving it left or right, i.e., a
      //  bitshift if the block intersects the next slice and no operation
      //  otherwise.
      // each block at z-distance dz from the ray origin will contribute to all
      // slices within 32/dz of the current ray origin, but since the bitshifts
      // we are doing form a geometric series with base 2, the total
      // contribution is easy to compute; it's lut_value*(1<<(32/dz)-1) by
      // formula. it also means we can increase our occlusion distance to 80,
      // because we need only store a mask for the left half of each slice; we
      // can simply reverse the bits to obtain the corresponding mask in the
      // right half, for the block which is reflected across the normal axis.
      // Finally, if we want to rotate the cone 90 degrees, that's really just
      // the same as swapping the x and y coordinates of the block, which is
      // again a simple bitwise operation, before the LUT lookup. using this
      // method we can raycast the entire hemisphere.

      //(because ambient occlusion has the property of reciprocity, every block
      // which contributes significantly to the current occlusion value must be
      // added to the dirty_list.  this is obviously localized to an 32x32x32
      // hemicube around the block).

      z0 = i;
    }

    if (max_delta >= 16) {
      int xx, yy, zz, dim;
      page->bounds(xx, yy, zz, dim);
      for (int dx = -4; dx < 4; ++dx) {
        for (int dy = -8; dy < 8; ++dy) {
          if (!dx && !dy) {
            continue;
          }
          page->has_changes_in_8x4_block(x + dx * 8, y + dy * 4) |= 0xffffffffuL;
        }
      }
    }
  }
  */
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
  //simpler flood fill lighting
  void world_view::update_occlusion(int subradius){
      
        int c0=center[0]*constants::CHUNK_WIDTH;
        int c1=center[1]*constants::CHUNK_WIDTH;
        int x0=c0-(subradius+1);
        int y0=c1-(subradius+1);
        int x1=c0+(subradius+1);
        int y1=c1+(subradius+1);
        update_occlusion(x0,x1,y0,y1);
        
  }
  void world_view::update_occlusion(int x0,int x1, int y0, int y1) {
        int i=0;
        world_page* page=wld->get_page(x0,y0,0);
        int sx,sy,sz,dim;
        
         bool valid=false;
         page->bounds(sx,sy,sz,dim);
         block_t* barray=&page->get(sx,sy,0,valid);
         int N=std::max(x1-x0,y1-y0);
         int N3=N+4;
         std::memset(neighbor_mask,0,N3*N3*sizeof(bool));
         constexpr int N2=3+constants::LIGHT_COMPONENTS;
         uint64_t changed=page->has_changes_in_8x4_block(x0 , y0)[0];
        for(int x=x0; x<=x1; ++x){
            int j=0;
            for(int y=y0; y<=y1; ++y){
                
                
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
                    if(i>=1 && j>=1){
                        for(int dx=-1; dx<=1; ++dx){
                            for(int dy=-1; dy<=1; ++dy){
                                neighbor_mask[(i+dx)*N3+j+dy]=true;
                            }
                        }
                    }
                    else{
                        neighbor_mask[i*N3+j]=true;
                    }
                }
                ++j;
            }
            ++i;
        }
        
        i=0;
        for(int x=x0; x<=x1; ++x){
            int j=0;
            for(int y=y0; y<=y1; ++y){
                if(!page->contains(x,y,0)){
                    page=wld->get_page(x,y,0);
                    page->bounds(sx,sy,sz,dim);
                    barray=&page->get(sx,sy,0,valid);
                }
                bool dirty=(neighbor_mask[i*N3+j]);  
                if(!dirty){
                    continue;
                }
                std::memset(&(input[((i*N3+j)*constants::WORLD_HEIGHT)*N2]),0,sizeof(uint8_t)*N2*constants::WORLD_HEIGHT);
                
                uint8_t* Lbase=page->get_light(x,y,0);
                   
                uint16_t skip_invisible_array[constants::WORLD_HEIGHT]={0};
                int z=wld->get_z(x,y,skip_invisible_array,page);
                const block_t* b2=&(barray[(((x-sx)*(int)dim)+(y-sy))*(int) constants::WORLD_HEIGHT]);
                
                for(int k=0; k<=std::min(z-1,constants::WORLD_HEIGHT-1); k+=std::max((int)skip_invisible_array[k],1)){
                        
                        int blk=std::abs(b2[k]);     
                        for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                            input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+m]=255-(int)Lbase[k*constants::LIGHT_COMPONENTS+m];
                        }
                        input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+constants::LIGHT_COMPONENTS]=(block_is_opaque(blk))?255u:0u;
                        input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+1+constants::LIGHT_COMPONENTS]=(int)block_emissive_strength(blk);
                        input[((i*N3+j)*constants::WORLD_HEIGHT+k)*N2+2+constants::LIGHT_COMPONENTS]=(b2[k]>0)?1u:0u;   
                      
                        if(!blk){   
                            break;
                        }
                }
                
                ++j;
            }
            ++i;
        }
        i=1;
        page=wld->get_page(x0+1,y0+1,0);
        
        page->bounds(sx,sy,sz,dim);
        barray=&page->get(sx,sy,0,valid);
                    
        
        for(int x=x0+1; x<=x1-1; x+=1){
            int j=1;
            for(int y=y0+1; y<=y1-1; y+=1){  
                if(!page->contains(x,y,0)){
                    page=wld->get_page(x,y,0);
                    page->bounds(sx,sy,sz,dim);
                }
                
                uint8_t* Lbase=nullptr;
                Lbase=page->get_light(x,y,0);
                int max_delta=0;
                bool dirty=neighbor_mask[i*N3+j];
                if(!dirty){
                    continue;
                }
                uint16_t skip_invisible_array[constants::WORLD_HEIGHT]={0};
                int z=wld->get_z(x,y,skip_invisible_array,page);
                const block_t* b2=&(barray[(((x-sx)*(int)dim)+(y-sy))*(int) constants::WORLD_HEIGHT]);
                
                int occ[1024]={0};
                bool occ_any=false;
                if(i>0 && i<N-1){
                    if(j>0 && j<N-1){
                        for(int dx=-1; dx<=1; ++dx){
                            for(int dy=-1; dy<=1; ++dy){
                                int z1=page->zmap[(x+dx-sx)*dim+(y+dy-sy)];
                                int dzmax=z1-z;
                                if(dzmax<=0){
                                    continue;
                                }
                                if(!dx && !dy) continue;
                                uint8_t* ip=&input[((i*N3+j+dx*N3+dy)*constants::WORLD_HEIGHT+z)*N2];
                                
                                for(int dz=1; dz<=std::min(dzmax, constants::WORLD_HEIGHT-z); ++dz){
                                    if(!ip[dz*N2+2+constants::LIGHT_COMPONENTS]) break;
                                    if(!ip[dz*N2+constants::LIGHT_COMPONENTS]) continue;
                                    //rasterize the block bottom face
                                    //using integer pixel coordinates at 32x32 res
                                    int x0=8*dx/dz;   
                                    int y0=8*dy/dz;
                                    int y1=8*dy/dz+8/dz;         
                                    int x1=8*dx/dz+8/dz;    
                                    for(int i2=x0+16; i2<=x1+16; ++i2){
                                        for(int j2=y0+16; j2<=y1+16; ++j2){
                                            if(i2<0 || i2>31 || j2<0 || j2>31){ continue; }
                                            occ[i2*32+j2]=std::max(64+80/dz,occ[i2*32+j2]);  
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
                    
                    for(int m=0; m<constants::LIGHT_COMPONENTS; m+=6){
                            
                            int Lold=Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m];
                            max_delta=std::max(max_delta,std::abs(255-(int)Lold));
                            Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m]=255u;
                            
                            input[((i*N3+j)*constants::WORLD_HEIGHT+z)*N2+m]=0u;
                        
                    }   
                    
                }
                else{
                    for(int bit=0; bit<1024; ++bit){
                        ao+=std::max(0,64*!occ[bit]);
                    }
                    
                    ao=std::min((ao*255)/(1024*128),255);
                    //if(ao) std::cerr<<ao<<" ";  
                    for(int m=0; m<constants::LIGHT_COMPONENTS; m+=6){
                            
                            int Lold=Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m];
                            max_delta=std::max(max_delta,std::abs((int)ao-(int)Lold));
                            Lbase[std::min(z,constants::WORLD_HEIGHT-1)*constants::LIGHT_COMPONENTS+m]=ao;
                            input[((i*N3+j)*constants::WORLD_HEIGHT+z)*N2+m]=255-ao;
                        
                    }
                }
                for(int k=0; k<=std::min(z+1,constants::WORLD_HEIGHT-1); k+=std::max((int)skip_invisible_array[k],1)){
                            float L1[constants::LIGHT_COMPONENTS];
                            
                            for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                                L1[m]=0.0;
                            }
                            if(b2[k]<0){
                                continue;
                            }   
                            if(block_is_opaque(std::abs(b2[k]))){
                                continue;
                            }
                            for(int dx=-1; dx<=1; ++dx){
                                for(int dy=-1; dy<=1; ++dy){
                                    for(int dz=-1; dz<=1; ++dz){
                                        
                                        if(!dx && !dy && !dz){
                                            continue;
                                        }
                                        if(k+dz<0 || k+dz>=constants::WORLD_HEIGHT){
                                            continue;
                                        }
                                        int ix2=((i+dx)*N3+j+dy)*constants::WORLD_HEIGHT+k+dz;
                                        if(input[ix2*N2+constants::LIGHT_COMPONENTS]){
                                            continue;
                                        }
                                        for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                                            float component_dot=constants::LPV_WEIGHT[(m%6)*27+((dx+1)*3+dy+1)*3+dz+1];
                                            L1[m]+=(float)(255-input[ix2*N2+m])/255.0*0.00001*float(component_dot>=1)+(float)input[ix2*N2+1+constants::LIGHT_COMPONENTS]/255.0;
                                        }
                                    }
                                }
                            }
                            
                            for(int m=0; m<constants::LIGHT_COMPONENTS; ++m){
                                int Lold=(int)Lbase[k*constants::LIGHT_COMPONENTS+m];
                                int Lnew=std::min(float(Lold)+(L1[m]*255.0),255.0);
                                Lbase[k*constants::LIGHT_COMPONENTS+m]=(uint8_t) Lnew;
                                int delta=(int)Lnew-(int)Lold;
                                max_delta=std::max(std::abs(delta),max_delta);
                            }
                            
                            if(!b2[k]){
                                break;
                            }
                }
                
                 int bit=((x-x0)%8)*4+(y-y0)%4;
                 
                if(max_delta<16){     
                 page->has_changes_in_8x4_block(x , y)[0] ^=  ((uint64_t(1u)<<uint64_t(bit)));
                }   
                else{
                  page->has_changes_in_8x4_block(x , y)[0] |= uint64_t(1u)<<uint64_t(32u);
                }
                
                j+=1;
            }
            i+=1;
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
          (cx * constants::CHUNK_WIDTH - radius) + nearest_multiple(center[0], constants::CHUNK_WIDTH),
          (cy * constants::CHUNK_WIDTH - radius) + nearest_multiple(center[1], constants::CHUNK_WIDTH));
      reuse[j]->force_change();
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
        all_visible[all_visible.size() - 1]->copy_to_gpu();
      }
    }
  }

  void world_view::queue_update_stale_meshes() { // add all meshes with stale light values
                                     // or block values to a queue for updating.
      
    int Nx=(radius/constants::CHUNK_WIDTH*2+1);
    mesh_update_queue.reserve(all_visible.size());
    if (std::abs(center[0] - center_old[0]) == 1) {
      update_visible_list(center[0] - center_old[0], 0);
      center_old[0] = center[0];
    }
    if (std::abs(center[1] - center_old[1]) == 1) {
      update_visible_list(0, center[1] - center_old[1]);
      center_old[1] = center[1];
    }
    
    for (int dx = -32; dx <= 32; dx += constants::CHUNK_WIDTH) {
      for (int dy = -32; dy <= 32; dy += constants::CHUNK_WIDTH) {
        int c1 = (dx + radius) * Nx / (int64_t) constants::CHUNK_WIDTH + (dy+radius) / (int64_t) constants::CHUNK_WIDTH;
        if (all_visible[c1]->is_dirty(wld)) {
            
          update_occlusion(all_visible[c1]->x0-1,all_visible[c1]->x0+16, all_visible[c1]->y0-1,all_visible[c1]->y0+16);
          all_visible[c1]->update(wld);
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

  void world_view::remesh_from_queue() {

    if ((int)mesh_update_queue.size() >= (radius)) {
      mesh_update_head += mesh_update_queue.size() / 2;
    }
    int num_to_update =
        first_frame ? mesh_update_queue.size()
                    : 1; // we don't need to update more than one chunk mesh per
                         // frame and it takes a little while anyway
    for (int j = 0; j < num_to_update; ++j) {
      if (mesh_update_head < mesh_update_queue.size()) {
        update_occlusion(mesh_update_queue[mesh_update_head]->x0-1,mesh_update_queue[mesh_update_head]->x0+16, mesh_update_queue[mesh_update_head]->y0-1,mesh_update_queue[mesh_update_head]->y0+16);
        mesh_update_queue[mesh_update_head]->update(wld);
        ++mesh_update_head;
      } else {
        mesh_update_queue.clear();
        mesh_update_head = 0;
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
