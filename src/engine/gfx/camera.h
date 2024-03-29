#pragma once
#include "Magnum/Magnum.h"
#include "Magnum/Math/Matrix4.h"
#include <algorithm>
using namespace Magnum;
struct camera {
  Matrix4 view;
  bool perspective;
  Matrix4 projection;
  float fov;
  float aspect;
  Vector2 scale;
  float far_clip;
  float near_clip;
  void set_perspective(float new_fov, float new_aspect, float new_near_clip,
                       float new_far_clip);
  void set_ortho(Vector2 new_scale, float near_clip, float far_clip);
  void look_at(Vector3 eye_position, Vector3 eye_direction, Vector3 up);
  bool frustum_cull_box(Range3D box);
  camera();
  void fps(Vector3 eye, float pitch, float yaw);
  Vector4 screen_to_world(Vector4 u);
};
