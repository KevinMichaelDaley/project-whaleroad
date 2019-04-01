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
  uint16_t *neighborhood;

  int sizex = 8 + z_neighborhood_size, sizey = 4 + z_neighborhood_size;
  if (z_neighborhood_size > 1) {
    neighborhood = new uint16_t[sizex * sizey * constants::WORLD_HEIGHT];
  }

  world_page* page = &(wld->get_page(x0, y0, 0));
  size_t n_columns_z=0;
  for (int x = x0 - z_neighborhood_size / 2; x <= x0 + z_neighborhood_size / 2;
       ++x) {
    for (int y = y0 - z_neighborhood_size / 2;
         y <= y0 + z_neighborhood_size / 2; ++y) {
      wld->get_z(x, y, &(neighborhood[n_columns_z * constants::WORLD_HEIGHT]), page);
      ++n_columns_z;
    }
  }

  for (int x = x0; x < x1;) {
    for (int y = x0; y < y1;) {

      world_page *page = &(wld->get_page(x, y, 0));
      
      bool valid = false;

      block_t &bref = page->get(x0, y0, 0, valid);

      assert(valid);
      int sx, sy, sz, dim; // get the intersection of the page AABB and the
                           // region spanned by the current physics controller
      page->bounds(sx, sy, sz, dim);
      int i1 = std::min(sx + dim, x1), j1 = std::min(sy + dim, y1),
          i0 = std::max(x0, sx), j0 = std::max(y0, sy);
      // build the union of the neighborhoods of every column in the 8x4
      // subchunk
      uint16_t z_neighbors[sizex * sizey] = {0};
      for (int i = -sizex / 2; i < sizex / 2; ++i) {
        for (int j = -sizey / 2; j < sizey / 2; ++j) {
          z_neighbors[i * sizey + j] =
              wld->get_z(x + i, y + j, &(neighborhood[(i * sizey + j) * constants::WORLD_HEIGHT]), page);
        }
      }
      // now iterate over the subchunk and apply the operation
      for (int i = i0; i < i1; i += 8) {
        for (int j = j0; j < j1; j += 4)   {

            uint32_t mask = page->has_changes_in_8x4_block(i, j) & 0xffffffffu;
            if (diff_only && mask == 0) {
              continue;
            }
            for (int bit = 0; bit < 32; ++bit) {
                if (diff_only && ((mask & bit) == 0)) {
                    continue;
                }
    

                int column_index = constants::WORLD_HEIGHT * ((i0 - sx) * dim + (i1 - sy));

                block_t *columns = &((&bref)[column_index]);

                op(columns, &(neighborhood[i * sizey + j]), &(z_neighbors[i * sizey + j]), i, j, x,y, page, wld);
            }
        }
        ++x;
        ++y;
        }
      }
    }
}

void block_filter_chain::update_sub_region(Range3Di new_bounds) {
  bounds = new_bounds;
}
