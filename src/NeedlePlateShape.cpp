#include "NeedlePlateShape.hpp"

#include <algorithm>

NeedlePlateShape::NeedlePlateShape(int needleLength_, int needleHalfWidth_,
                                   int topOffset_, int plateThickness_,
                                   float needlePotential_,
                                   float platePotential_)
    : needleLength(needleLength_), needleHalfWidth(needleHalfWidth_),
      topOffset(topOffset_), plateThickness(plateThickness_),
      needlePotential(needlePotential_), platePotential(platePotential_) {}

void NeedlePlateShape::build(BoundaryConditionGrid &out, int N) const {
  out.renderMask.assign(N * N, 0.0f);
  out.fixedMask.assign(N * N, 0);
  out.fixedPotential.assign(N * N, 0.0f);

  if (N <= 0)
    return;

  const int cx = N / 2;
  const int clampedNeedleLength = std::max(1, std::min(needleLength, N - 1));
  const int clampedNeedleHalfWidth = std::max(0, needleHalfWidth);
  const int clampedTopOffset = std::max(0, std::min(topOffset, N - 1));
  const int clampedPlateThickness = std::max(1, std::min(plateThickness, N));

  const int needleStart = clampedTopOffset;
  const int needleEnd = std::min(N, needleStart + clampedNeedleLength);
  const int needleLeft = std::max(0, cx - clampedNeedleHalfWidth);
  const int needleRight = std::min(N - 1, cx + clampedNeedleHalfWidth);

  for (int i = needleStart; i < needleEnd; ++i) {
    for (int j = needleLeft; j <= needleRight; ++j) {
      int idx = i * N + j;
      out.renderMask[idx] = 1.0f;
      out.fixedMask[idx] = 1;
      out.fixedPotential[idx] = needlePotential;
    }
  }

  const int plateStart = N - clampedPlateThickness;
  for (int i = plateStart; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      int idx = i * N + j;
      out.renderMask[idx] = 1.0f;
      out.fixedMask[idx] = 1;
      out.fixedPotential[idx] = platePotential;
    }
  }
}
