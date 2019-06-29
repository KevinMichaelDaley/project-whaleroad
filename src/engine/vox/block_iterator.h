#pragma once

#include "block.h"
#include "common/constants.h"

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector3.h>
class world_page;
class world;

std::string read_text_file(std::string filename);
class block_iterator {
public:
  typedef void (*column_operation)(block_t *, uint16_t *, uint16_t *, int, int,
                                   int, int, world_page *, world *);
  static void iter_columns(world *wld, Magnum::Range3Di bounds,
                           column_operation op, int neighborhood_size,
                           bool diff_only = false);
};
