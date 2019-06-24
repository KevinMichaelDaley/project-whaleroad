#pragma once
#include "common/constants.h"
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
class chunk_mesh {
  friend class block_default_forward_pass;
  bool force_dirty, dirty;
  size_t vbo_sz;
  int Nverts[constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT];
  bool changed[constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT];
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
  BVertex verts[constants::WORLD_HEIGHT*constants::CHUNK_WIDTH*constants::CHUNK_WIDTH];
  GL::Buffer vertexBufferCube, vertexBuffer, indexBufferCube;
  GL::Mesh mesh[constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT];

public:
  int x0, y0;
  int volume;
public:
  bool is_dirty(world* w);
  void update(world* w);
  void gen(world* w);
  void gen_column(int x, int y, world* w);
  bool gen_instance(int x, int y, int z, block_t b, int *Lf);
  void copy_back();
  void start_at(int x, int y);
  chunk_mesh();
  GL::Mesh &get_mesh(int z) { return mesh[z]; }
  void force_change() { force_dirty = true; }
  void copy_to_gpu(int z);
  bool is_visible(camera cam, int z0) {
    return (Nverts[z0] > 0) && cam.frustum_cull_box(Range3D{{x0,y0,z0*constants::CHUNK_HEIGHT},{x0+constants::CHUNK_WIDTH,y0+constants::CHUNK_WIDTH, z0*constants::CHUNK_HEIGHT+constants::CHUNK_HEIGHT}});
  }
  float min_depth(camera cam){
      float zmin=1000000000;
      Vector4 corner1=Vector4{constants::CHUNK_WIDTH+x0, constants::CHUNK_WIDTH+y0, constants::WORLD_HEIGHT,1.0};
      Vector4 corner2=Vector4{x0,y0,0,1.0};
      Vector4 corner_p=((cam.projection*cam.view)*corner1);
      zmin=std::min(zmin,corner_p.z()/corner_p.w()*0.5f+0.5f);
      corner_p=((cam.projection*cam.view)*corner2);
      zmin=std::min(zmin,corner_p.z()/corner_p.w()*0.5f+0.5f);
      return zmin;
  }
  void draw(GL::AbstractShaderProgram* program, int z){
    mesh[z].draw(*program);
  }
};
