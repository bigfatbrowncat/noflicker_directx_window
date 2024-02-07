// Local headers
#include "D3DContext.h"
#include "DCompContext.h"
#include "GraphicContents.h"

// OS headers
#include <Windows.h>

// C++ stl
#include <memory>
#include <mutex>

class TriangleGraphicContents : public GraphicContents {
private:
    int width = 0, height = 0;

public:
    void updateLayout(int width, int height) override {
        this->width = width; this->height = height;

        // Uncomment this fake resizing load here to see how the app handles it
        // 100ms is a huge time pretty enough to recalculate even a very complicated layout
        //
        // Sleep(100);
    }

    std::vector<Vertex> getVertices() override {
        float aspect = (float) width / (float) height;
        float k = 760.f / (float) width;
        float sin60 = sqrtf(3.f) / 2;
        std::vector<Vertex> vertices = {
            {0.0f * k,  0.5f * sin60 * aspect * k,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
            {0.5f * k,  -0.5f * sin60 * aspect * k, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f},
            {-0.5f * k, -0.5f * sin60 * aspect * k, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}
        };
        return vertices;
    }

    std::string getShader() override {
        return {
            "struct PSInput {\n"
            "	float4 position : SV_POSITION;\n"
            "	float4 color : COLOR;\n"
            "};\n"
            "PSInput VSMain(float4 position : POSITION0, float4 color : COLOR0) {\n"
            "	PSInput result;\n"
            "	result.position = position;\n"
            "	result.color = color;\n"
            "	return result;\n"
            "}\n"
            "float4 PSMain(PSInput input) : SV_TARGET {\n"
            "	return input.color;\n"
            "}\n"
        };
    }
};

// Global declarations
std::shared_ptr<D3DContext> context;
std::shared_ptr<DCompContext> dcompContext;
std::shared_ptr<TriangleGraphicContents> contents;
bool exitPending;

// Passthrough (t) if truthy. Crash otherwise.
template<class T> T win32_check(T t)
{
    if (t) return t;

    // Debuggers are better at displaying HRESULTs than the raw DWORD returned by GetLastError().
    HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
    while (true) __debugbreak();
}


// Win32 message handler.
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

        case WM_NCCALCSIZE: {
            // Use the result of DefWindowProc's WM_NCCALCSIZE handler to get the upcoming client rect.
            // Technically, when wparam is TRUE, lparam points to NCCALCSIZE_PARAMS, but its first
            // member is a RECT with the same meaning as the one lparam points to when wparam is FALSE.
            DefWindowProc(hwnd, message, wparam, lparam);
            if (RECT *rect = (RECT *) lparam; rect->right > rect->left && rect->bottom > rect->top) {
                contents->updateLayout(rect->right - rect->left, rect->bottom - rect->top);
                context->reposition(*rect);
            }
            // We're never preserving the client area, so we always return 0.
            return 0;
        }

        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
    }
}

// The app entry point.
int WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int)
{
    contents = std::make_shared<TriangleGraphicContents>();
    context = std::make_shared<D3DContext>(contents);

    // Register the window class.
    WNDCLASS wc = {};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = hinstance;
    wc.hCursor = win32_check(LoadCursor(nullptr, IDC_ARROW));
    wc.lpszClassName = TEXT("D3DWindow");
    wc.cbClsExtra = sizeof(void*);  // Extra pointter
    win32_check(RegisterClass(&wc));

    std::wstring windowTitle = L"A Never Flickering DirectX Window";
#if defined(USE_DX11)
    windowTitle += L" [Direct3D 11]";
#elif defined(USE_DX12)
    windowTitle += L" [Direct3D 12]";
#else
    #error "Either USE_DX11 or USE_DX12 should be chosen"
#endif

    // Create the window. We can use WS_EX_NOREDIRECTIONBITMAP
    // since all our presentation is happening through DirectComposition.
    HWND hwnd = win32_check(CreateWindowEx(
            WS_EX_NOREDIRECTIONBITMAP, wc.lpszClassName, windowTitle.c_str(),
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