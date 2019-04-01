#pragma once
#include "world.h"
#include <Magnum/Math/Range.h>
class block_filter_chain {
public:
  typedef void (*column_operation)(block_t *, uint16_t *, uint16_t *,
                                   int, int, int, int, world_page *, world *);
private:
  Range3Di bounds;
  std::vector<column_operation *> passes;
public:
  block_filter_chain();

  void add_pass(column_operation *pass);

  void update_sub_region(Range3Di new_bounds);
};

class block_iterator {
public:
  static void iter_columns(world *wld, Range3Di bounds, block_filter_chain::column_operation op,
                           int z_neighborhood_size = 1, bool diff_only = false);
};
