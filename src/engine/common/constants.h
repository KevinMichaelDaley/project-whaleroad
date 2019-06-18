#pragma once
namespace constants {
constexpr const  int WORLD_HEIGHT = 1024;
constexpr const  int UDP_MTU = 576; 
constexpr const  int MAX_IP_HEADER_SIZE = 60;
constexpr const  int UDP_HEADER_SIZE = 8;
constexpr const  int MAX_DATAGRAM_SIZE =
    UDP_MTU - MAX_IP_HEADER_SIZE - UDP_HEADER_SIZE;
constexpr const  int CHUNK_WIDTH = 16;
constexpr const  int MAX_RESIDENT_PAGES=500;
constexpr const  int PAGE_DIM=128;
constexpr const  int LIGHT_COMPONENTS=6;
constexpr const  float LPV_WEIGHT[]={
    -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -2, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -2, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, -1, -1, -1, 0, 0, 0, 1, 1, 1, -1, -2, -1, 0, 0, 0, 1, 1, 1, -1, -1, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, -1, -1, -1, 1, 1, 1, 0, 0, 0, -1, -2, -1, 1, 1, 1, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -2, -1, -1, -1, -1};
} // namespace constants
    
