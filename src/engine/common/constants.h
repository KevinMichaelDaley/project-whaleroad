#pragma once
#include <cstdint>
namespace constants {
constexpr const int WORLD_HEIGHT = 96;
constexpr const int MAX_IP_HEADER_SIZE = 60;
constexpr const int UDP_HEADER_SIZE = 8;
constexpr const int CHUNK_WIDTH = 16;
constexpr const int CHUNK_HEIGHT = 48;
constexpr const int64_t MAX_RESIDENT_PAGES = 160;
constexpr const int PAGE_DIM = 128;
constexpr const int LIGHT_COMPONENTS = 12;
constexpr const int64_t PAGE_RAM =
    PAGE_DIM * PAGE_DIM *
        (LIGHT_COMPONENTS * WORLD_HEIGHT * 1 + WORLD_HEIGHT * 2 * 2 + 2) +
    (PAGE_DIM * PAGE_DIM) / 32 * 8;
// light                                    //blocks,invisible_blocks
// //dirty_list   //zmap
constexpr const int64_t ALL_PAGES_RAM = MAX_RESIDENT_PAGES * PAGE_RAM;
constexpr const int MAX_CONCURRENCY = 12;
constexpr const int INVERSE_COMPONENTS[] = {1, 0, 3, 2, 5, 4};
constexpr const float LPV_WEIGHT[] = {
    -1, 0,  1,  -1, 0,  1,  -1, 0,  1,  -1, 0,  1,  -2, 0,  1,  -1, 0,  1,
    -1, 0,  1,  -1, 0,  1,  -1, 0,  1,  1,  0,  -1, 1,  0,  -1, 1,  0,  -1,
    1,  0,  -1, 1,  0,  -2, 1,  0,  -1, 1,  0,  -1, 1,  0,  -1, 1,  0,  -1,
    -1, -1, -1, 0,  0,  0,  1,  1,  1,  -1, -2, -1, 0,  0,  0,  1,  1,  1,
    -1, -1, -1, 0,  0,  0,  1,  1,  1,  1,  1,  1,  0,  0,  0,  -1, -1, -1,
    1,  1,  1,  0,  0,  0,  -1, -2, -1, 1,  1,  1,  0,  0,  0,  -1, -1, -1,
    -1, -1, -1, -1, -2, -1, -1, -1, -1, 0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  -1, -1, -1, -1, -2, -1, -1, -1, -1};
constexpr const float LPV_BIAS[] = {0.001, 0.3, 0.3, 0.3, 0.3, 0.3};
} // namespace constants
