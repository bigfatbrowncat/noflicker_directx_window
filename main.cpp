// Local headers
#include "D3DContext.h"
#include "DCompContext.h"

// OS headers
#include <Windows.h>

// C++ stl
#include <memory>
#include <mutex>

// Global declarations
std::shared_ptr<D3DContext> context;
std::shared_ptr<DCompContext> dcompContext;
bool exitPending;

/// <summary>
/// Passthrough (t) if truthy. Crash otherwise.
/// </summary>
template<class T> T win32_check(T t)
{
    if (t) return t;

    // Debuggers are better at displaying HRESULTs than the raw DWORD returned by GetLastError().
    HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
    while (true) __debugbreak();
}

void updateLayout(int width, int height) {
    // Uncomment this fake resizing load here to see how the app handles it
    // 100ms is a huge time pretty enough to recalculate even a very complicated layout
    //
    //Sleep(100);
}



/// <summary>
/// Win32 message handler.
/// </summary>
LRESULT window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    std::mutex draw_mutex;
    switch (message)
    {
        case WM_DESTROY: {
            // Destroy the DirectComposition context properly,
            // so that the window fades away beautifully.
            dcompContext->unbind();
        }

        case WM_CLOSE: {
            exitPending = true;
            return 0;
        }

        case WM_SIZING: {
            RECT& wr = *(RECT*)lparam;
            updateLayout(wr.right - wr.left, wr.bottom - wr.top);
            return 0;
        }

        case WM_NCCALCSIZE: {
            // Use the result of DefWindowProc's WM_NCCALCSIZE handler to get the upcoming client rect.
            // Technically, when wparam is TRUE, lparam points to NCCALCSIZE_PARAMS, but its first
            // member is a RECT with the same meaning as the one lparam points to when wparam is FALSE.
            DefWindowProc(hwnd, message, wparam, lparam);
            if (RECT *rect = (RECT *) lparam; rect->right > rect->left && rect->bottom > rect->top) {
                UINT width = rect->right - rect->left;
                UINT height = rect->bottom - rect->top;

                context->resize(width, height);
            }
            // We're never preserving the client area, so we always return 0.
            return 0;
        }

        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
    }
}

/// <summary>
/// The app entry point.
/// </summary>
int WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int)
{
    context = std::make_shared<D3DContext>();

    // Register the window class.
    WNDCLASS wc = {};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = hinstance;
    wc.hCursor = win32_check(LoadCursor(nullptr, IDC_ARROW));
    wc.lpszClassName = TEXT("D3DWindow");
    win32_check(RegisterClass(&wc));

    // Create the window. We can use WS_EX_NOREDIRECTIONBITMAP
    // since all our presentation is happening through DirectComposition.
    HWND hwnd = win32_check(CreateWindowEx(
            WS_EX_NOREDIRECTIONBITMAP, wc.lpszClassName, TEXT("D3D Window"),
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hinstance, nullptr));

    // The DCompContext creation/destruction is fundamentally asymmetric.
    // We are cleaning up the resources in WM_DESTROY, but should NOT create the object in WM_CREATE.
    // Instead, we create it here between construction of the window and showing it
    dcompContext = std::make_shared<DCompContext>(hwnd, context);

    // Show the window and enter the message loop.
    ShowWindow(hwnd, SW_SHOWNORMAL);

    exitPending = false;
    while (!exitPending)
    {
        MSG msg;
        win32_check(GetMessage(&msg, nullptr, 0, 0) > 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}