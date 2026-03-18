#include <vector>
#include <cmath>
#include <random>
#include <iostream>

#include "dielectric_breakdown.cpp"

// main.cpp
int main() {
    constexpr int N = 201;  // Compile-time constant
    int arr[N][N];

    Cluster<N> cluster(arr);  // Pass the array
    
    cluster.compute();
    cluster.print();
    return 0;
}