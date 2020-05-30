#pragma once
#include <cstdio>

#include "../../Util/util.hpp"

namespace griddly {

class GridLocation {
 public:
  GridLocation(int32_t x, int32_t y) : x(x), y(y) {}

  bool operator==(const GridLocation& other) const {
    return x == other.x && y == other.y;
  }

  struct Hash {
    std::size_t operator()(GridLocation const& location) const noexcept {
      std::size_t seed = 0;
      hash_combine(seed, location.x);
      hash_combine(seed, location.y);
      return seed;
    }
  };

  const int32_t x;
  const int32_t y;
};

}  // namespace griddly
