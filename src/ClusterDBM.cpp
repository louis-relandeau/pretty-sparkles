#include "ClusterDBM.hpp"

#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cassert>

Cluster::Cluster(std::vector<float>& arr, int N)
        : arr(arr), N(N), cx(N/2), cy(N/2),
          grid(std::vector<Cell>(N*N)),
          rng(45), dist(0.0, 1.0) {};

void Cluster::init() {
    int R = N / 2 - 2;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int dx = i - cx, dy = j - cy;
            double r = std::sqrt(dx*dx + dy*dy);
            if (r >= R) {
                grid[i*N+j].f = 1.0;
                grid[i*N+j].boundary = true;
                arr[i*N+j] = 1;
            } else {
                grid[i*N+j].f = 0;
                grid[i*N+j].boundary = false;
                arr[i*N+j] = 0;
            }
            grid[i*N+j].cluster = false;

        }
    }
    grid[cx*N+cy].cluster = true;
    grid[cx*N+cy].f = 0.0;
    arr[cx*N+cy] = 1;

    checkForFieldFile();
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

void Cluster::checkForFieldFile() {
    // Hash the grid field to encode size and boundary conditions, so we can reuse it across runs
    std::string hash = std::to_string(hashFieldF());
    std::string filename = "fields/field_" + hash + ".bin";
    if (std::filesystem::exists(filename)) {
        std::cout << "Loading field from " << filename << "\n";
        std::ifstream in(filename, std::ios::binary);
        in.read((char*)grid.data(), grid.size() * sizeof(Cell));
    } else {
        std::cout << "No field file found, initializing...\n";
        initializeField();
        std::ofstream out(filename, std::ios::binary);
        out.write((char*)grid.data(), grid.size() * sizeof(Cell));
    }
}
void Cluster::initializeField() {
    int maxStep = 1;

    // find largest power of two ≤ N
    while (maxStep * 2 < N) {
        maxStep *= 2;
    }

    size_t total_iters = 0;

    for (int step = maxStep; step >= 1; step /= 2) {

        std::cout << " Solving level step = " << step << ": ";

        double max_diff;
        size_t iter = 0;

        do {
            max_diff = 0.0;
            iter++;
            total_iters++;

            for (int i = step; i < N - step; i += step) {
                for (int j = step; j < N - step; j += step) {

                    auto field = computePointLaplace({i, j}, step);
                    if (!field) {
                        continue;
                    }

                    double old = grid[i*N + j].f;
                    double diff = std::abs(*field - old);

                    if (diff > max_diff) {
                        max_diff = diff;
                    }

                    grid[i*N + j].f = *field;
                }
            }

            std::cout
                << "   step=" << step
                << " iter=" << iter
                << " diff=" << max_diff
                << "\r" << std::flush;

        } while (max_diff > 1e-5);

        std::cout << "\n";

        if (step > 1) {
            interpolateLevel(step);
        }
    }

    std::cout << " Total iterations: " << total_iters << "\n";
}

bool Cluster::isFixed(int i, int j) {
    return grid[i*N+j].boundary || grid[i*N+j].cluster;
}

std::optional<double> Cluster::computePointLaplace(Point p, int step) {
    if (isFixed(p.x, p.y)) {
        return std::nullopt;
    }
    double v = grid[(p.x-step)*N+p.y].f + 
               grid[(p.x+step)*N+p.y].f + 
               grid[p.x*N+(p.y-step)].f +
               grid[p.x*N+(p.y+step)].f;  


    return 0.25 * v;
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

void Cluster::solveLaplace(size_t ITER) {
    for (int it = 0; it < ITER; ++it) {
        for (int i = 1; i < N-1; ++i) {
            for (int j = 1; j < N-1; ++j) {
                if (auto field = computePointLaplace({i,j}, 1)) {
                    grid[i*N+j].f = *field;
                }
            }
        }
    }
}

// TODO instead of looking  at all of them only look at the points part of the cluster
std::vector<Point> Cluster::getCandidates() {
    std::vector<Point> out;
    for (int i = 1; i < N-1; ++i) {
        for (int j = 1; j < N-1; ++j) {
            if (!grid[i*N+j].cluster) {
                arr[i*N+j] = grid[i*N+j].f;
                if (grid[(i-1)*N+j].cluster || grid[(i+1)*N+j].cluster || 
                        grid[i*N+(j-1)].cluster || grid[i*N+(j+1)].cluster)
                    out.push_back({i,j});
            }
        }
    }
            
    return out;
}

Point Cluster::pick(const std::vector<Point>& cands) {
    std::vector<double> probs;
    double sum = 0.0;
    for (auto& p : cands) {
        double val = std::pow(grid[p.x*N+p.y].f, ETA);
        probs.push_back(val);
        sum += val;
    }
    for (auto& p : probs) p /= sum;
    double r = dist(rng), acc = 0.0;
    for (int i = 0; i < (int)cands.size(); ++i)
        if ((acc += probs[i]) >= r) return cands[i];
    return cands.back();
}

void Cluster::step(size_t ITER = 10) {
    solveLaplace(ITER);
    auto cands = getCandidates();
    if (cands.empty()) return;
    auto p = pick(cands);
    grid[p.x*N+p.y].cluster = true;
    grid[p.x*N+p.y].f = 0.0;
    arr[p.x*N+p.y] = 1;
}

void Cluster::compute() {
    init();
    std::cout << "Computing growth steps...\n";
    for (int i = 0; i < STEPS; ++i) {
        std::cout << "Step " << i+1 << "/" << STEPS << "\r" << std::flush;
        step();
    }
}

void Cluster::print() {
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i)
            std::cout << (grid[i*N+j].cluster ? "#" : ".");
        std::cout << "\n";
    }
}