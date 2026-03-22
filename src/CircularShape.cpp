// Create a parent class of this to implement different shapes
#include <vector>
#include <iostream>
#include "CircularShape.hpp"

CircularShape::CircularShape(std::vector<float>& geometry, int N) : gridsize(N), result(geometry) {
    int cx = N / 2;
    int cy = N / 2;

    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j < N; j++) {
            int dx = i - cx;
            int dy = j - cy;
            if ((dx*dx + dy*dy < radius*radius) && (dx*dx + dy*dy > (radius-thickness)*(radius-thickness))){
                // std::cout << "Changed result" << std::endl;
                result[i*N+j] = 1;
            }else {
                result[i*N+j] = 0;
            }
            
        }
        
    }
}

// 