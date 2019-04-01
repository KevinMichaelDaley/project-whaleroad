#include "block.h"
#include <cmath>
bool block_is_slippery(block_t block) {
  return (block != STONE) && (block != SANDSTONE);
}
bool block_is_opaque(block_t block) {
  return block_is_visible(std::abs(block)) && block != WATER;
}
bool block_is_visible(block_t block) { return block > 0 && block != AIR; }
bool block_is_solid(block_t block) {
  return block_is_visible(std::abs(block)) && block != WATER;
}

void block_albedo(block_t bc, float &r, float &g, float &b) {
  if (bc < WATER) {
    r = 0;
    g = 0;
    b = 0;
  } else if (bc == SAND) {
    r = 1.0;
    g = 0.9;
    b = 0.5;
  } else if (bc == SANDSTONE) {
    r = 0.8;
    g = 0.7;
    b = 0.3;
  } else if (bc == STONE) {
    r = 0.4;
    g = 0.5;
    b = 0.3;
  } else if (bc == DIRT) {
    r = 0.6;
    g = 0.5;
    b = 0.3;
  } else if (bc == SNOW) {
    r = 0.9;
    g = 0.9;
    b = 0.9;
  } else if (bc == WATER) {
    r = 1.0;
    g = 1.0;
    b = 1.0;
  }

  else if (bc == GRASS) {
    r = 0.6;
    g = 0.7;
    b = 0.3;
  } else {
    r = 1;
    b = 1;
    g = 1;
  }
}
