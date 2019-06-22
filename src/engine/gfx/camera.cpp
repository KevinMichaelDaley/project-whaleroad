#include "camera.h"

#include "Magnum/Math/Frustum.h"
#include "Magnum/Math/Intersection.h"
#include <cassert>
void camera::set_perspective(float new_fov, float new_aspect,
                             float new_near_clip, float new_far_clip) {
  if (new_fov < 0) {
    assert(perspective);
    new_fov = fov;
  }
  if (new_aspect < 0) {
    if (perspective)
      new_aspect = aspect;
    else {
      new_aspect = scale.x() / scale.y();
    }
  }
  if (new_near_clip < 0) {
    new_near_clip = near_clip;
  }
  if (new_far_clip < 0) {
    new_far_clip = far_clip;
  }
  perspective = true;
  fov = new_fov;
  far_clip = new_far_clip;
  near_clip = new_near_clip;
  aspect = new_aspect;
  Magnum::Math::Rad<float> fovr{(float)(fov*M_PI/180.0f)};
  projection = Matrix4::perspectiveProjection(fovr, aspect, near_clip, far_clip);
}
void camera::set_ortho(Vector2 new_scale, float new_near_clip, float new_far_clip) {

  if (new_near_clip < 0) {
    new_near_clip = near_clip;
  }
  if (new_far_clip < 0) {
    new_far_clip = far_clip;
  }
  if (new_scale.dot() == 0) {
    assert(perspective);
    new_scale = scale;
  }

  far_clip = new_far_clip;
  near_clip = new_near_clip;
  scale = new_scale;
  projection = Matrix4::orthographicProjection(scale, near_clip, far_clip);
}
void camera::look_at(Vector3 eye_position, Vector3 eye_direction, Vector3 up) {
  view = Matrix4::lookAt(eye_position, eye_position + eye_direction, up)
             .invertedRigid();
}
/*
std::array<plane_t, 6> camera::frustum() {
  Matrix4 device_to_world = (projection * view).inverted();
  Vector3 plane_normals[6] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                              {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
  std::array<plane_t, 6> ans = {{{}, 0}};
  for (int i = 0; i < 6; ++i) {
    Vector3 world_n = device_to_world.transformDirection(plane_normals[i]);
    Vector3 world_p = device_to_world.transformPoint(plane_normals[i]);
    float d = -dot(world_p, world_n);
    ans[i].n = world_n;
    ans[i].d = d;
  }
}*/
camera::camera()
    : view(Matrix4{}), perspective(true), fov(100),
      aspect(21.0 / 9.0), near_clip(0.1), far_clip(100.0) {
if(perspective){
  set_perspective(fov, aspect, near_clip, far_clip);
}
else{
    set_ortho(fov*Vector2(aspect,1),near_clip,far_clip);
}
}

 bool camera::frustum_cull_box(Range3D box){
            
        Magnum::Frustum f=Magnum::Frustum::fromMatrix(projection*view);
        if(Magnum::Math::Intersection::rangeFrustum(box,f)){
            return true;
        }
        if(Magnum::Math::Intersection::pointFrustum(box.backBottomLeft(),f)) return true;
        return false;
 }
