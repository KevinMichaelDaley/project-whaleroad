#pragma once
namespace constants {
constexpr const unsigned int WORLD_HEIGHT = 1024;
constexpr const unsigned int UDP_MTU = 576;
constexpr const unsigned int MAX_IP_HEADER_SIZE = 60;
constexpr const unsigned int UDP_HEADER_SIZE = 8;
constexpr const unsigned int MAX_DATAGRAM_SIZE =
    UDP_MTU - MAX_IP_HEADER_SIZE - UDP_HEADER_SIZE;
constexpr const unsigned int CHUNK_WIDTH = 16;
constexpr const unsigned int MAX_RESIDENT_PAGES=1024;
constexpr const unsigned int PAGE_DIM=512;
constexpr const unsigned int LIGHT_COMPONENTS=1;
} // namespace constants
