#include "chunk_mesh.h"
#include "vox/world.h"
#include <cstdlib>
#include <cstring>
#include <mutex>
bool chunk_mesh::gen_instance(int x, int y, int z, block_t b, int *Lf) {

  chunk_mesh::BVertex v = {0};
  uint32_t x2 = (unsigned int)(x);
  uint32_t y2 = (unsigned int)(y);
  uint32_t z2 = (unsigned int)(z);
  uint8_t xydiff = x2 * 16 + y2;
  uint32_t which = (unsigned int)std::abs(b);
  unsigned char L1 = (uint8_t)Lf[0];
  unsigned char L2 = (uint8_t)Lf[1];
  unsigned char L3 = (uint8_t)Lf[2];
  unsigned char L4 = (uint8_t)Lf[3];
  unsigned char L5 = (uint8_t)Lf[4];
  unsigned char L6 = (uint8_t)Lf[5];
  uint32_t L1u = uint32_t(z2 % constants::CHUNK_HEIGHT) +
                 (uint32_t(xydiff) << 6uL) + (uint32_t(which % 4096) << 14uL) +
                 ((L6 / 4) << 24uL);
  uint32_t L2u = (uint32_t(L1 / 4 + ((L2 / 4) << 6uL) + ((L3 / 4) << 12uL) +
                           ((L4 / 4) << 18uL) + ((L5 / 4) << 24uL)));
  v.L1 = L1u;
  v.L2 = L2u;
  int N =
      constants::CHUNK_HEIGHT * constants::CHUNK_WIDTH * constants::CHUNK_WIDTH;
  int z0 = z / constants::CHUNK_HEIGHT;
  verts[z0 * N + (Nverts2[z0]++)] = v;
  changed2[z0] = true;
  return true;
}
void chunk_mesh::gen_column(int x, int y, world *wld) {
  uint16_t skip_invisible_array[constants::WORLD_HEIGHT] = {0};
  int zmax = wld->get_z_unsafe(x + x0, y + y0, skip_invisible_array);
  bool valid = false;
  volatile block_t &b = wld->get_voxel(x + x0, y + y0, 0, valid);
  int Nz = zmax;
  for (int z = 0; z <= (int)std::min(unsigned(zmax),
                                     unsigned(constants::WORLD_HEIGHT - 1));
       z += std::max(int(skip_invisible_array[z]), 1)) {
    block_t b2 = __sync_fetch_and_add(&((&b)[z]), 0);

    if (!b2 && z > (int)wld->ocean_level)
      break;
    if (!block_is_opaque(b2) && b2 > 0) {
      Nz -= 1;
    }
    if (!block_is_visible(b2)) {
      continue;
    }
    int Lsum[6] = {0x0}, N[6] = {0x0};
    for (int dx1 = -2; dx1 <= 2; ++dx1) {
      for (int dy1 = -2; dy1 <= 2; ++dy1) {

        int dz1 = 0;
        for (int m = 0; m < 6; ++m) {

          int dx = FacesNormal[m % 6][0];
          int dy = FacesNormal[m % 6][1];
          int dz = FacesNormal[m % 6][2];
          if (z + dz < 0 || z + dz > constants::WORLD_HEIGHT - 1) {
            Lsum[m % 6] += 255;
            N[m] += 1;
            continue;
          }
          block_t b2 = wld->get_voxel(x + x0 + dx + dx1, y + y0 + dy + dy1,
                                      z + dz + dz1);
          if (b2 > 1) {
            continue;
          }
          if (b2 < 0) {
            continue;
          }
          uint8_t *L1 = wld->get_light(x + x0 + dx + dx1, y + y0 + dy + dy1,
                                       z + dz + dz1);
          ;
          Lsum[m] += std::min(255, int(L1[m]+L1[m + 6] / 6));
          N[m] += 1;
          // printf("%i", L[1]);
        }
      }
    }
    /*
    int Ltop=0;
    for(int m=1; m<6; ++m){
        Ltop=std::max(Ltop,L[m]);
    }
    */
    for (int m = 0; m < 6; ++m) {
      Lsum[m] /= std::max(N[m], 1);
    }

    volume += Nz;
    gen_instance((x), (y), (z), b2, Lsum);
  }
}
void chunk_mesh::gen(world *wld) {
  volume = 0;
  for (int i = 0; i < constants::WORLD_HEIGHT / constants::CHUNK_HEIGHT; ++i) {
    if (Nverts[i] == 0) {
      Nverts2[i]=0;
      continue;
    }
    Nverts2[i] = 0;
    changed2[i] = true;
  }
  for (int x = 0; x < constants::CHUNK_WIDTH; ++x) {
    for (int y = 0; y < constants::CHUNK_WIDTH; ++y) {
      gen_column(x, y, wld);
    }
  }
  for (int i = 0; i < constants::WORLD_HEIGHT / constants::CHUNK_HEIGHT; ++i) {
    Nverts[i]=Nverts2[i];
    changed[i]=changed2[i];
  }
}

bool chunk_mesh::is_dirty(world *wld) {
  if (force_dirty) {
    force_dirty = false;
    return true;
  }
  if (dirty) {
    return true;
  }
  world_page *isle = wld->get_page(x0, y0, 0);
  if (isle->dirty_list == nullptr) {
    return false;
  }
  bool o = false;
  for (int i = -1; i <= (int)constants::CHUNK_WIDTH / 8; ++i) {
    for (int j = -1; j <= (int)constants::CHUNK_WIDTH / 4; ++j) {
      int x =
          std::min((uint64_t)isle->dim / 8 - 1,
                   std::max((uint64_t)(x0 - isle->x0) / 8 + i, (uint64_t)0));
      int y =
          std::min((uint64_t)isle->dim / 4 - 1,
                   std::max((uint64_t)(y0 - isle->y0) / 4 + j, (uint64_t)0));
      if (!isle->contains(x * 8, y * 4, 0)) {
        isle = wld->get_page(x * 8, y * 4, 0);
      }
      if (isle->has_changes_in_8x4_block(x * 8, y * 4)[0] != 0) {
        if (i >= 0 && i < (int)constants::CHUNK_WIDTH && j >= 0 &&
            j < (int)constants::CHUNK_WIDTH) {
          isle->has_changes_in_8x4_block(x * 8, y * 4)[0] ^= (uint64_t)1u
                                                             << (uint64_t)32u;
          dirty = true;
        }
      }
    }
  }
  return dirty;
}
void chunk_mesh::update(world *w) {
  std::lock_guard<std::mutex> lock(ready_for_gpu_copy);
  gen(w);
  dirty = false;
  update_queued = false;
}
struct CubeVertex {
  float x, y, z;
  float u, v;
  unsigned int face_index1, faceindex2, faceindex3;
};
chunk_mesh::chunk_mesh() :  update_queued{false} {

  std::array<uint8_t, 6 * 2 * 3> indices;
  std::array<CubeVertex, 6 * 4> vertices;
  int max_index = 0;
  int max_vertex = 0;
  dirty = false;
  const float whichface[6] = {2, 1, 0, 0, 0, 0};
  int face3[6][4];
  int face2[6][4];
  int mask[6] = {0};
  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {
      face2[face][vert] = -1;
      face3[face][vert] = -1;
      int vx = (FacesOffset[face][vert][0] > 0);
      int vy = (FacesOffset[face][vert][1] > 0);
      int vz = (FacesOffset[face][vert][2] > 0);
      mask[face] |= 1 << (int)(vx * 4 + vy * 2 + vz);
    }
  }

  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {

      int vx = (FacesOffset[face][vert][0] > 0);
      int vy = (FacesOffset[face][vert][1] > 0);
      int vz = (FacesOffset[face][vert][2] > 0);
      int vmask = 1 << (int)(vx * 4 + vy * 2 + vz);
      for (int face1 = 0; face1 < 6; ++face1) {
        if (face1 != face && ((mask[face1] & vmask) == vmask)) {
          face2[face][vert] = face1;
          break;
        }
      }
    }
  }
  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {

      int vx = (FacesOffset[face][vert][0] > 0);
      int vy = (FacesOffset[face][vert][1] > 0);
      int vz = (FacesOffset[face][vert][2] > 0);
      int vmask = 1 << (int)(vx * 4 + vy * 2 + vz);
      for (int face1 = 0; face1 < 6; ++face1) {
        if (face1 != face && face1 != face2[face][vert] &&
            ((mask[face1] & vmask) == vmask)) {
          face3[face][vert] = face1;
          break;
        }
      }
    }
  }

  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {

      assert(face2[face][vert] >= 0 && face3[face][vert] >= 0);
      // printf("%i %i\n", face2[face][vert], face3[face][vert]);
      vertices[max_vertex++] = {(float)FacesOffset[face][vert][0],
                                (float)FacesOffset[face][vert][1],
                                (float)FacesOffset[face][vert][2],
                                (float)FacesUV[vert][0] / 256.0f,
                                (float)FacesUV[vert][1] / 3.0f +
                                    whichface[face] / 3.0f,
                                (unsigned)face,
                                std::max((unsigned)face2[face][vert], 1u),
                                std::max((unsigned)face3[face][vert], 1u)};
    }
    indices[max_index++] = max_vertex - 3;
    indices[max_index++] = max_vertex - 1;
    indices[max_index++] = max_vertex - 4;
    indices[max_index++] = max_vertex - 2;
    indices[max_index++] = max_vertex - 4;
    indices[max_index++] = max_vertex - 1;
  }
  for (int i = 0; i < constants::WORLD_HEIGHT / constants::CHUNK_HEIGHT; ++i) {
    mesh[i] = GL::Mesh{};
    Nverts[i] = 0;

    int offset = constants::CHUNK_HEIGHT * constants::CHUNK_WIDTH *
                 constants::CHUNK_WIDTH * i;
    vertexBufferCube.setData(vertices, GL::BufferUsage::StaticDraw);
    indexBufferCube.setData(indices, GL::BufferUsage::StaticDraw);
    mesh[i]
        .setPrimitive(GL::MeshPrimitive::Triangles)
        .addVertexBufferInstanced(vertexBuffer, 1, offset * sizeof(BVertex),
                                  L1{}, L2{})
        .addVertexBuffer((vertexBufferCube), 0, pos{}, uv{}, f1{}, f2{}, f3{})
        .setIndexBuffer((indexBufferCube), 0, GL::MeshIndexType::UnsignedByte)
        .setInstanceCount(0)
        .setCount(max_index);
    changed[i] = false;
    vbo_sz[i] = 0;
  }
  
  force_dirty = false;
}
void chunk_mesh::start_at(int x_, int y_) {
  x0 = x_;
  y0 = y_;
}
void chunk_mesh::copy_to_gpu(int z) {
    
  std::lock_guard<std::mutex> lock(ready_for_gpu_copy);
  int index = z;
  if (!changed[index])
    return;
  if (vbo_sz[index] < Nverts[index]) {
    vbo_sz[index] = Nverts[index];
    vertexBuffer.setData({(const void *)&verts[0], vbo_sz[index] * sizeof(BVertex)},
                         GL::BufferUsage::StaticDraw);

  } else {
    size_t N = constants::CHUNK_WIDTH * constants::CHUNK_WIDTH *
               constants::CHUNK_HEIGHT;
    
    vertexBuffer.setSubData(
        index * N * sizeof(BVertex),
        {(const void *)&(verts[index * N]), Nverts[index] * sizeof(BVertex)});
  }
  mesh[index].setInstanceCount(Nverts[index]);
  changed[index] = false;
}
