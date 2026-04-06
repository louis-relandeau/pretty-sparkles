#include <chrono>
#include <iostream>
#include <vector>
#include <functional>
#include <vector>
#include <iomanip>
#include <optional>

#include "ClusterDBM.hpp"

double measureTime(const std::function<void()>& func, bool quiet = true) {
    auto start = std::chrono::high_resolution_clock::now();
    // Redirect any output from func to /dev/null to avoid polluting benchmark results
    std::streambuf* orig_buf = std::cout.rdbuf();
    if (quiet) {
        std::cout.rdbuf(nullptr);
    }
    func();
    // Restore original buffer
    std::cout.rdbuf(orig_buf);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    return elapsed.count();
}

struct BenchmarkResult {
    std::string label;
    std::optional<double> time;
};

void printResults(std::string header, const std::vector<BenchmarkResult>& results) {
    std::cout <<  "========== " << header << "==========\n";

    size_t max_label = 0;
    for (auto& r : results) {
        max_label = std::max(max_label, r.label.size());
    }

    std::cout << "+-" << std::string(max_label, '-') << "-+----------+\n";
    std::cout << "| " << std::setw(max_label) << std::left << "Task" << " | Time(s)  |\n";
    std::cout << "+-" << std::string(max_label, '-') << "-+----------+\n";

    for (auto& r : results) {
        std::cout << "| " << std::setw(max_label) << std::left << r.label
                  << " | ";
        if (r.time.has_value()) {
            std::cout << std::setw(8) << std::fixed << std::setprecision(5) << r.time.value();
        } else {
            std::cout << "   N/A   ";
        }
        std::cout << " |\n";
    }

    std::cout << "+-" << std::string(max_label, '-') << "-+----------+\n";
}

int main() {
    std::vector<BenchmarkResult> results;
    bool quiet = true;
    
    /// ========== ClusterDBM Benchmark ==========
    
    const int N = 300;
    std::vector<float> arr(N*N, 0);
    std::vector<float> arc(N*N, 0);
    Cluster cluster(arr, arc, N);
    // Measure time to do the init with force recompute (ignoring any existing field file)
    results.push_back({"Initialization time with force recompute", measureTime([&]() {
        cluster.init(true /*forceRecompute*/);
    })});

    // Measure time to do the init without force recompute (should be faster if field file exists)
    results.push_back({"Initialization time without force recompute", measureTime([&]() {
        cluster.init(false /*forceRecompute*/);
    })});

    // Measure time to N steps
    int numSteps = 1000;
    results.push_back({"Time for " + std::to_string(numSteps) + " steps", measureTime([&]() {
        for (int i = 0; i < numSteps; ++i) {
            cluster.step();
        }
    }, quiet)});
    results.push_back({" |->FPS: " + std::to_string(numSteps / results.back().time.value()), std::nullopt});
    std::string header = "ClusterDBM Benchmark (N=" + std::to_string(N) + ")";
    printResults(header, results);
    results.clear();

    return 0;
}