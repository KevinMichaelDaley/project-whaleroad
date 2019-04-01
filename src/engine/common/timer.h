#pragma once
#include <chrono>
class timer {
private:
  static std::chrono::time_point<std::chrono::high_resolution_clock> t_start;
  static std::chrono::high_resolution_clock::time_point t1_, t0_;

public:
  static void set_start() ;
  static double now() ;
  static double step() ;
  static void next();
};
