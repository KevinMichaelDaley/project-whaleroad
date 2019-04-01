#include "chunk_mesh.h"
#include "vox/world.h"
#include <cstdlib>
#include <cstring>
#include <mutex>
bool chunk_mesh::gen_instance(int x, int y, int z, block_t b,
                              unsigned char *Lf) {

  chunk_mesh::BVertex v = {0, 0, 0};
  uint32_t x2 = (unsigned int)(x);
  uint32_t y2 = (unsigned int)(y);
  uint32_t z2 = (unsigned int)(z);
  uint32_t xydiff = x2 * 16 + y2;
  uint32_t which = (unsigned int)std::abs(b);
  uint32_t L1 = uint32_t(Lf[0]) + (uint32_t(Lf[1]) << 8uL) +
                (uint32_t(Lf[2]) << 16uL) + (uint32_t(Lf[3]) << 24uL);
  uint32_t L2 = uint32_t(Lf[4]) + (uint32_t(Lf[5]) << 8uL) +
                (uint32_t(Lf[6]) << 16uL) + (uint32_t(Lf[7]) << 24uL);
  uint32_t L3 = uint32_t(Lf[8]) + (uint32_t(z2) << 8uL) +
                (uint32_t(xydiff) << 16uL) + (uint32_t(which) << 24uL);

  v.L1 = L1;
  v.L2 = L2;
  v.L3 = L3;
  verts[Nverts++] = v;
  return true;
}
void chunk_mesh::gen_column(int x, int y, world *wld) {
  unsigned char *L = wld->get_light(x + x0, y + y0, 0);
  uint16_t skip_invisible_array[constants::WORLD_HEIGHT] = {0};
  int zmax = wld->get_z(x + x0, y + y0, skip_invisible_array);
  unsigned z;
  bool valid = false;
  block_t &b = wld->get_voxel(x + x0, y + y0, 0, valid);
  if (!valid) {
    return;
  }
  for (z = 0; z <= std::min(zmax, 255);
       z += std::max(int(skip_invisible_array[z]), 1)) {
    block_t b2 = (&b)[z];

    if (!b2 && z > wld->ocean_level + 2)
      break;
    if (!block_is_visible(b2)) {
      continue;
    }

    gen_instance((x), (y), (z), b2, &(L[(z)*9]));
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
  world_page& isle = wld->get_page(std::floor(x0/1024.0f)*1024,std::floor(y0/1024.0f)*1024,0);
  if (isle.dirty_list == nullptr) {
    return false;
  }
  bool o = false;
  for (int i = -1; i <= constants::CHUNK_WIDTH / 8; ++i) {
    for (int j = -1; j <= constants::CHUNK_WIDTH / 4; ++j) {
      int x = std::min((uint64_t)isle.dim / 8 - 1,
                       std::max((uint64_t)(x0 - isle.x0) / 8 + i, (uint64_t)0));
      int y = std::min((uint64_t)isle.dim / 4 - 1,
                       std::max((uint64_t)(y0 - isle.y0) / 4 + j, (uint64_t)0));
      if (!isle.contains(x * 8, y * 4, 0)) {
        continue;
      }
      if ((isle.dirty_list[x * isle.dim / 4 + y]) != 0) {
        if (i >= 0 && i < constants::CHUNK_WIDTH && j >= 0 && j < constants::CHUNK_WIDTH)
          isle.dirty_list[x * isle.dim / 4 + y] ^= (uint64_t)1 << (uint64_t)32;
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
  float nmlx, nmly, nmlz;
};
chunk_mesh::chunk_mesh() : vbo_sz(0) {
  mesh_points = GL::Mesh{};
  mesh = GL::Mesh{};
  std::array<uint8_t, 6 * 2 * 3> indices;
  std::array<CubeVertex, 6 * 4> vertices;
  int max_index = 0;
  int max_vertex = 0;
  const float whichface[6] = {2, 1, 0, 0, 0, 0};
  for (int face = 0; face < 6; ++face) {
    for (int vert = 0; vert < 4; ++vert) {

      vertices[max_vertex++] = {(float)FacesOffset[face][vert][0],
                                (float)FacesOffset[face][vert][1],
                                (float)FacesOffset[face][vert][2],
                                (float)FacesUV[vert][0] / 256.0f,
                                (float)FacesUV[vert][1] / 3.0f +
                                    whichface[face] / 3.0f,
                                (float)FacesNormal[face][0],
                                (float)FacesNormal[face][1],
                                (float)FacesNormal[face][2]};
    }
    indices[max_index++] = max_vertex - 4;
    indices[max_index++] = max_vertex - 3;
    indices[max_index++] = max_vertex - 2;
    indices[max_index++] = max_vertex - 3;
    indices[max_index++] = max_vertex - 2;
    indices[max_index++] = max_vertex - 1;
  }
  vertexBufferCube.setData(vertices, GL::BufferUsage::StaticDraw);
  indexBufferCube.setData(indices, GL::BufferUsage::StaticDraw);
  mesh.setPrimitive(GL::MeshPrimitive::Triangles)
      .addVertexBufferInstanced(vertexBuffer, 1, 0, L1{}, L2{}, L3{})
      .addVertexBuffer((vertexBufferCube), 0, pos{}, uv{}, nml{})
      .setIndexBuffer((indexBufferCube), 0, GL::MeshIndexType::UnsignedByte)
      .setInstanceCount(0)
      .setCount(max_index);

  mesh_points.setPrimitive(GL::MeshPrimitive::Points)
      .addVertexBuffer(vertexBuffer, 0, L1{}, L2{}, L3{})
      .setCount(0);
  changed = false;
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

