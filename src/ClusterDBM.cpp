#include "ClusterDBM.hpp"

#include <vector>
#include <cmath>
#include <random>
#include <iostream>

Cluster::Cluster(std::vector<float>& arr, std::vector<float>& arc, int N)
        : arr(arr), arc(arc), N(N), cx(N/2), cy(N/2),
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
}

void Cluster::solveLaplace(size_t ITER) {
    for (int it = 0; it < ITER; ++it)
        for (int i = 1; i < N-1; ++i)
            for (int j = 1; j < N-1; ++j)
                if (!grid[i*N+j].boundary && !grid[i*N+j].cluster)
                    grid[i*N+j].f = 0.25 * (grid[(i-1)*N+j].f + grid[(i+1)*N+j].f + 
                                            grid[i*N+(j-1)].f + grid[i*N+(j+1)].f);
}

// TODO instead of looking  at all of them only look at the points part of the cluster
std::vector<Point> Cluster::getCandidates() {
    std::vector<Point> out;
    for (int i = 1; i < N-1; ++i)
        for (int j = 1; j < N-1; ++j){
            arr[i*N+j] = grid[i*N+j].f;
            if (!grid[i*N+j].cluster){
                if (grid[(i-1)*N+j].cluster || grid[(i+1)*N+j].cluster || 
                        grid[i*N+(j-1)].cluster || grid[i*N+(j+1)].cluster)
                    out.push_back({i,j});
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
    arc[p.x*N+p.y] = 1;
    
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