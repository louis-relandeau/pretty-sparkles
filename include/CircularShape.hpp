#pragma once
#include <vector>
class CircularShape {
private:
    int gridsize;
    int radius = (gridsize - 2)/2;
    int thickness = 5;
public:
    std::vector<float>& result;
    CircularShape(std::vector<float>& geometry, int N);
};
