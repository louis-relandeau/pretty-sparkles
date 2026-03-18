#include <vector>
#include <cmath>
#include <random>
#include <iostream>

struct Cell {
    double f = 0.0;
    bool cluster = false;
    bool boundary = false;
};

template<int N>
class Cluster {
private:
    int STEPS = 2000;
    double ETA = 1.25;
    int ITER = 5;
    
    std::vector<std::vector<Cell>> grid;
    int  (&arr)[N][N];
    int cx, cy;
    std::mt19937 rng{45};
    std::uniform_real_distribution<double> dist{0.0, 1.0};

public:
    Cluster(int (&externalArr)[N][N])
        : arr(externalArr), cx(N/2), cy(N/2),
          grid(N, std::vector<Cell>(N)) {}

    void init() {
        int R = N / 2 - 2;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int dx = i - cx, dy = j - cy;
                double r = std::sqrt(dx*dx + dy*dy);
                if (r >= R) {
                    grid[i][j].f = 1.0;
                    grid[i][j].boundary = true;
                    arr[i][j] = 1;
                }
            }
        }
        grid[cx][cy].cluster = true;
        grid[cx][cy].f = 0.0;
        arr[cx][cy] = 1;
    }

    void solveLaplace() {
        for (int it = 0; it < ITER; ++it)
            for (int i = 1; i < N-1; ++i)
                for (int j = 1; j < N-1; ++j)
                    if (!grid[i][j].boundary && !grid[i][j].cluster)
                        grid[i][j].f = 0.25 * (grid[i-1][j].f + grid[i+1][j].f + 
                                               grid[i][j-1].f + grid[i][j+1].f);
    }

    std::vector<std::pair<int,int>> getCandidates() {
        std::vector<std::pair<int,int>> out;
        for (int i = 1; i < N-1; ++i)
            for (int j = 1; j < N-1; ++j)
                if (!grid[i][j].cluster && 
                    (grid[i-1][j].cluster || grid[i+1][j].cluster || 
                     grid[i][j-1].cluster || grid[i][j+1].cluster))
                    out.push_back({i,j});
        return out;
    }

    std::pair<int,int> pick(const std::vector<std::pair<int,int>>& cands) {
        std::vector<double> probs;
        double sum = 0.0;
        for (auto& p : cands) {
            double val = std::pow(grid[p.first][p.second].f, ETA);
            probs.push_back(val);
            sum += val;
        }
        for (auto& p : probs) p /= sum;
        double r = dist(rng), acc = 0.0;
        for (int i = 0; i < (int)cands.size(); ++i)
            if ((acc += probs[i]) >= r) return cands[i];
        return cands.back();
    }

    void step() {
        solveLaplace();
        auto cands = getCandidates();
        if (cands.empty()) return;
        auto p = pick(cands);
        grid[p.first][p.second].cluster = true;
        grid[p.first][p.second].f = 0.0;
        arr[p.first][p.second] = 1;
    }

    void compute() {
        std::cout << "computing growth steps...\n";
        for (int i = 0; i < STEPS; ++i) {
            std::cout << "Step " << i+1 << "/" << STEPS << "\r" << std::flush;
            step();
        }
    }

    void print() {
        for (int j = 0; j < N; ++j) {
            for (int i = 0; i < N; ++i)
                std::cout << (grid[i][j].cluster ? "#" : ".");
            std::cout << "\n";
        }
    }
};