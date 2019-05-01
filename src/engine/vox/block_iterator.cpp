#include "block_iterator.h"
block_filter_chain::block_filter_chain() : passes{} {}

void block_filter_chain::add_pass(column_operation *pass) { passes.push_back(pass); }

void block_iterator::iter_columns(world *wld, Magnum::Range3Di bounds, block_filter_chain::column_operation op,
                                      int z_neighborhood_size,
                                      bool diff_only) {
  Vector3i bbL = bounds.backBottomLeft();
  Vector3i ftR = bounds.frontTopRight();
  int x0 = bbL.x(); int x1 = std::max(int(ftR.x()), x0 + 1);
  int y0 = bbL.x(); int y1 = std::max(int(ftR.y()), y0 + 1);
  uint16_t *neighborhood=nullptr;

  int sizex = 8 + z_neighborhood_size, sizey = 4 + z_neighborhood_size;
  if (z_neighborhood_size > 1) {
    neighborhood = new uint16_t[(sizex) * (sizey) * constants::WORLD_HEIGHT];
  }

  for (int x = x0; x < x1;) {
    for (int y = x0; y < y1;) {
      world_page *page = &(wld->get_page(x, y, 0));
      bool valid = false;

      int sx, sy, sz, dim; // get the intersection of the page AABB and the
                           // current region
      page->bounds(sx,sy,sz,dim);
      int i1 = std::min(sx + dim, x1), j1 = std::min(sy + dim, y1),
          i0 = std::max(x0, sx), j0 = std::max(y0, sy);
          
      block_t &bref = page->get(sx, sy, 0, valid);

      assert(valid);
      for (int i = i0; i < i1; i += 8) {
        for (int j = j0; j < j1; j += 4)   {
      // build the union of the neighborhoods of every column in the 8x4
      // subchunk
      
            uint16_t z_neighbors[(sizex)* (sizey)] = {0};
                for (int i2 = -sizex / 2; i2 < sizex / 2; ++i2) {
                    for (int j2 = -sizey / 2; j2 < sizey / 2; ++j2) {
                        
                        if (z_neighborhood_size > 1) {    
                            world_page* page2=nullptr;
                            if(page->contains(x+i2,y+j2,0)){
                                page2=page;
                            }
                            z_neighbors[(i2+sizex/2) * (sizey+1) + (j2+sizey/2)] =
                                    wld->get_z(x + i2, y + j2, &(neighborhood[((i2+sizex/2) * (sizey) + (j2+sizey/2)) * constants::WORLD_HEIGHT]), page2);
                            
                        }
                    }
                }
            uint32_t mask = page->has_changes_in_8x4_block(i, j) & 0xffffffffu;
            if (diff_only && mask == 0) {
              continue;
            }
            
      // now iterate over the subchunk and apply the operation
            for (int bit = 0; bit < 32; ++bit) {
                if (diff_only && ((mask & bit) == 0)) {
                    continue;
                }
                int xb=bit/4;
                int yb=bit/4;

                int column_index = constants::WORLD_HEIGHT * ((i-sx+xb) * dim + (j-sy+yb));

                block_t *columns = &((&bref)[column_index]);

                op(columns, &(neighborhood[constants::WORLD_HEIGHT*((xb) * (sizey) + yb)]), &(z_neighbors[(xb) * (sizey) + yb]), i, j, x,y, page, wld);
            }
        }
        ++x;
        ++y;
        }
      }
    }
    if(neighborhood!=nullptr){
        delete[] neighborhood;
    }
}

void block_filter_chain::update_sub_region(Range3Di new_bounds) {
  bounds = new_bounds;
}
