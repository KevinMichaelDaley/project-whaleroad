#pragma once

#include <Magnum/Trade/ImageData.h>

#include <Corrade/Containers/ArrayView.h>

#include "Magnum/Trade/AbstractImporter.h"
#include "Magnum/Trade/ImageData.h"

#include "Magnum/Math/Vector2.h"
#include "PerlinNoise.h"
#include "world.h"
#include <MagnumPlugins/AnyImageImporter/AnyImageImporter.h>
#include <cstdio>
#include <cstring>
using namespace Magnum;

class Generator {

public:
  Generator(int seed) {}
  ~Generator() { delete[] data; }
};
