#pragma once

#include <cstdint>
#include <vector>

struct BoundaryConditionGrid {
  std::vector<float> renderMask;
  std::vector<uint8_t> fixedMask;
  std::vector<float> fixedPotential;
};

class BoundaryGeometry {
public:
  virtual ~BoundaryGeometry() = default;

  virtual void build(BoundaryConditionGrid &out, int N) const = 0;
};
