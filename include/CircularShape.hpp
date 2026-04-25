#pragma once

#include "BoundaryGeometry.hpp"
#include <vector>

class CircularShape : public BoundaryGeometry {
private:
  int thickness;
  float centerPotential;
  float outerPotential;

public:
  explicit CircularShape(int thickness = 5, float centerPotential = 1.0f,
                         float outerPotential = 0.0f);

  void build(BoundaryConditionGrid &out, int N) const override;
};
