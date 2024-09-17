#pragma once

#include <string>
#include <vector>

struct TextureVertex {
    [[maybe_unused]] float x, y, z, tx, ty;   //, r, g, b, a;     // "Maybe unused" because all the data is passed to the GPU
};
struct RGBAVertex {
	[[maybe_unused]] float x, y, z, r, g, b, a;     // "Maybe unused" because all the data is passed to the GPU
};

template <typename V> struct _GraphicContents {
    virtual void updateLayout(int width, int height) = 0;
    virtual std::vector<V> getVertices() = 0;
    virtual std::string getShader() = 0;
};

#if defined(USE_DX11)
typedef _GraphicContents<TextureVertex> GraphicContents;
#elif defined(USE_DX12)
typedef _GraphicContents<RGBAVertex> GraphicContents;
#else
#error "You should set either USE_DX11 or USE_DX12"
#endif
