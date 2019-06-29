#include "block_iterator.h"
#include "world.h"
#include <iostream>
#include <sstream>
#include "Magnum/GL/Renderer.h"
void MAGNUM_VERIFY_NO_GL_ERROR(const char* file, int line){
    int err=(int)Magnum::GL::Renderer::error();
    if(err != (int)Magnum::GL::Renderer::Error::NoError){
       std::cerr<<"gl error:" <<err<<" : "<<std::string(file)<< " line "<< line<<"\n";
    }
}
    
void block_iterator::iter_columns(world *wld, Magnum::Range3Di bounds, block_iterator::column_operation op, 
                                      int size_neighborhood,
                                      bool diff_only) {
  Vector3i bbL = bounds.backBottomLeft();
  Vector3i ftR = bounds.frontTopRight();
  int x0 = bbL.x(); int x1 = std::max(int(ftR.x()), x0 + 1);
  int y0 = bbL.x(); int y1 = std::max(int(ftR.y()), y0 + 1);
  
  for (int x = x0; x < x1; x+=8) {
    for (int y = y0; y < y1; y+=4) {
      world_page *page = wld->get_page(x, y, 0);
      bool valid = false;   
      int sx,sy,sz,dim;
      page->bounds(sx,sy,sz,dim);
      block_t &bref = *((block_t*) &(page->get(x, y, 0, valid)));
      if(!valid){
          continue;
      }
            
        uint16_t neighborhood[( size_neighborhood+8)*( size_neighborhood+4)*(int)constants::WORLD_HEIGHT];
        uint16_t z_neighbors[( size_neighborhood+8)*( size_neighborhood+4)];
      // build the union of the neighborhoods of every column in the 8x4
      // subchunk
            if(size_neighborhood>1){
                int Nx=size_neighborhood+8;
                int Ny=size_neighborhood+4;
                for(int xb=- size_neighborhood/2; xb< size_neighborhood/2+8; ++xb){
                    for(int yb=- size_neighborhood/2; yb< size_neighborhood/2+4; ++yb){
                        int index_nb= (xb+size_neighborhood/2)*Ny+(yb+size_neighborhood/2);
                        z_neighbors[index_nb]=wld->get_z(xb+x,yb+y, &neighborhood[index_nb*(int) constants::WORLD_HEIGHT],page);
                    }
                }
            }
            uint64_t mask = page->has_changes_in_8x4_block(x, y)[0];
            if (diff_only && mask == 0u) {
              continue;
            }
            
      // now iterate over the subchunk and apply the operation
            for (int bit = 0; bit < 32; ++bit) {
                if (diff_only && ((mask & bit) == 0)) {
                    continue;
                }
                int xb=bit/4;
                int yb=bit%4;

                int column_index = (int) constants::WORLD_HEIGHT * ((x-sx+xb) * (int) dim + (y-sy+yb));

                block_t *columns = &((&bref)[column_index]);
                int index_nb=((xb+size_neighborhood/2) * (4+size_neighborhood) + yb+(size_neighborhood/2));
                op(columns, &(neighborhood[(int)constants::WORLD_HEIGHT*index_nb]), &(z_neighbors[index_nb]), x+xb, y+yb, x+xb,y+yb, page, wld);
            }
            
      }
    }
}

