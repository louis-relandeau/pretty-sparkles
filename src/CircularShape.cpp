#include "CircularShape.hpp"

#include <algorithm>
#include <cmath>

CircularShape::CircularShape(int thickness_, float centerPotential_,
                             float outerPotential_)
    : thickness(thickness_), centerPotential(centerPotential_),
      outerPotential(outerPotential_) {}

void CircularShape::build(BoundaryConditionGrid &out, int N) const {
  out.renderMask.assign(N * N, 0.0f);
  out.fixedMask.assign(N * N, 0);
  out.fixedPotential.assign(N * N, 0.0f);
  if (N <= 0)
    return;

  const int cx = N / 2;
  const int cy = N / 2;
  const int radius = std::max(1, (N - 2) / 2);
  const int clampedThickness = std::max(1, std::min(thickness, radius));
  const int inner = radius - clampedThickness;

  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      int dx = i - cx;
      int dy = j - cy;
      float r = std::sqrt(static_cast<float>(dx * dx + dy * dy));
      if (r >= static_cast<float>(inner) && r <= static_cast<float>(radius)) {
        int idx = i * N + j;
        out.renderMask[idx] = 1.0f;
        out.fixedMask[idx] = 1;
        out.fixedPotential[idx] = outerPotential;
      }
    }
  }

  int centerIdx = cx * N + cy;
  out.renderMask[centerIdx] = 1.0f;
  out.fixedMask[centerIdx] = 1;
  out.fixedPotential[centerIdx] = centerPotential;
}