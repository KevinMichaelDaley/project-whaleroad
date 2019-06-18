#pragma once
#include "block.h"
#include "world_builder.h"
#include "block_iterator.h"
#include "common/constants.h"
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <array>
using namespace Magnum;
class world;

double now();
class chunk_mesh;
class world_view;
class world_light;
class world_page { // a single page of the terrain; multiple can be loaded at
                   // once and placed arbitrarily but they generally shouldn't
                   // intersect as it will cause problems with the rendering.
                   // this way we can have arbitrarily large worlds in every
                   // direction and stream them to and from the disk as needed.
  friend world;
  friend world_view;
  friend world_light;
  friend chunk_mesh;

private:
  bool nulled;
  int x0, y0,
      z0; // the offset of the bottom-left-back corner of the page extents
  long long dim; // this is the size of the page in both x and y
  block_t *blocks; // the array of voxels by palette index (see block.h for more
                   // info).  The block value at point P=(x+x0,y+y0,z+z0) is at
                   // index i=(x*dim+y)*WORLD_HEIGHT+z of this array.
  unsigned char *light;
  uint16_t *zmap;
  uint16_t  *invisible_blocks;
  uint64_t
      *dirty_list; // a bitfield for each 8x4 square region along the xy plane.
                   // Setting bit 32 triggers a mesh update around the region.
                   // setting bit b in [0,31] triggers a lighting update for
                   // column {x=b/4, y=b%4} of the region.  updating the
                   // lighting results in bit 32 being set.
  world* wld;
  bool loaded;
public:
  world_page(int x0_, int y0_, int z0_, int dim_, world* wld_)

      : x0(x0_), y0(y0_), z0(z0_), blocks(nullptr), light(nullptr), zmap(nullptr),
        dirty_list(nullptr), invisible_blocks(nullptr), dim(dim_), nulled(false) , loaded(false), wld(wld_){
    assert(dim > 0);
  }

  world_page(): blocks(nullptr), light(nullptr), dirty_list(nullptr),zmap(nullptr), invisible_blocks(nullptr), dim(0), nulled(true), loaded(false), wld(nullptr) {}
  void bounds(int &x, int &y, int &z, int &d);
  bool load();
  bool save();
  bool swap();
  bool contains(int x, int y, int z);
  block_t get(int x, int y, int z);
  block_t &get(int x, int y, int z, bool &valid);
  unsigned char *get_light(int x, int y, int z);
  bool set(int x, int y, int z, block_t b, bool update_rle = true, bool update_neighborhood=true);
  void reset_diff();
  uint64_t* has_changes_in_8x4_block(int x, int y);
};

class world { // this is the class that manages the world and loads the block
              // pages.
public:
  int ocean_level;
  std::array<world_page, constants::MAX_RESIDENT_PAGES> current_;
  int num_pages;
  world_page* get_page(int i, int j, int k);
  unsigned char *get_light(int x, int y, int z);
  block_t &get_voxel(int x, int y, int z, bool &valid);
  block_t get_voxel(int x, int y, int z);
  bool set_voxel(int x, int y, int z, block_t b, bool update_rle = true, bool update_neighborhood=true);
  bool cleanup(int px, int py, int pz, int vanish_dist);
  void save_all();
  int get_z(int x, int y, uint16_t *rle = nullptr, world_page* page=nullptr);
    
  static world* load_or_create(std::string name, bool& new_world){
      world_builder::map_world_file(name, new_world);
      return new world();
  }
  world(){
      ocean_level = 8;
      num_pages=0;
      
    for(int i=0; i<(int)constants::MAX_RESIDENT_PAGES; ++i){
        current_[i]=world_page();
    }
  }
};


class world_view {
private:
  bool first_frame;
  world *wld;
  bool updated_center;
  std::vector<chunk_mesh *> all_visible;
  int center[3], center_old[3];
  int radius;
  std::vector<chunk_mesh *> mesh_update_queue;
  int mesh_update_head;

        uint8_t* input;
        bool* neighbor_mask;
public:
  world *get_world() ;
  world_view(world* w, Vector3 center0, int rad): center{(int)(center0.x()/constants::CHUNK_WIDTH), (int)(center0.y()/constants::CHUNK_WIDTH), (int)center0.z()},
                                                    center_old{(int)(center0.x()/constants::CHUNK_WIDTH), (int)(center0.y()/constants::CHUNK_WIDTH), (int)center0.z()}, 
                                                    wld{w},
                                                    radius{rad},
                                                    mesh_update_head{0},
                                                    first_frame{true},
                                                    updated_center{true}
                                    {
                                        
                                        input=new uint8_t[constants::WORLD_HEIGHT*200*200*(constants::LIGHT_COMPONENTS+3)]; 
                                        
                                        neighbor_mask=new bool[200*200];
                                        
                                    }
  void update_center(Vector3 player_position) ;
  void update_occlusion(int subradius) ;
  void update_occlusion(int x0, int x1, int y0, int y1) ;

  int nearest_multiple(int x, int base) ;
  void update_visible_list(
      int dx,
      int dy) ;

  void initialize_meshes() ;
  void queue_update_stale_meshes() ;
  void remesh_from_queue() ;

  const std::vector<chunk_mesh *> &get_all_visible() ;
  ~world_view();
};
/*
class world_light{
public:
    static void calculate_block_shadow(block_t *column, uint16_t *skip_neighbors,
                                uint16_t *z_neighbors, int ix, int iy, int x, int y,
                                world_page *page, world *wld) ;
};*/
