#pragma once
#include "camera.h"
#include "drawable.h"
#include "vox/block.h"
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
using namespace Magnum;
class world;

const float FacesUV[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};

const float FacesOffset[6][4][3] =
    {    // offset of every block vertex from the bottom-left-back corner.
         // indexable by face, vertex modulo face, and component.
         // makes our meshgen code below muuuch tidier!
        {// bottom
         {0, 0, 0},
         {1, 0, 0},
         {0, 1, 0},
         {1, 1, 0}},

        {// top
         {0, 0, 1},
         {1, 0, 1},
         {0, 1, 1},
         {1, 1, 1}},

        {// back
         {0, 0, 0},
         {0, 0, 1},
         {1, 0, 0},
         {1, 0, 1}},

        {// front
         {0, 1, 0},
         {0, 1, 1},
         {1, 1, 0},
         {1, 1, 1}},

        {// left
         {0, 0, 0},
         {0, 0, 1},
         {0, 1, 0},
         {0, 1, 1}},

        {// right
         {1, 0, 0},
         {1, 0, 1},
         {1, 1, 0},
         {1, 1, 1}}};

const float FacesNormal[6][3] = {{0, 0, -1}, {0, 0, 1},  // bottom, top
                                 {0, -1, 0}, {0, 1, 0},  // back, front
                                 {-1, 0, 0}, {1, 0, 0}}; // left, right
class chunk_mesh : public drawable {
  bool force_dirty;
  size_t vbo_sz;
  int Nverts;
  bool changed;
  typedef GL::Attribute<0, UnsignedInt> L1;
  typedef GL::Attribute<1, UnsignedInt> L2;
  typedef GL::Attribute<2, UnsignedInt> L3;
  typedef GL::Attribute<3, Vector3> pos;
  typedef GL::Attribute<4, Vector2> uv;
  typedef GL::Attribute<5, UnsignedInt> f1;
  typedef GL::Attribute<6, UnsignedInt> f2;
  typedef GL::Attribute<7, UnsignedInt> f3;

public:
  struct BVertex {
    uint32_t L1;
    uint32_t L2;
    uint32_t L3;
  };

protected:
  BVertex verts[65536];
  GL::Buffer vertexBufferCube, vertexBuffer, indexBufferCube;
  GL::Mesh mesh_points;
  GL::Mesh mesh;

public:
  int x0, y0;

public:
  bool is_dirty(world* w);
  void update(world* w);
  void gen(world* w);
  void gen_column(int x, int y, world* w);
  bool gen_instance(int x, int y, int z, block_t b, int *Lf);
  void copy_back();
  void start_at(int x, int y);
  chunk_mesh();
  GL::Mesh &get_mesh() { return mesh; }

  GL::Mesh &get_point_mesh() { return mesh_points; }
  void force_change() { force_dirty = true; }
  void copy_to_gpu();
  virtual bool is_visible(camera *cam) {
    return (Nverts > 0);
  }

  virtual void draw(GL::AbstractShaderProgram* program){
    mesh.draw(*program);
  }
};
