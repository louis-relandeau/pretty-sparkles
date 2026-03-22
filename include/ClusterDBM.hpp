#pragma once

#include <vector>
#include <random>
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
    std::vector<float>& arc;
    int N;
    int cx, cy;
    std::vector<Cell> grid;

    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

    void checkForFieldFile(bool forceRecompute);
    uint64_t hashFieldF();
    void computeFieldMultiscale();
    bool isFixed(int i, int j);
    bool computePointLaplace(const Point& p, int step, double& out);
    void interpolateLevel(int step);
    void solveLaplace();
    std::vector<Point> getCandidates();
    std::vector<Point> pick(std::vector<Point>& cands);

public:
    Cluster(std::vector<float>& arr, std::vector<float>& arc, int N);

    void init(bool forceRecompute = false);
    void step();
};