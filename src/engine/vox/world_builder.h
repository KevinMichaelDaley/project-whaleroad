#include "common/stream.h"

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <sys/mman.h>
using namespace Magnum;
class world_builder {
private:
  static int fd;
  static uint64_t fsize;
  static std::string wfilename;
  static void* fptr;
public:
  enum map_mode{
      MAP_READ,
      MAP_WRITE
  };
private:
   static uint64_t mortonEncode(unsigned int x, unsigned int y, unsigned int z);
   static void* map_file();
public:
  static void map_world_file(std::string filename, bool& new_file);
  
  static stream * get_file_with_offset_r(Vector3i page_offset);

  static stream * get_file_with_offset_w(Vector3i page_offset);
  
  static void unmap_world_file();
};
