#pragma once

#include <vector>
#include <random>

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

    void solveLaplace(size_t ITER);
    std::vector<Point> getCandidates();
    Point pick(const std::vector<Point>& cands);
public:
    Cluster(std::vector<float>& arr, int N);

    void init();
    void step(size_t ITER);
    void compute();
    void print();
};