#pragma once

#if defined(USE_DX11)
#include <d3d11.h>
#include <dxgi1_2.h>
#elif defined(USE_DX12)
#include <d3d12.h>
#include <dxgi1_4.h>
#else
#error "You should set either USE_DX11 or USE_DX12"
#endif


/// <summary>
/// Crash if hr != S_OK.
/// </summary>
static void hr_check(HRESULT hr)
{
    if (hr == S_OK) return;
    while (true) __debugbreak();
}

struct D3DContext {
#if defined(USE_DX11)
    ID3D11Device *device;
    ID3D11DeviceContext *deviceContext;
    IDXGISwapChain1 *swapChain;
#elif defined(USE_DX12)
    struct FrameContext
    {
        ID3D12CommandAllocator* CommandAllocator;
        UINT64                  FenceValue;
    };

    static int const NUM_BACK_BUFFERS = 3;
    static int const NUM_FRAMES_IN_FLIGHT = 3;

    FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
    UINT                         g_frameIndex = 0;

    ID3D12Device* device;
    ID3D12DescriptorHeap* descriptorHeap;
    ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = nullptr;
    ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = nullptr;
    ID3D12CommandQueue*          g_pd3dCommandQueue = nullptr;
    ID3D12GraphicsCommandList*   g_pd3dCommandList = nullptr;
    ID3D12Fence*                 g_fence = nullptr;
    HANDLE                       g_fenceEvent = nullptr;
    UINT64                       g_fenceLastSignaledValue = 0;
    IDXGISwapChain3*             swapChain = nullptr;
    HANDLE                       g_hSwapChainWaitableObject = nullptr;
    ID3D12Resource*              g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

private:
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void WaitForLastSubmittedFrame();
    void FlushGPU();
    FrameContext* WaitForNextFrameResources();

public:

#else
#error "You should set either USE_DX11 or USE_DX12"
#endif

    D3DContext();
    //void bindToWindow(HWND hwnd);
    void resize(unsigned int width, unsigned int height);
    ~D3DContext();
};