#pragma once

#include <Windows.h>

class Base {
public:
    // Crash if hr != S_OK.
    static void hr_check(HRESULT hr)
    {
        if (hr == S_OK) return;
        while (true) __debugbreak();
    }
};