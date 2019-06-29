#include "asset_manager.h"
#include <Magnum/GL/Mesh.h>
class prop : public drawable {
private:
  GL::Mesh *mesh;
  Rig *rig;
  GL::Texture2D *tex;
  Range3D aabb;
  Vector3D pos;
  Rotation dir;
  Matrix4 world_matrix;
  double animation_start;
  int animation_state;

public:
  prop(std::string mesh_name) {
    animation_start = 0;
    animation_state = 0;
    mesh = asset_manager::get_mesh(mesh_name + ".mesh", aabb);
    tex = asset_manager::get_texture(mesh_name + ".tga");
    rig = asset_manager::get_rig(mesh_name + ".rig");
  }
  virtual Range3D getAABB() {
    Vector3 a[8];
    a[0] = world_matrix * aabb.backBottomLeft();
    a[1] = world_matrix * aabb.frontBottomLeft();
    a[2] = world_matrix * aabb.backTopLeft();
    a[3] = world_matrix * aabb.frontTopLeft();

    a[4] = world_matrix * aabb.backBottomRight();
    a[5] = world_matrix * aabb.frontBottomRight();
    a[6] = world_matrix * aabb.backTopRight();
    a[7] = world_matrix * aabb.frontTopRight();
    float fmin = std::numeric_limit<float>::min();
    float fmax = std::numeric_limit<float>::max();
    Vector3 mn = Vector3D{fmax, fmax, fmax};
    Vector3 mx = Vector3D{fmin, fmin, fmin};
    for (int i = 0; i < 8; ++i) {
      mx = Math::max(mx, a[i]);
      mn = Math::max(mn, a[i]);
    }
    return Range2D{mn, mx};
  }
  Matrix4 get_transform() { return world_matrix; }
  void set_identity() { world_matrix = Matrix4{}; }
  void rotate(Vector3 axis, float angle) {
    world_matrix = Matrix4::rotation(Magnum::Rad(angle), axis) * world_matrix;
  }

  void translate(Vector3 offset) {
    world_matrix = Matrix4::translation(offset) * world_matrix;
  }
  void rotate_towards(Vector3 fwd, Vector3 up = Vector3{0, 0, 1}) {
    world_matrix = Matrix4::lookAt({0, 0, 0}, fwd, up);
  }

  virtual bool is_visible(camera *cam) {
    return cam->frustum_cull_box(getAABB());
  }
  virtual int set_animation_state(int state) {
    animation_start = timer::now();
    animation_state = state;
  }

  virtual void draw(shader *program, camera cam) {
    program->uniform("modelviewproj",
                     cam.projection * cam.view * get_transform());
    program->uniform("texture", texture);
    if (rig != nullptr) {
      float offset_time = timer::now() - animation_start;
      for (int i = 0, e = rig.get_num_bones(); i < e; ++i) {
        std::stringstream ss;
        ss << i;
                program->uniform("bone_matrix["+ss.str()+"]",rig.get_bone_matrix(animation_state,offset_time,i);
      }
    }

    mesh->draw(program);
  }
};
