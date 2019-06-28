#pragma once
#include "character.h"
#include "gfx/camera.h"
class player : public character {
  float jump_speed;
  float look_x, look_y;
  bool spawned;
  bool jump_down, w_down, a_down, s_down, d_down;
  std::string name_;
  float sensitivity;
  camera cam;
public:
  camera get_cam(){
      return cam;
  }
  int target[3];
  virtual std::string get_name(){
      return name_;
  }
  player(std::string name, world* wld, int sx = 0, int sy = 0, int sz = 0,
         bool spawn_random = true, bool spawn_on_surface = true)
      : jump_speed(3.0f), name_(name),
        character(wld, sx, sy, sz, spawn_random, spawn_on_surface) {
    block_last = 0;
    lag=0;
    place_block = STONE;
    left_mouse_down = false;
    right_mouse_down = false;
    spawned = false;
    chisel_strength = 4;
    max_block_health = -1;
    look_x = 0;
    look_y = 1;
    jump_down = false;
    w_down = false;
    a_down = false;
    s_down = false;
    d_down = false;
    sensitivity = 0.05f;
   }
  void spawn() { spawned = true; }
  void despawn() { spawned = false; }
  bool is_in_world() { return spawned; }
#ifndef __ANDROID__
  typedef Magnum::Platform::Sdl2Application::KeyEvent::Key keycode_t;

  void keydown(keycode_t k) {
    if (k == keycode_t::Space) {
      jump_down = true;
    } else if (k == keycode_t::W) {
      w_down = true;
    }

    else if (k == keycode_t::A) {
      a_down = true;
    }

    else if (k == keycode_t::S) {
      s_down = true;
    }

    else if (k == keycode_t::D) {
      d_down = true;
    }

    else if (k == keycode_t::C) {
      toggle_climbing();
    }
    
    else if (k == keycode_t::One) {
       place_block=GRASS;
    }
    else if (k == keycode_t::Two) {
       place_block=STONE;
    }
    else if (k == keycode_t::Three) {
       place_block=DIRT;
    }
  }
  void keyup(keycode_t k) {
    if (k == keycode_t::Space) {
      jump_down = false;
    } else if (k == keycode_t::W) {
      w_down = false;
    }
    

    else if (k == keycode_t::A) {
      a_down = false;
    }

    else if (k == keycode_t::S) {
      s_down = false;
    }

    else if (k == keycode_t::D) {
      d_down = false;
    }
  }
#endif
  void mousemove(int x, int y) {
    look_x += x *sensitivity;
    look_y += y *sensitivity;
  }
  double block_last;
  int lag;
  int place_block;
  bool left_mouse_down;
  virtual void update(float dt) {
    if (w_down && !s_down) {
      pedal(true, 2.4f);
    } else if (s_down && !w_down) {
      pedal(false, 1.8f);
    } else if (a_down && !d_down) {
      strafe(true, 2.4);
    }

    else if (d_down && !a_down) {
      strafe(false, 2.4);
    }
    if (jump_down) {
      jump(6.0f);
      jump_down = false;
    }
    viewProj=cam.projection*cam.view;
    if (left_mouse_down) {
      float xx, yy, zz;
      float rx, ry, rz;
      dir(rx, ry, rz);
      eye(xx, yy, zz);
      float n = std::sqrt(rx * rx + ry * ry + rz * rz);
      rx /= n;
      ry /= n;
      rz /= n;
      float t = 0;
      world *wld = get_world();
      if (now() - block_last > 0.1 * (place_block > 0)) {
        Matrix4 unproj = viewProj.inverted();
        Vector3 eye_ = unproj.transformPoint({0.0, 0.0, 0.0});
        xx = eye_.x();
        yy = eye_.y();
        zz = eye_.z();
        block_target_type = 0;
        while (t < 10) {
          float x2 = xx + t * rx, y2 = yy + t * ry, z2 = zz + t * rz;
          block_t b = wld->get_voxel(x2, y2, z2);
          block_health = 100000;
          if (b > WATER) {
            target[0] = floor(x2);
            target[1] = floor(y2);
            target[2] = floor(z2);
            break;
          }
          t += 0.5f;
        }
        if (t > sqrt(2) && t < 10 && place_block > AIR && target[2] > 0) {
          float dot = -1;
          Vector3 nml, I{xx - target[0], yy - target[1], zz - target[2]};
          constexpr float FacesNormal[6][3]={{1,0,0},{-1,0,0},{0,1,0}, {0, -1,0}, {0,0,1}, {0,0,-1}};
          for (int face = 0; face < 6; ++face) {
            Vector3 n = {FacesNormal[face][0], FacesNormal[face][1],
                         FacesNormal[face][2]};
            float dot2 = n.x() * I.x() + n.y() * I.y() + n.z() * I.z();
            if (dot2 > dot) {
              if (std::abs(wld->get_voxel(
                      std::floor(target[0] + 0.5 + n.x()),
                      std::floor(target[1] + 0.5 + n.y()),
                      std::floor(target[2] + 0.5 + n.z()))) <= WATER) {
                dot = dot2;
                nml = n;
              }
            }
          }
          if (dot > 0 && lag<0) {
            // if(inventory.has(place_block,1);
            wld->set_voxel(std::floor(target[0] + 0.5 + nml.x()),
                           std::floor(target[1] + 0.5 + nml.y()),
                           std::floor(target[2] + 0.5 + nml.z()), place_block);
            lag=50;
          }

          // inventory.remove(place_block,1);
        }

        target[2] = -100000;
        block_last = now();
      }
    }
    --lag;
    if (right_mouse_down) {
      float xx, yy, zz;
      float rx, ry, rz;
      dir(rx, ry, rz);
      eye(xx, yy, zz);
      float n = std::sqrt(rx * rx + ry * ry + rz * rz);
      rx /= n;
      ry /= n;
      rz /= n;
      float t = 0;
      world *wld = get_world();

      if (now() - block_last > 0.5 * (max_block_health > 0)) {
        Matrix4 unproj = viewProj.inverted();
        Vector3 eye_ = unproj.transformPoint({0.0, 0.0, 0});
        xx = eye_.x();
        yy = eye_.y();
        zz = eye_.z();
        block_target_type = 0;
        while (t < 10) {
          float x2 = xx + t * rx, y2 = yy + t * ry, z2 = zz + t * rz;
          block_t b = wld->get_voxel(x2, y2, z2);
          block_health = 100000;
          if (b > WATER) {
            if (floor(x2) != target[0] || floor(y2) != target[1] ||
                floor(z2) != target[2]) {
              block_health = (b == STONE || b == SANDSTONE) ? 4 : 2;
              max_block_health = block_health;
            }
            block_target_type = b;
            target[0] = floor(x2);
            target[1] = floor(y2);
            target[2] = floor(z2);
            break;
          }
          t += 0.5f;
        }
        block_last = now();
      }
      if (block_target_type > WATER && t < 10) {
        block_health -= chisel_strength * dt;
        if (block_health <= 0) {
          wld->set_voxel(target[0], target[1], target[2], AIR);
          target[2] = -100000;
          max_block_health = -1;
          block_target_type = 0;
          block_health = 100000;
        }
      }
    }
    ((character*)this)->update_physics(dt);
    look(look_x,look_y, dt, 0);
    look_x=0;
     look_y=0;
    Vector3 eye, fw, up;
    ((character*)this)->get_eye_basis(eye,fw,up);
    cam.look_at(eye,fw,up);
  }
  bool right_mouse_down;
  block_t block_target_type;
  int block_health;
  int chisel_strength;
  int max_block_health;
  Matrix4 viewProj;
  void mousedown(int which_button) {

    if (which_button == 0) {
      left_mouse_down = true;
    }
    if (which_button == 1) {
      right_mouse_down = true;
    }
  }

  void mouseup(int which_button) {
    if (which_button == 1) {
      right_mouse_down = false;
      target[2] = -100000;
      max_block_health = -1;
      block_target_type = AIR;
      block_last = now();
    } else if (which_button == 0) {

      left_mouse_down = false;
      target[2] = -100000;
      max_block_health = -1;
      block_target_type = AIR;
    }
  }
};
