#pragma once

#include <vector>
#include <random>
#include <cstdint>

struct Point { int x, y; };

class SolverDBM {
private:
    int STEPS = 2000;
    double ETA = 1.25;

    std::vector<float>& field;
    std::vector<float>& arc;
    std::vector<uint8_t> boundary; // internal copy, 0 or 1 for now

    int N;
    int cx, cy;

    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

    void checkForFieldFile(bool forceRecompute);
    uint64_t hashFieldF() const;
    void computeFieldMultiscale();
    bool isFixed(int i, int j) const;
    bool computePointLaplace(int x, int y, int step, double& out) const;
    void interpolateLevel(int step);
    void solveLaplace();
    std::vector<Point> getCandidates();
    std::vector<Point> pick(std::vector<Point>& cands);

public:
    SolverDBM(std::vector<float>& field, std::vector<float>& arc, int N, const std::vector<uint8_t>& boundary_in = std::vector<uint8_t>());

    void init(bool forceRecompute = false);
    void step();
    void setBoundary(const std::vector<uint8_t>& b);
};
