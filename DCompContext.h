#pragma once

// Local headers
#include "D3DContext.h"

// OS headers
#include <dcomp.h>
#include <Windows.h>

// C++ stl
#include <memory>

struct DCompContext {
    IDCompositionDevice* dcomp;
    IDCompositionTarget* target;
    IDCompositionVisual* visual;

    // This function should NOT be called from the destructor, instead it has to be called in WM_DESTROY
    void unbind();

    DCompContext(HWND hwnd, const std::shared_ptr<D3DContext>& context);
};
