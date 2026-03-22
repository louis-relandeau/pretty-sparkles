#include "ClusterDBM.hpp"

#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cassert>

Cluster::Cluster(std::vector<float>& arr, std::vector<float>& arc, int N)
        : arr(arr), arc(arc), N(N), cx(N/2), cy(N/2),
          grid(std::vector<Cell>(N*N)),
          rng(45), dist(0.0, 1.0) {};

void Cluster::init(bool forceRecompute) {
    int R = N / 2 - 2;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int dx = i - cx, dy = j - cy;
            double r = std::sqrt(dx*dx + dy*dy);
            if (r >= R) {
                grid[i*N+j].f = 1.0;
                grid[i*N+j].boundary = true;
                arc[i*N+j] = 1;
            } else {
                grid[i*N+j].f = 0;
                grid[i*N+j].boundary = false;
                arc[i*N+j] = 0;
            }
            grid[i*N+j].cluster = false;

        }
    }
    grid[cx*N+cy].cluster = true;
    grid[cx*N+cy].f = 0.0;
    arc[cx*N+cy] = 1;

    checkForFieldFile(forceRecompute);
}

uint64_t Cluster::hashFieldF() {
    uint64_t hash = 1469598103934665603ULL; // FNV offset basis

    for (const auto& cell : grid) {
        static_assert(sizeof(double) == 8);
        uint64_t bits;
        std::memcpy(&bits, &cell.f, sizeof(double));

        hash ^= bits;
        hash *= 1099511628211ULL; // FNV prime
    }

    return hash;
}

void Cluster::checkForFieldFile(bool forceRecompute) {
    // Hash the grid field to encode size and boundary conditions, so we can reuse it across runs
    std::string hash = std::to_string(hashFieldF());
    std::string filename = "fields/field_" + hash + ".bin";
    if (std::filesystem::exists(filename) && !forceRecompute) {
        std::cout << "Loading field from " << filename << "\n";
        std::ifstream in(filename, std::ios::binary);
        in.read((char*)grid.data(), grid.size() * sizeof(Cell));
    } else {
        if (forceRecompute) {
            std::cout << "Force recompute enabled, ignoring existing field file if any.\n";
        } else {
            std::cout << "No field file found at " << filename << ", initializing...\n";
        }
        computeFieldMultiscale();
        std::ofstream out(filename, std::ios::binary);
        out.write((char*)grid.data(), grid.size() * sizeof(Cell));
    }
}

void Cluster::computeFieldMultiscale() {
    int maxStep = 1;

    // find largest power of two ≤ N
    while (maxStep * 2 < N) {
        maxStep *= 2;
    }

    size_t total_iters = 0;

    for (int step = maxStep; step >= 1; step /= 2) {

        double max_diff;
        size_t iter = 0;

        do {
            max_diff = 0.0;
            iter++;
            total_iters++;

            for (int i = step; i < N - step; i += step) {
                for (int j = step; j < N - step; j += step) {

                    double field;
                    if (!computePointLaplace({i, j}, step, field)) {
                        continue;
                    }

                    double old = grid[i*N + j].f;
                    double diff = std::abs(field - old);

                    if (diff > max_diff) {
                        max_diff = diff;
                    }

                    grid[i*N + j].f = field;
                }
            }

        } while (max_diff > 1e-3);

        if (step > 1) {
            interpolateLevel(step);
        }
    }
}

bool Cluster::isFixed(int i, int j) {
    return grid[i*N+j].boundary || grid[i*N+j].cluster;
}

bool Cluster::computePointLaplace(const Point& p, int step, double& out) {
    const Cell* g = grid.data();
    int x = p.x;
    int y = p.y;
    int base = x * N + y;

    const Cell& c = g[base];
    if (c.boundary || c.cluster)
        return false;

    int stride = step * N;

    double v = g[base - stride].f +
               g[base + stride].f +
               g[base - step].f +
               g[base + step].f;

    out = 0.25 * v;
    return true;
}

void Cluster::interpolateLevel(int step) {
    int half = step / 2;

    for (int i = 0; i < N - step; i += step) {
        for (int j = 0; j < N - step; j += step) {

            int i0 = i;
            int j0 = j;
            int i1 = i + step;
            int j1 = j + step;

            int im = i + half;
            int jm = j + half;

            if (!isFixed(im, j0))
                grid[im*N + j0].f =
                    0.5 * (grid[i0*N + j0].f + grid[i1*N + j0].f);

            if (!isFixed(i0, jm))
                grid[i0*N + jm].f =
                    0.5 * (grid[i0*N + j0].f + grid[i0*N + j1].f);

            if (!isFixed(im, jm))
                grid[im*N + jm].f =
                    0.25 * (
                        grid[i0*N + j0].f +
                        grid[i1*N + j0].f +
                        grid[i0*N + j1].f +
                        grid[i1*N + j1].f
                    );
        }
    }
}

void Cluster::solveLaplace() {
    computeFieldMultiscale();
}

// TODO instead of looking  at all of them only look at the points part of the cluster
std::vector<Point> Cluster::getCandidates() {
    std::vector<Point> out;
    for (int i = 1; i < N-1; ++i) {
        for (int j = 1; j < N-1; ++j) {
            arr[i*N+j] = grid[i*N+j].f;
            if (!isFixed(i, j)) {
                if (grid[(i-1)*N+j].cluster || grid[(i+1)*N+j].cluster || 
                        grid[i*N+(j-1)].cluster || grid[i*N+(j+1)].cluster)
                    out.push_back({i,j});
            }
        }
    }
            
    return out;
}

std::vector<Point> Cluster::pick(std::vector<Point>& cands) {
    int numPicks = (int)std::ceil(0.0075 * cands.size());

    std::vector<double> weights;
    weights.reserve(cands.size());

    for (auto& p : cands) {
        weights.push_back(std::pow(grid[p.x * N + p.y].f, ETA));
    }

    std::vector<Point> picks;

    for (int n = 0; n < numPicks; ++n) {
        std::discrete_distribution<int> dist(weights.begin(), weights.end());
        int idx = dist(rng);

        picks.push_back(cands[idx]);

        cands.erase(cands.begin() + idx);
        weights.erase(weights.begin() + idx);
    }

    return picks;
}

void Cluster::step() {
    solveLaplace();
    auto cands = getCandidates();
    if (cands.empty()) {
        return;
    }
    auto picks = pick(cands);
    for (const auto& p : picks) {
        grid[p.x*N+p.y].cluster = true;
        grid[p.x*N+p.y].f = 0.0;
        arc[p.x*N+p.y] = 1;
    }
}