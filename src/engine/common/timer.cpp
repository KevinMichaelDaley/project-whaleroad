#include "timer.h"
#include "vox/world.h"
std::chrono::time_point<std::chrono::high_resolution_clock> timer::t_start;
std::chrono::high_resolution_clock::time_point timer::t1_, timer::t0_;
void timer::set_start() {
  t_start = std::chrono::high_resolution_clock::now();
  t0_ = t_start;
  t1_ = t_start;
}
double timer::now() {
  return std::chrono::duration_cast<std::chrono::duration<double>>(
             std::chrono::high_resolution_clock::now() - t_start)
      .count();
}
double timer::step() { return std::chrono::duration_cast<std::chrono::duration<double>>(t1_ - t0_).count(); }
void timer::next() {
  t0_ = t1_;
  t1_ = std::chrono::high_resolution_clock::now();
}
double now(){
    return timer::now();
}
