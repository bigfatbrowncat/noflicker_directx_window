#pragma once

// Local headers
#include "D3DContext.h"
#include "Base.h"

// OS headers
#include <dcomp.h>
#include <Windows.h>

// C++ stl
#include <memory>

struct DCompContext : public Base {
    IDCompositionDevice* dcomp;
    IDCompositionTarget* target;
    IDCompositionVisual* visual;

    // This function should NOT be called from the destructor, instead it has to be called in WM_DESTROY
    void unbind();

	// Call this function immediately after the CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, ...)
    DCompContext(HWND hwnd, const std::shared_ptr<D3DContext>& context);
};
