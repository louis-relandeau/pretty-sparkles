#pragma once

#include <vector>
#include <random>
#include <optional>
#include <cstdint>

struct Cell {
    double f;
    bool cluster;
    bool boundary;
};

struct Point {
    int x, y;
};

class Cluster {
private:
    int STEPS = 2000;
    double ETA = 1.25;

    std::vector<float>& arr;
    int N;
    int cx, cy;
    std::vector<Cell> grid;

    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

    void checkForFieldFile(bool forceRecompute);
    uint64_t hashFieldF();
    void computeFieldMultiscale();
    bool isFixed(int i, int j);
    // If nullopt, point is either boundary or cluster
    std::optional<double> computePointLaplace(Point p, int step);
    void interpolateLevel(int step);
    void solveLaplace();
    std::vector<Point> getCandidates();
    std::vector<Point> pick(std::vector<Point>& cands);

public:
    Cluster(std::vector<float>& arr, int N);

    void init(bool forceRecompute = false);
    void step();
};