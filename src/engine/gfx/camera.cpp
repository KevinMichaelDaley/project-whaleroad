#include "camera.h"
#include "Magnum/Math/Frustum.h"
#include "Magnum/Math/Intersection.h"
#include "Magnum/Math/Matrix3.h"
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
  Magnum::Math::Rad<float> fovr{(float)(fov * M_PI / 180.0f)};
  projection =
      Matrix4::perspectiveProjection(fovr, aspect, near_clip, far_clip);
}
void camera::set_ortho(Vector2 new_scale, float new_near_clip,
                       float new_far_clip) {

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
  Vector3 fwd = eye_direction.normalized();
  Vector3 side = cross(fwd, up.normalized());
  up = cross(side, fwd);
  Matrix4 m1{Vector4{side, 0}, Vector4{up, 0}, Vector4{-fwd, 0},
             Vector4{0, 0, 0, 1}};
  view = (m1.transposed()) * Matrix4::translation(-eye_position);
}
void camera::fps(Vector3 eye, float pitch, float yaw) {
  // I assume the values are already converted to radians.
  float cosPitch = cos(pitch);
  float sinPitch = sin(pitch);
  float cosYaw = cos(yaw);
  float sinYaw = sin(yaw);

  Vector3 xaxis = Vector3{cosYaw, -sinYaw, 0};
  Vector3 yaxis = Vector3{sinYaw * sinPitch, cosYaw * sinPitch, cosPitch};
  Vector3 zaxis = Vector3{sinYaw * cosPitch, cosPitch * cosYaw, -sinPitch};

  // Create a 4x4 view matrix from the right, up, forward and eye position
  // vectors
  view =
      Matrix4{Vector4(xaxis.x(), yaxis.x(), zaxis.x(), 0),
              Vector4(xaxis.y(), yaxis.y(), zaxis.y(), 0),
              Vector4(xaxis.z(), yaxis.z(), zaxis.z(), 0), Vector4(0, 0, 0, 1)};
  view = view * Matrix4::translation(-eye);
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
    : view(Matrix4{}), perspective(true), fov(100), aspect(21.0 / 9.0),
      near_clip(0.1), far_clip(256.0) {
  if (perspective) {
    set_perspective(fov, aspect, near_clip, far_clip);
  } else {
    set_ortho(fov * Vector2(aspect, 1), near_clip, far_clip);
  }
}
Vector4 camera::screen_to_world(Vector4 u) {

  Matrix4 viewprojinv = (projection * view).inverted();
  Vector4 dir = (viewprojinv * Vector4{u.x(), u.y(), 1.0, 0.0}).normalized();
  Vector4 v = view * dir;
  float d = (std::max(u.z() - near_clip, 0.0f) / v.z());
  Vector4 v2 = viewprojinv * (Vector4{u.x(), u.y(), 0.0, 1.0}) + d * dir;
  return v2 / v2.w();
}

bool camera::frustum_cull_box(Range3D box) {

  Magnum::Frustum f = Magnum::Frustum::fromMatrix(projection * view);

  if (Magnum::Math::Intersection::rangeFrustum(box, f)) {
    return true;
  }
  if (Magnum::Math::Intersection::pointFrustum(box.backBottomLeft(), f))
    return true;
  return false;
}
