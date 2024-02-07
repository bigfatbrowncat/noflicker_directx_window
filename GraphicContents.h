#pragma once

#include <string>
#include <vector>

struct Vertex {
    [[maybe_unused]] float x, y, z, r, g, b, a;     // "Maybe unused" because all the data is passed to the GPU
};

struct GraphicContents {
    virtual void updateLayout(int width, int height) = 0;
    virtual std::vector<Vertex> getVertices() = 0;
    virtual std::string getShader() = 0;
};
