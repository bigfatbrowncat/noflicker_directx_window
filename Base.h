#pragma once

#include <Windows.h>

class Base {
public:
    // Crash if hr != S_OK.
    static void hr_check(HRESULT hr)
    {
		// Ignore the "occluded" state as a success
		if (hr == DXGI_STATUS_OCCLUDED) return;

		if (hr == S_OK) return;
        while (true) __debugbreak();
    }
};