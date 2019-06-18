#include "common/timer.h"
#include "common/entity.h"
#include "vox/world.h"
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Vector3.h>
#ifndef __ANDROID__
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <algorithm>
#include <array>
#define _USE_MATH_DEFINES
#include <cmath>
#define M_PI 3.14159265358979323846f


class character : public entity {
  float health;
  bool can_climb;
  float x[3], xdot[3];
  float look_vector[3];
  float walk_vector[3];
  float jump_speed;
  float foot_positions[3];
  float eye_positions[3];
  bool on_water, on_land, stop_moving;
  float jump_start;
  int climb_dir;
  bool collide_with_entire_base;
  float terminal_velocity;
  float gravity_multiplier;
  float friction;
  float radius_x, radius_y, height;
  float fall_start;
  float fall_damage;
  float climb_speed;
  bool climbing;
  bool flying;
  bool jump_;
  bool under_water;
  bool first_frame;
  Vector3 up_vector;
  world* wld_;
public:
  world* get_world(){
      return wld_;
  }
  void dir(float& rx, float& ry, float& rz){
      rx=look_vector[0];
      ry=look_vector[1];
      rz=look_vector[2];
  }
  void eye(float& rx, float& ry, float& rz){
      rx=eye_positions[0]+x[0];
      ry=eye_positions[1]+x[1];
      rz=eye_positions[2]+x[2];
  }
  character(world* wld, float xx = 0, float yy = 0, float zz = 0, bool spawn_random = true,
            bool spawn_on_surface = true, float rx = 0.3, float ry = 0.3,
            float rz = 0.5, float eye_height = 0.35, float angle = 0){
    wld_=wld;
    first_frame = true;
    health = 1.0f;
    if (spawn_random) {
      xx = ((rand() % 10)) + 1000;
      yy = ((rand() % 10)) + 1000;
    }
    if (spawn_on_surface || spawn_random) {
      zz = std::max(std::max(get_world()->get_z(xx, yy) + 13,
                             get_world()->ocean_level + 13),
                    11);
    }
    set_position(xx, yy, zz);
    set_velocity(0.0f, 0.0f, 0.0f);
    collide_with_entire_base = true;
      look_vector[0] = cos(angle);
      look_vector[1] = sin(angle);
      look_vector[2] = 0.0f;
    terminal_velocity = 50.0f;
    friction = 0.9f;
    fall_start = now();
    can_climb = false;
    climbing = false;
    flying = false;
    on_land = true;
    on_water = false;
    stop_moving = false;
    radius_x = rx;
    radius_y = ry;
    height = rz;
    eye_positions[0] = 0;
    eye_positions[1] = 0;
    eye_positions[2] = eye_height;
    up_vector[0] = 0;
    up_vector[1] = 0;
    up_vector[2] = 1;
    climb_speed = 0.25f;
    fall_damage = 1.0f;
    gravity_multiplier = 0.2f;
    fall_start = now();
    jump_start = now();
    jump_ = false;
  }
  void get_eye_basis(Vector3& eye, Vector3& fwd, Vector3& up){
      eye=Vector3{eye_positions[0]+x[0], eye_positions[1]+x[1], eye_positions[2]+x[2]};
      fwd=Vector3{look_vector[0], look_vector[1], look_vector[2]};
      up=Vector3{up_vector[0], up_vector[1], up_vector[2]};
  }
  void set_position(float xx, float yy, float zz) {
    x[0] = xx;
    x[1] = yy;
    x[2] = zz;
  }
  Vector3 get_position(){
      return Vector3{x[0],x[1],x[2]};
  }
  void set_velocity(float dx, float dy, float dz) {
    xdot[0] = dx;
    xdot[1] = dy;
    xdot[2] = dz;
  }
  void add_impulse(float dx, float dy, float dz) {
    xdot[0] += dx;
    xdot[1] += dy;
    xdot[2] += dz;
  }
  void look(float xx, float yy, float zz) {
    look_vector[0] = xx;
    look_vector[1] = yy;
    look_vector[2] = zz;
  }
  void walk_dir(float dx, float dy, float dz) {
    walk_vector[0] = dx;
    walk_vector[1] = dy;
    walk_vector[2] = dz;
  }
  void walk_toward(float dx, float dy, float dz, float speed) {
    walk_vector[0] = (dx = x[0]);
    walk_vector[1] = (dy - x[1]);
    float d = (dx - x[0]) * (dx - x[0]) + (dy - x[1]) * (dy - x[1]);
    d = std::sqrt(d);
    walk_vector[0] *= speed / d;
    walk_vector[1] *= speed / d;
    walk_vector[2] = 0;
  }
  void set_climb_dir(float dir) { climb_dir = dir; }
  float signed_distance_to(float x2[3]) {
    x2[0] -= x[0];
    x2[1] -= x[1];
    x2[2] -= x[2];
    x2[2] -= std::min(std::max(x2[2], 0.0f), height);
    return std::sqrt(x2[0] * x2[0] + x2[1] * x2[1] + x2[2] * x2[2]) - radius_x;
  }
  bool collides_with_block(float xx, float yy, float zz, float dx, float dy,
                           float dz) {
    float d = 1.0 / 1;
    if (zz > (x[2] + height + dz) || zz + d < x[2] + dz) {
      return false;
    }
    if (yy > (x[1] + dy + radius_x) || yy + d < (x[1] - radius_x + dy)) {
      return false;
    }

    if (xx > (x[0] + dx + radius_x) || xx + d < (x[0] - radius_x + dx)) {
      return false;
    }
    Vector3 c{(x[0] + dx), (x[1] + dy), (x[2] + dz)};
    bool collides_xy = false;

    float offsets[4][2][2] = {
        {{0, 0}, {0, d}}, {{0, d}, {d, d}}, {{d, d}, {d, 0}}, {{d, 0}, {0, 0}}};
    for (int face = 0; face < 4; ++face) {
      Vector3 a{xx + offsets[face][0][0], yy + offsets[face][0][1], 0};
      Vector3 b{xx + offsets[face][1][0], yy + offsets[face][1][1], 0};
      if ((c - a).projected(b - a).length() <= radius_x) {
        collides_xy = true;
      }
    }
    if (!collides_xy) {
      bool inside_x =
          ((x[0] + radius_x + dx) <= xx + d && x[0] - radius_x + dx >= xx);
      bool inside_y =
          ((x[1] + radius_x + dy) <= yy + d && (x[1] + radius_x + dy) >= yy);
      if (!inside_x || !inside_y) {
        return false;
      }
    }

    if (zz > (x[2] + dz) && zz < (x[2] + dz + height)) {
      return true;
    }

    if (zz + d > (x[2] + dz) && zz + 1 < (x[2] + dz + height)) {
      return true;
    }
    return false;
  }

  float sdf_box(Vector3 p, Vector3 c) {
    float x = std::max(p.x() - c.x() - 0.5f, c.x() - p.x() - 0.5f);

    float y = std::max(p.y() - c.y() - 0.5f, c.y() - p.y() - 0.5f);

    float z = std::max(p.z() - c.z() - 0.5f, c.z() - p.z() - 0.5f);

    float d = x;
    d = std::max(d, y);
    d = std::max(d, z);
    return d;
  }
  float sdBox(float y[3], int i, int j, int k) {
    return sdf_box({y[0], y[1], y[2]}, {(float)i, (float)j, (float)k});
  }
  void update_physics(float dt) {
    if (first_frame) {
      fall_start = now();
      first_frame = false;
      return;
    }
    world *w = get_world();

    float dz0 =
        std::max((xdot[2]) * dt - gravity_multiplier * (!on_land && !on_water) *
                                      9.81f * (float)(now() - fall_start) * dt,
                 -terminal_velocity * dt);
    float dy = ((xdot[1]) * dt + (on_land + (!on_land && on_water) * 0.1)
                    ? walk_vector[1] * friction * dt
                    : 0.0);
    float dx = ((xdot[0]) * dt + (on_land + (!on_land && on_water) * 0.3 +
                                  (!on_land && !on_water) * 0.1)
                    ? walk_vector[0] * friction * dt
                    : 0.0);

    float dz1 = std::max(dz0 - dt * dt * 9.81f / 2.0f * gravity_multiplier,
                         -terminal_velocity * dt);
    on_water = false;
    on_land = false;
    stop_moving = false;

    can_climb = false;
    bool near_land = false;
    float dmin = 100000;
    int xx0 = (x[0] + std::min(0.0f, dx * 10.0f) / 10.0 - radius_x);
    int yy0 = (x[1] + std::min(0.0f, dy * 10.0f) / 10.0 - radius_y);
    int zz0 = std::floor(x[2] + std::min(0.0f, dz1 * 10.0f) / 10.0f) - 1;
    int Nx = std::ceil(x[0] + std::max(dx * 10.0f / 10.0f, 0.0f) + radius_x) -
             xx0 + 1;
    int Ny = std::ceil(x[1] + std::max(dy * 10.0f / 10.0f, 0.0f) + radius_y) -
             yy0 + 1;
    int Nz =
        std::ceil(x[2] + std::max(dz1 * 10.0f, 0.0f) / 10.0 + height) + 1 - zz0;
    /*
    std::vector<float> d, d2;
    d.reserve(Nx*Ny*Nz*64);
    for (int ii = 0; ii < Nx; ii += 1) {

            for (int jj = 0; jj < Ny; jj += 1) {


                    for (int kk = 0; kk < Nz; kk += 1) {

                            int i = xx0 + ii;
                            int j = yy0 + jj;
                            int k = zz0 + kk;
                            block_t b = w->get_voxel(i, j, k);

                            for (int l = 0; l < 4; l += 1) {

                                    for (int m = 0; m < 4; m += 1) {
                                            for (int n = 0; n < 4; n += 1) {
                                                    if (b > WATER) {
                                                            float d0 =
    std::max(4.0f - l, (float)l); d0 = std::min(std::max(4.0f - m, (float)m),
    d0); d0 = std::min(std::max(4.0f - n, (float)n), d0);
                                                            d.push_back(-std::abs(d0
    / 4.0f));
                                                    }
                                                    else {
                                                            float d0 = 9999.0f;

                                                            float x2[3] = { i,
    j,k }; for (int r = 1; r < std::max(std::max(Nx, Ny), Nz); ++r) { for (int
    ii2 = -r; ii2 <= r; ii2 += 2 * r) { for (int jj2 = -r; jj2 <= r; jj2 += 2 *
    r) { for (int kk2 = -r; kk2 <= r; kk2 += 2 * r) { if
    (w->get_voxel(ii2+i+l/4.0, jj2+j+l/4.0, kk2+k+l/4.0)>WATER) { d0 =
    std::min(d0, std::abs(sdBox(x2, ii2+i, jj2+j, kk2+k)));
                                                                                            }
                                                                                    }
                                                                            }
                                                                    }
                                                                    if (d0 <
    9999.0f) { break;
                                                                    }
                                                            }


                                                            d.push_back(d0);
                                                    }
                                            }
                                    }
                            }
                    }
            }
    }*/
    int t2 = 0;
    float dz;
    for (t2 = 1; (t2 < 32) && !stop_moving; t2 += 2) {
      dz = std::max(dz0 - dt * dt * 9.81f * (t2 * t2 / 100.0f) / 2.0f *
                              gravity_multiplier,
                    -terminal_velocity * dt);
      for (int ii = 0; ii < Nx; ii += 1) {

        for (int jj = 0; jj < Ny; jj += 1) {
          for (int kk = 0; kk < Nz; kk += 1) {
            int i = xx0 + ii;
            int j = yy0 + jj;
            int k = zz0 + kk;

            block_t b = std::abs(w->get_voxel(i, j, k));
            if (b < WATER) {
              continue;
            }
            if (k <= std::ceil(x[2] + dz * t2 / 10.0 + 1)) {
              near_land = near_land || (b > WATER);

              if (collides_with_block(i, j, k + 1, dx * t2 / 10.0,
                                      dy * t2 / 10.0, dz * (t2 - 1) / 10.0)) {
                on_land = on_land || (b > WATER);
              }
            }
            for (int l = 0; l < 4; ++l) {
              for (int m = 0; m < 4; ++m) {
                for (int n = 0; n < 4; ++n) {
                  float x2[3] = {i + l / 4.0f, j + m / 4.0f, k + n / 4.0f};
                  int ix = (ii * 4 + l) * Ny * Nz * 4 * 4 +
                           (jj * 4 + m) * Nz * 4 + kk * 4 + n;
                  // d[ix]=std::max(-std::abs(signed_distance_to(x2)), d[ix]);
                }
              }
            }

            int face = 0;
            if (collides_with_block(i, j, k, dx * t2 / 10.0, dy * t2 / 10.0,
                                    dz * t2 / 10.0)) {
              if (b > WATER && t2 <= 10) {
                stop_moving = true;
              }
              float pos[3] = {(float)i, (float)j, (float)k};
              on_water = on_water || (b == WATER && t2 == 1);

              if (on_water && !on_land && t2 == 1 && b == WATER) {
                if (k == (int)eye_positions[2] + x[2]) {
                  under_water = true;
                }
              }
            }
          }
        }
      }
    }
    if (stop_moving) {
      dx = dy = dz = 0;
      walk_vector[0] = walk_vector[1] = walk_vector[2] = 0;
      stop_moving = false;
      return;
    }
    dz = dz0;
    if (on_land) {
      fall_start = now();
      friction = 1.0f;
      dz = std::max(dz, 0.0f);
    }
    if ((-terminal_velocity * 0.2) > dz / dt && on_land) {
      damage(fall_damage * (-dz / dt) / terminal_velocity);
      dz = 0.0;
    }

    else if (on_water && !on_land) {
      dz += 0.25f * sin(0.2 * now()) * dt;
      friction = 0.4f;
    }
    if (dz <= 0 && xdot[2] > 0 && !near_land) {
      fall_start = now();
      xdot[2] = 0;
    }
    if (jump_) {
      jump_ = false;
      if (near_land && dz <= 0.0) {
        xdot[2] = jump_speed;
        xdot[1] = jump_speed * dy;
        xdot[0] = jump_speed * dx;
      }
      jump_speed = 0.0;
    }
    walk_vector[0] = 0;
    walk_vector[1] = 0;
    walk_vector[2] = 0;

    x[0] += dx;
    x[1] += dy;
    x[2] += dz;
  }
  virtual void update(float dt){
      update_physics(dt);
  }
  void apply_impulse(float x, float y, float z) {
    xdot[1] += y;
    xdot[0] += x;
    xdot[2] += z;
  }
  void strafe(bool l, float speed) {
    Magnum::Math::Vector3<float> whichway, up{0, 0, 1},
        fwd{look_vector[0], look_vector[1], look_vector[2]};
    whichway = Magnum::Math::cross<float>(up, fwd);
    if (!l) {
      whichway = -whichway;
    }
    whichway = whichway.normalized();
    walk_vector[0] += speed * whichway.x();
    walk_vector[1] += speed * whichway.y();
    walk_vector[2] = 0;
  }
  void pedal(bool f, float speed) {
    int s = 1 - 2 * !f;
    walk_vector[0] +=
        speed * s * look_vector[0] /
        sqrt(look_vector[0] * look_vector[0] + look_vector[1] * look_vector[1] +
             look_vector[2] * look_vector[2] * under_water);
    walk_vector[1] +=
        speed * s * look_vector[1] /
        sqrt(look_vector[0] * look_vector[0] + look_vector[1] * look_vector[1] +
             look_vector[2] * look_vector[2] * under_water);
    walk_vector[2] +=
        speed * s * look_vector[2] * under_water /
        sqrt(look_vector[0] * look_vector[0] + look_vector[1] * look_vector[1] +
             look_vector[2] * look_vector[2] * under_water);
    ;
  }
  void look(float x, float y, float dt, int which_head) {
    Magnum::Math::Vector3<float> whichway, up{0, 0, 1},
        fwd{look_vector[0], look_vector[1], look_vector[2]},
        up2{up_vector[0], up_vector[1], up_vector[2]};
    whichway = Magnum::Math::cross<float>(up, fwd);
    auto a = Magnum::Math::Quaternion<float>::rotation(
        Magnum::Math::Rad<float>(2.0f * M_PI * y * dt), whichway.normalized());
    auto b = Magnum::Math::Quaternion<float>::rotation(
        Magnum::Math::Rad<float>(-2.0f * M_PI * x * dt), up.normalized());
    fwd = (a * b).transformVectorNormalized(fwd.normalized());
    up2 = (a * b).transformVectorNormalized(up2.normalized());
    look_vector[0] = fwd.x();
    look_vector[1] = fwd.y();
    look_vector[2] = fwd.z();
    //up_vector[0] = up2.x();
    //up_vector[1] = up2.y();
    //up_vector[2] = up2.z();
  } /*
  void look(float x, float y, float dt, int which_head) {
          float q[4];
          if(look_vector[3]>0.9 && y>0){
                  y=0.0;
          }
          if(look_vector[3]<-0.9 && y<0){
                  y=0.0;
          }
          toQuaternion(-x*dt,y*dt,0,q);
                  rot(look_vector,q,look_vector);
          } */
  void jump(float speed) {
    jump_speed = speed;
    jump_ = true;
    jump_start = now();
  }
  void damage(float dmg) { health -= dmg; }
  float get_health() { return health; }
  float is_alive() { return health > 0.0f; }
  void toggle_flying() { flying = !flying; }
  void toggle_climbing() { climbing = !climbing; }
};


