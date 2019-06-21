#include "chunk_mesh.h"
#include "vox/world.h"
#include <cstdlib>
#include <cstring>
#include <mutex>
bool chunk_mesh::gen_instance(int x, int y, int z, block_t b,
                              int *Lf) {

  chunk_mesh::BVertex v = {0};
  uint32_t x2 = (unsigned int)(x);
  uint32_t y2 = (unsigned int)(y);
  uint32_t z2 = (unsigned int)(z);
  uint8_t xydiff = x2 * 16 + y2;
  uint32_t which = (unsigned int)std::abs(b);
  unsigned char L1=(uint8_t)Lf[0];
  unsigned char L2=(uint8_t)Lf[1];
  unsigned char L3=(uint8_t)Lf[2];
  unsigned char L4=(uint8_t)Lf[3];
  unsigned char L5=(uint8_t)Lf[4];
  unsigned char L6=(uint8_t)Lf[5];
  unsigned char L7=(uint8_t)Lf[6];
  unsigned char L8=(uint8_t)Lf[7];
  unsigned char L9=(uint8_t)Lf[8];
  unsigned char L10=(uint8_t)Lf[9];
  unsigned char L11=(uint8_t)Lf[10];
  unsigned char L12=(uint8_t)Lf[11];
  uint32_t L1u = uint32_t(z2%65536) +
                (uint32_t(xydiff) << 16uL) + (uint32_t(which%256) << 24uL);
  uint32_t L2u = uint64_t(which/256) + (uint64_t(L1+(L2<<4uL)+(L3<<8uL)+(L4<<12uL)+(L5<<16uL)+(L6<<20uL))<<8uL);
  uint32_t L3u = uint64_t(which/256) + (uint64_t(L1+(L7<<4uL)+(L8<<8uL)+(L9<<12uL)+(L10<<16uL)+(L11<<20uL))<<8uL);
  v.L1 = L1u;
  v.L2 = L2u;
  v.L3 = L3u;
  verts[Nverts++] = v;      
  return true;
}
void chunk_mesh::gen_column(int x, int y, world *wld) {
  uint16_t skip_invisible_array[constants::WORLD_HEIGHT] = {0};
  int zmax = wld->get_z(x + x0, y + y0, skip_invisible_array);
  bool valid = false;
  block_t &b = wld->get_voxel(x + x0, y + y0, 0, valid);
  
  for (int z = 0; z <= (int) std::min(unsigned(zmax), unsigned(constants::WORLD_HEIGHT-1));
       z += std::max(int(skip_invisible_array[z]), 1)) {
    block_t b2 = (&b)[z];

    if (!b2 && z > (int) wld->ocean_level)
      break;
    if (!block_is_visible(b2)) {
      continue;
    }
    int L[12]={0x0};
    int bt[6]={0x0};
    for(int m=0; m<6; ++m){
        
        int dx=FacesNormal[m%6][0];
        int dy=FacesNormal[m%6][1];
        int dz=FacesNormal[m%6][2];
        if(z+dz<0 || z+dz>constants::WORLD_HEIGHT-1){
            L[m%6]=255;
        }
        block_t b2=wld->get_voxel(x+x0+dx,y+y0+dy,z+dz);
        if(b2>1){
            continue;
        }
        if(b2<0){
            continue;
        }
        bt[m]=1;
        uint8_t* L1=wld->get_light(x+x0+dx,y+y0+dy,z+dz);
        ;
        float L2=(float)L1[m]+L1[m+6]/64.0;
        L[m]=(int)std::min(L2,255.0f);
        L[m+6]=(int) L1[m+12];
    }   
    
    int Ltop=0;
    int N=0;
    for(int m=1; m<6; ++m){
        Ltop+=L[m];
        N+=bt[m];
    }
    
    for(int m=1; m<6; ++m){
        L[m]=std::min(127,Ltop/(2*std::max(N,1)));
    }
    
    
    gen_instance((x), (y), (z), b2, L);
  }
}

void chunk_mesh::gen(world *wld) {
  Nverts = 0;
  for (int x = 0; x < 16; ++x) {
    for (int y = 0; y < 16; ++y) {
      gen_column(x, y, wld);
      changed = true;
    }
  }
}

bool chunk_mesh::is_dirty(world *wld) {
  if (force_dirty) {
    force_dirty = false;
    return true;
  }
  world_page* isle = wld->get_page(x0,y0,0);
  if (isle->dirty_list == nullptr) {
    return false;
  }
  bool o = false;
  for (int i = -1; i <= (int)constants::CHUNK_WIDTH / 8; ++i) {
    for (int j = -1; j <= (int)constants::CHUNK_WIDTH / 4; ++j) {
      int x = std::min((uint64_t)isle->dim / 8 - 1,
                       std::max((uint64_t)(x0 - isle->x0) / 8 + i, (uint64_t)0));
      int y = std::min((uint64_t)isle->dim / 4 - 1,
                       std::max((uint64_t)(y0 - isle->y0) / 4 + j, (uint64_t)0));
      if (!isle->contains(x * 8, y * 4, 0)) {
        isle = wld->get_page(x*8,y*4,0);
      }
      if (isle->has_changes_in_8x4_block(x*8,y*4)[0] != 0) {
        if (i >= 0 && i < (int) constants::CHUNK_WIDTH && j >= 0 && j < (int) constants::CHUNK_WIDTH)
          isle->has_changes_in_8x4_block(x*8,y*4)[0] ^= (uint64_t)1u << (uint64_t)32u;
           o = true;
      }
    }
  }
  return o;
}
void chunk_mesh::update(world* w) { gen(w); }
struct CubeVertex {
  float x, y, z;
  float u, v;
  unsigned int face_index1, faceindex2, faceindex3;
};
chunk_mesh::chunk_mesh() : vbo_sz(0) {
  mesh_points = GL::Mesh{};
  mesh = GL::Mesh{};
  std::array<uint8_t, 6 * 2 * 3> indices;
  std::array<CubeVertex, 6 * 4> vertices;
  int max_index = 0;
  int max_vertex = 0;
  const float whichface[6] = {2, 1, 0, 0, 0, 0};
  int face3[6][4];
  int face2[6][4];
  int mask[6]={0};
  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {
        face2[face][vert]=-1;
        face3[face][vert]=-1;
        int vx=(FacesOffset[face][vert][0]>0);
        int vy=(FacesOffset[face][vert][1]>0);
        int vz=(FacesOffset[face][vert][2]>0);
        mask[face]|=1<<(int)(vx*4+vy*2+vz);
    }
  }
  
  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {
        
        int vx=(FacesOffset[face][vert][0]>0);
        int vy=(FacesOffset[face][vert][1]>0);
        int vz=(FacesOffset[face][vert][2]>0);
        int vmask=1<<(int)(vx*4+vy*2+vz);
        for (int face1 = 0; face1 < 6; ++face1) {
            if(face1!=face && (mask[face1]&vmask!=0)){
                face2[face][vert]=face1;
                break;
            }
        }
    }
  }
  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {
        
        int vx=(FacesOffset[face][vert][0]>0);
        int vy=(FacesOffset[face][vert][1]>0);
        int vz=(FacesOffset[face][vert][2]>0);
        int vmask=1<<(int)(vx*4+vy*2+vz);
        for (int face1 = 0; face1 < 6; ++face1) {
            if(face1!=face && face1!=face2[face][vert] && (mask[face1]&vmask!=0)){
                face3[face][vert]=face1;
                break;
            }
        }
    }
  }
                
  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {
        
        assert(face2[face][vert]>=0 && face3[face][vert]>=0);
          
      vertices[max_vertex++] = {(float)FacesOffset[face][vert][0],
                                (float)FacesOffset[face][vert][1],
                                (float)FacesOffset[face][vert][2],
                                (float)FacesUV[vert][0] / 256.0f,
                                (float)FacesUV[vert][1] / 3.0f +
                                    whichface[face] / 3.0f,
                               (unsigned)face,(unsigned)face2[face][vert],(unsigned)face3[face][vert]};
    }
    indices[max_index++] = max_vertex - 3;
    indices[max_index++] = max_vertex - 1;
    indices[max_index++] = max_vertex - 4;
    indices[max_index++] = max_vertex - 2;
    indices[max_index++] = max_vertex - 4;
    indices[max_index++] = max_vertex - 1;
  }
  vertexBufferCube.setData(vertices, GL::BufferUsage::StaticDraw);
  indexBufferCube.setData(indices, GL::BufferUsage::StaticDraw);
  mesh.setPrimitive(GL::MeshPrimitive::Triangles)
      .addVertexBufferInstanced(vertexBuffer, 1, 0, L1{}, L2{},L3{})
      .addVertexBuffer((vertexBufferCube), 0, pos{}, uv{}, f1{},f2{},f3{})
      .setIndexBuffer((indexBufferCube), 0, GL::MeshIndexType::UnsignedByte)
      .setInstanceCount(0)
      .setCount(max_index);

  mesh_points.setPrimitive(GL::MeshPrimitive::Points)
      .addVertexBuffer(vertexBuffer, 0, L1{}, L2{}, L3{})
      .setCount(0);
  changed = false;
  force_dirty=false;
  Nverts = 0;
  vbo_sz = 0;
}
void chunk_mesh::start_at(int x_, int y_) {
  x0 = x_;
  y0 = y_;
}
void chunk_mesh::copy_to_gpu() {
  if (!changed)
    return;
  if (vbo_sz == 0) {
    vertexBuffer.setData({(const void *)&verts[0], 65536 * sizeof(BVertex)},
                         GL::BufferUsage::DynamicDraw);
    vbo_sz = 256 * 256 * sizeof(BVertex);
  } else {
    vertexBuffer.setSubData(
        0, {(const void *)&verts[0], 256 * 256 * sizeof(BVertex)});
  }
  mesh_points.setCount(Nverts);
  mesh.setInstanceCount(Nverts);

  changed = false;
}

