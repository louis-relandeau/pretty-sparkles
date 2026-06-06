#include "SolverDBM.hpp"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

SolverDBM::SolverDBM(std::vector<float> &field_, std::vector<float> &arc_,
                     int N_, const std::vector<uint8_t> &fixedMaskIn,
                     const std::vector<float> &fixedPotentialIn)
    : field(field_), arc(arc_), N(N_), cx(N / 2), cy(N / 2), rng(45),
      dist(0.0, 1.0) {
  if (!fixedMaskIn.empty() && !fixedPotentialIn.empty() &&
      (int)fixedMaskIn.size() == N * N &&
      (int)fixedPotentialIn.size() == N * N) {
    fixedMask = fixedMaskIn;
    fixedPotential = fixedPotentialIn;
  } else {
    fixedMask.clear();
    fixedPotential.clear();
  }

#ifdef _OPENMP
  std::cout << "OpenMP threads (max): " << omp_get_max_threads() << "\n";
#else
  std::cout << "OpenMP not enabled in this build.\n";
#endif
}

void SolverDBM::setBoundaryConditions(
    const std::vector<uint8_t> &fixedMaskIn,
    const std::vector<float> &fixedPotentialIn) {
  if ((int)fixedMaskIn.size() == N * N &&
      (int)fixedPotentialIn.size() == N * N) {
    fixedMask = fixedMaskIn;
    fixedPotential = fixedPotentialIn;
  }
}

void SolverDBM::init(bool forceRecompute) {
  int R = N / 2 - 2;

  arc.assign(N * N, 0.0f);
  ignitionMask.assign(N * N, 0);

  if (!fixedMask.empty() && !fixedPotential.empty()) {
#ifdef _OPENMP
#pragma omp parallel for collapse(2) schedule(static)
#endif
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < N; ++j) {
        int idx = i * N + j;
        field[idx] = fixedMask[idx] ? fixedPotential[idx] : 0.0f;
      }
    }
  } else {
    fixedMask.assign(N * N, 0);
    fixedPotential.assign(N * N, 0.0f);
#ifdef _OPENMP
#pragma omp parallel for collapse(2) schedule(static)
#endif
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < N; ++j) {
        int idx = i * N + j;
        int dx = i - cx, dy = j - cy;
        double r = std::sqrt(dx * dx + dy * dy);
        if (r >= R) {
          field[idx] = 0.0f;
          fixedMask[idx] = 1;
          fixedPotential[idx] = 0.0f;
        } else {
          field[idx] = 0.0f;
        }
      }
    }

    int centerIdx = cx * N + cy;
    fixedMask[centerIdx] = 1;
    fixedPotential[centerIdx] = 1.0f;
    field[centerIdx] = 1.0f;
  }

  float bestPotential = -1e30f;
  for (int idx = 0; idx < N * N; ++idx) {
    if (!fixedMask[idx])
      continue;
    if (fixedPotential[idx] > bestPotential) {
      bestPotential = fixedPotential[idx];
    }
  }

  const float eps = 1e-6f;
  for (int idx = 0; idx < N * N; ++idx) {
    if (!fixedMask[idx])
      continue;
    if (std::abs(fixedPotential[idx] - bestPotential) <= eps) {
      ignitionMask[idx] = 1;
    }
  }

  int igniteCount = 0;
  for (int idx = 0; idx < N * N; ++idx) {
    if (ignitionMask[idx])
      ++igniteCount;
  }
  if (igniteCount == 0) {
    std::cout
        << "Warning: no ignition boundary found; arc growth will not start.\n";
  } else {
    // Keep the discharge channel at the same potential as the ignition
    // electrode.
    arcPotential = bestPotential;
  }

  checkForFieldFile(forceRecompute);
}

uint64_t SolverDBM::hashFieldF() const {
  uint64_t hash = 1469598103934665603ULL; // FNV offset basis
  const uint64_t prime = 1099511628211ULL;

  // mix N
  hash ^= static_cast<uint64_t>(N);
  hash *= prime;

  for (const auto &v : field) {
    uint32_t bits;
    static_assert(sizeof(bits) == sizeof(v));
    std::memcpy(&bits, &v, sizeof(v));
    hash ^= bits;
    hash *= prime;
  }

  for (const auto &b : fixedMask) {
    hash ^= static_cast<uint64_t>(b);
    hash *= prime;
  }

  for (const auto &v : fixedPotential) {
    uint32_t bits;
    static_assert(sizeof(bits) == sizeof(v));
    std::memcpy(&bits, &v, sizeof(v));
    hash ^= bits;
    hash *= prime;
  }

  return hash;
}

void SolverDBM::checkForFieldFile(bool forceRecompute) {
  uint64_t h = hashFieldF();
  std::string filename = "fields/field_" + std::to_string(h) + ".bin";

  if (std::filesystem::exists(filename) && !forceRecompute) {
    std::cout << "Loading field from " << filename << "\n";
    std::ifstream in(filename, std::ios::binary);
    if (in) {
      in.read((char *)field.data(), field.size() * sizeof(float));
      in.read((char *)fixedMask.data(), fixedMask.size() * sizeof(uint8_t));
      in.read((char *)fixedPotential.data(),
              fixedPotential.size() * sizeof(float));
    }
  } else {
    if (forceRecompute) {
      std::cout
          << "Force recompute enabled, ignoring existing field file if any.\n";
    } else {
      std::cout << "No field file found at " << filename
                << ", initializing...\n";
    }
    computeFieldMultiscale();
    std::filesystem::create_directories("fields");
    std::ofstream out(filename, std::ios::binary);
    if (out) {
      out.write((char *)field.data(), field.size() * sizeof(float));
      out.write((char *)fixedMask.data(), fixedMask.size() * sizeof(uint8_t));
      out.write((char *)fixedPotential.data(),
                fixedPotential.size() * sizeof(float));
    }
  }
}

void SolverDBM::computeFieldMultiscale() {
  int maxStep = 1;
  while (maxStep * 2 < N)
    maxStep *= 2;

  for (int step = maxStep; step >= 1; step /= 2) {
    double max_diff;

    // Red-Black Gauss-Seidel
    do {
      max_diff = 0.0;

      // red pass
#ifdef _OPENMP
#pragma omp parallel for collapse(2) reduction(max : max_diff) schedule(guided)
#endif
      for (int i = 0; i < N; i += step) {
        for (int j = 0; j < N; j += step) {
          int ri = (i / step) + (j / step);
          if ((ri & 1) != 0)
            continue; // skip black
          double field_val;
          if (!computePointLaplace(i, j, step, field_val))
            continue;
          int idx = i * N + j;
          double old = field[idx];
          double diff = std::abs(field_val - old);
          field[idx] = static_cast<float>(field_val);
          if (diff > max_diff)
            max_diff = diff;
        }
      }

      // black pass
#ifdef _OPENMP
#pragma omp parallel for collapse(2) reduction(max : max_diff) schedule(guided)
#endif
      for (int i = 0; i < N; i += step) {
        for (int j = 0; j < N; j += step) {
          int ri = (i / step) + (j / step);
          if ((ri & 1) == 0)
            continue; // skip red
          double field_val;
          if (!computePointLaplace(i, j, step, field_val))
            continue;
          int idx = i * N + j;
          double old = field[idx];
          double diff = std::abs(field_val - old);
          field[idx] = static_cast<float>(field_val);
          if (diff > max_diff)
            max_diff = diff;
        }
      }

    } while (max_diff > 1e-3);

    if (step > 1)
      interpolateLevel(step);
  }
}

bool SolverDBM::isFixed(int i, int j) const {
  int idx = i * N + j;
  return fixedMask[idx] || (arc[idx] != 0.0f);
}

bool SolverDBM::computePointLaplace(int x, int y, int step, double &out) const {
  if (isFixed(x, y))
    return false;

  double sum = 0.0;
  int count = 0;

  const int neighbors[4][2] = {
      {x - step, y}, {x + step, y}, {x, y - step}, {x, y + step}};

  for (const auto &n : neighbors) {
    int ni = n[0];
    int nj = n[1];
    if (ni < 0 || ni >= N || nj < 0 || nj >= N)
      continue;
    sum += field[ni * N + nj];
    ++count;
  }

  if (count == 0)
    return false;

  out = sum / static_cast<double>(count);
  return true;
}

void SolverDBM::interpolateLevel(int step) {
  int half = step / 2;
#ifdef _OPENMP
#pragma omp parallel for collapse(2) schedule(static)
#endif
  for (int i = 0; i < N - step; i += step) {
    for (int j = 0; j < N - step; j += step) {
      int i0 = i, j0 = j, i1 = i + step, j1 = j + step;
      int im = i + half, jm = j + half;

      if (!isFixed(im, j0))
        field[im * N + j0] = 0.5f * (field[i0 * N + j0] + field[i1 * N + j0]);

      if (!isFixed(i0, jm))
        field[i0 * N + jm] = 0.5f * (field[i0 * N + j0] + field[i0 * N + j1]);

      if (!isFixed(im, jm))
        field[im * N + jm] = 0.25f * (field[i0 * N + j0] + field[i1 * N + j0] +
                                      field[i0 * N + j1] + field[i1 * N + j1]);
    }
  }
}

void SolverDBM::solveLaplace() { computeFieldMultiscale(); }

std::vector<Point> SolverDBM::getCandidates() {
  std::vector<Point> out;
  if (N <= 2)
    return out;

  std::vector<uint8_t> added(N * N); // for candidates already found

  const int di[4] = {-1, 1, 0, 0};
  const int dj[4] = {0, 0, -1, 1};

  auto addNeighbors = [&](int i, int j) {
    for (int k = 0; k < 4; ++k) {
      int ni = i + di[k];
      int nj = j + dj[k];
      if (ni < 0 || ni >= N || nj < 0 || nj >= N)
        continue;
      int nidx = ni * N + nj;
      if (added[nidx])
        continue;
      if (!isFixed(ni, nj)) {
        added[nidx] = 1;
        out.push_back({ni, nj});
      }
    }
  };

  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      int idx = i * N + j;
      if (arc[idx] != 0.0f)
        addNeighbors(i, j);
      if (ignitionMask[idx])
        addNeighbors(i, j);
    }
  }

  return out;
}

std::vector<Point> SolverDBM::pick(std::vector<Point> &cands) {
  int numPicks = std::max(1, (int)std::ceil(0.0075 * cands.size()));
  std::vector<double> weights;
  weights.reserve(cands.size());
  float minV = 1e30f;
  float maxV = -1e30f;
  for (const auto &p : cands) {
    float v = field[p.x * N + p.y];
    if (v < minV)
      minV = v;
    if (v > maxV)
      maxV = v;
  }

  const double eps = 1e-12;
  for (auto &p : cands) {
    float v = field[p.x * N + p.y];
    // Prefer low potential so the front grows toward the low-potential
    // electrode.
    double normalizedLow =
        static_cast<double>((maxV - v) / std::max(1e-12f, maxV - minV));
    double w = std::pow(std::max(eps, normalizedLow), ETA);
    weights.push_back(w);
  }

  std::vector<Point> picks;
  for (int n = 0; n < numPicks && !cands.empty(); ++n) {
    std::discrete_distribution<int> d(weights.begin(), weights.end());
    int idx = d(rng);
    picks.push_back(cands[idx]);
    cands.erase(cands.begin() + idx);
    weights.erase(weights.begin() + idx);
  }
  return picks;
}

void SolverDBM::step() {
  solveLaplace();
  auto cands = getCandidates();
  if (cands.empty())
    return;
  auto picks = pick(cands);
  for (const auto &p : picks) {
    int idx = p.x * N + p.y;
    arc[idx] = arcPotential;
    field[idx] = arcPotential;
  }
}