#pragma once
#include <cstdint>
enum block_value : int16_t {
  EMPTY = 0, AIR, WATER, SAND, GRASS, STONE, DIRT, SNOW, SANDSTONE
};
typedef int16_t block_t;
constexpr const block_t SEA = WATER;
constexpr const block_t BEDROCK = 127;
static block_t default_columm[65536] = {BEDROCK, SAND, SEA};
static block_t sky_column[65536] = {EMPTY};
bool block_is_slippery(block_t block);
bool block_is_opaque(block_t block);
bool block_is_visible(block_t block);
int block_emissive_strength(block_t block);
bool block_is_solid(block_t block);
void block_albedo(block_t block, float &r, float &g, float &b);
