#pragma once

#include <cstdint>
#include <random>
#include <vector>

struct Point {
  int x, y;
};

class SolverDBM {
private:
  int STEPS = 2000;
  double ETA = 1.25;

  std::vector<float> &field;
  std::vector<float> &arc;
  std::vector<uint8_t> fixedMask;
  std::vector<float> fixedPotential;
  std::vector<uint8_t> ignitionMask;
  float arcPotential = 1.0f;

  int N;
  int cx, cy;

  std::mt19937 rng;
  std::uniform_real_distribution<double> dist;

  void checkForFieldFile(bool forceRecompute);
  uint64_t hashFieldF() const;
  void computeFieldMultiscale();
  bool isFixed(int i, int j) const;
  bool computePointLaplace(int x, int y, int step, double &out) const;
  void interpolateLevel(int step);
  void solveLaplace();
  std::vector<Point> getCandidates();
  std::vector<Point> pick(std::vector<Point> &cands);

public:
  SolverDBM(std::vector<float> &field, std::vector<float> &arc, int N,
            const std::vector<uint8_t> &fixedMaskIn = std::vector<uint8_t>(),
            const std::vector<float> &fixedPotentialIn = std::vector<float>());

  void init(bool forceRecompute = false);
  void step();
  void setBoundaryConditions(const std::vector<uint8_t> &fixedMaskIn,
                             const std::vector<float> &fixedPotentialIn);
};
