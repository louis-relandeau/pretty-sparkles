#include "SolverDBM.hpp"

#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#ifdef _OPENMP
#include <omp.h>
#endif

SolverDBM::SolverDBM(std::vector<float>& field_,
                     std::vector<float>& arc_,
                     int N_, const std::vector<uint8_t>& boundary_in)
    : field(field_), arc(arc_), N(N_), cx(N/2), cy(N/2), rng(45), dist(0.0, 1.0)
{
    // copy or empty if not provided to let init do a default circle
    if (!boundary_in.empty() && (int)boundary_in.size() == N*N) {
        boundary = boundary_in;
    } else {
        boundary.clear();
    }

#ifdef _OPENMP
    std::cout << "OpenMP threads (max): " << omp_get_max_threads() << "\n";
#else
    std::cout << "OpenMP not enabled in this build.\n";
#endif

}

void SolverDBM::setBoundary(const std::vector<uint8_t>& b) {
    if ((int)b.size() == N*N) boundary = b;
}

void SolverDBM::init(bool forceRecompute) {
    int R = N / 2 - 2;

    arc.assign(N*N, 0.0f);

    if (!boundary.empty()) {
        #ifdef _OPENMP
        #pragma omp parallel for collapse(2) schedule(static)
        #endif
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int idx = i*N + j;
                field[idx] = boundary[idx] ? 1.0f : 0.0f;
            }
        }
    } else {
        boundary.assign(N*N, 0);
        #ifdef _OPENMP
        #pragma omp parallel for collapse(2) schedule(static)
        #endif
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int idx = i*N + j;
                int dx = i - cx, dy = j - cy;
                double r = std::sqrt(dx*dx + dy*dy);
                if (r >= R) {
                    field[idx] = 1.0f;
                    boundary[idx] = 1;
                } else {
                    field[idx] = 0.0f;
                    boundary[idx] = 0;
                }
            }
        }
    }

    arc[cx*N + cy] = 1.0f;
    field[cx*N + cy] = 0.0f;

    checkForFieldFile(forceRecompute);
}

uint64_t SolverDBM::hashFieldF() const {
    uint64_t hash = 1469598103934665603ULL; // FNV offset basis
    const uint64_t prime = 1099511628211ULL;

    // mix N
    hash ^= static_cast<uint64_t>(N);
    hash *= prime;

    for (const auto& v : field) {
        uint32_t bits;
        static_assert(sizeof(bits) == sizeof(v));
        std::memcpy(&bits, &v, sizeof(v));
        hash ^= bits;
        hash *= prime;
    }

    for (const auto& b : boundary) {
        hash ^= static_cast<uint64_t>(b);
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
            in.read((char*)field.data(), field.size() * sizeof(float));
            in.read((char*)boundary.data(), boundary.size() * sizeof(uint8_t));
        }
    } else {
        if (forceRecompute) {
            std::cout << "Force recompute enabled, ignoring existing field file if any.\n";
        } else {
            std::cout << "No field file found at " << filename << ", initializing...\n";
        }
        computeFieldMultiscale();
        std::filesystem::create_directories("fields");
        std::ofstream out(filename, std::ios::binary);
        if (out) {
            out.write((char*)field.data(), field.size() * sizeof(float));
            out.write((char*)boundary.data(), boundary.size() * sizeof(uint8_t));
        }
    }
}

void SolverDBM::computeFieldMultiscale() {
    int maxStep = 1;
    while (maxStep * 2 < N) maxStep *= 2;

    for (int step = maxStep; step >= 1; step /= 2) {
        double max_diff;
        std::vector<float> new_field = field;
        do {
            max_diff = 0.0;

#ifdef _OPENMP
#pragma omp parallel for collapse(2) reduction(max:max_diff) schedule(static)
#endif
            for (int i = step; i < N - step; i += step) {
                for (int j = step; j < N - step; j += step) {
                    double field_val;
                    if (!computePointLaplace(i, j, step, field_val)) continue;
                    int idx = i*N + j;
                    double old = field[idx];
                    double diff = std::abs(field_val - old);
                    new_field[idx] = static_cast<float>(field_val);
                    if (diff > max_diff) max_diff = diff;
                }
            }

            field.swap(new_field);
        } while (max_diff > 1e-3);

        if (step > 1) interpolateLevel(step);
    }
}

bool SolverDBM::isFixed(int i, int j) const {
    int idx = i*N + j;
    return boundary[idx] || (arc[idx] != 0.0f);
}

bool SolverDBM::computePointLaplace(int x, int y, int step, double& out) const {
    int base = x * N + y;
    if (isFixed(x, y)) return false;
    int stride = step * N;
    double v = field[base - stride] + 
               field[base + stride] + 
               field[base - step] + 
               field[base + step];
    out = 0.25 * v;
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
                field[im*N + j0] = 0.5f * (field[i0*N + j0] + field[i1*N + j0]);

            if (!isFixed(i0, jm))
                field[i0*N + jm] = 0.5f * (field[i0*N + j0] + field[i0*N + j1]);

            if (!isFixed(im, jm))
                field[im*N + jm] = 0.25f * (
                    field[i0*N + j0] + field[i1*N + j0] + field[i0*N + j1] + field[i1*N + j1]
                );
        }
    }
}

void SolverDBM::solveLaplace() {
    computeFieldMultiscale();
}

std::vector<Point> SolverDBM::getCandidates() {
    std::vector<Point> out;
    if (N <= 2) return out;

    std::vector<uint8_t> added(N * N); // for candidates already found

    const int di[4] = {-1, 1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int idx = i * N + j;
            if (arc[idx] == 0.0f) continue; // early reject non-arc points

            for (int k = 0; k < 4; ++k) {
                int ni = i + di[k];
                int nj = j + dj[k];
                if (ni <= 0 || ni >= N-1 || nj <= 0 || nj >= N-1) continue; // no out of bounds
                int nidx = ni * N + nj;
                if (added[nidx]) continue; // already found
                if (!isFixed(ni, nj)) {
                    added[nidx] = 1;
                    out.push_back({ni, nj});
                }
            }
        }
    }

    return out;
}

std::vector<Point> SolverDBM::pick(std::vector<Point>& cands) {
    int numPicks = std::max(1, (int)std::ceil(0.0075 * cands.size()));
    std::vector<double> weights;
    weights.reserve(cands.size());
    for (auto& p : cands) {
        float v = field[p.x * N + p.y];
        double w = std::pow(std::max(0.0f, v), ETA);
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
    if (cands.empty()) return;
    auto picks = pick(cands);
    for (const auto& p : picks) {
        int idx = p.x * N + p.y;
        arc[idx] = 1.0f;
        field[idx] = 0.0f;
    }
}