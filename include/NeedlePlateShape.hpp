#pragma once

#include "BoundaryGeometry.hpp"

class NeedlePlateShape : public BoundaryGeometry {
private:
  int needleLength;
  int needleHalfWidth;
  int topOffset;
  int plateThickness;
  float needlePotential;
  float platePotential;

public:
  NeedlePlateShape(int needleLength = 90, int needleHalfWidth = 1,
                   int topOffset = 2, int plateThickness = 2,
                   float needlePotential = 0.0f, float platePotential = 1.0f);

  void build(BoundaryConditionGrid &out, int N) const override;
};
