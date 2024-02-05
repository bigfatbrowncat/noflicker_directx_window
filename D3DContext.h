#pragma once

#include "Base.h"

#if defined(USE_DX11)
#include <d3d11.h>
#include <dxgi1_2.h>
#elif defined(USE_DX12)
#include <d3d12.h>
#include <dxgi1_4.h>
#else
#error "You should set either USE_DX11 or USE_DX12"
#endif

// The base class for the Direct3D contexts.
// Contains common (mostly DXGI) logic between the DirectX versions
struct D3DContextBase : public Base {
	IDXGIAdapter* intelAdapter = nullptr;
	IDXGIFactory2* dxgiFactory = nullptr;

	static bool checkRECTsIntersect(const RECT& r1, const RECT& r2);
	static bool checkRECTContainsPoint(const RECT& r, LONG x, LONG y);

	D3DContextBase();

	// If there is an Intel adapter in the system, we have to synchronize with it manually,
	// because we can face flickering instead. No idea, why, but "immediate"
	// rendering on Intel GPU is still not immediate.
	//
	// This one should be called between the drawing function and the swapChain->Present() call
	void syncIntelGPU(const RECT& position) const;

    static void checkDeviceRemoved(HRESULT hr);

    virtual ~D3DContextBase();
};

// The Direct3D-specific context. Depends on the DirectX version flags
struct D3DContext : public D3DContextBase {
#if defined(USE_DX11)
    ID3D11Device *device;
    ID3D11DeviceContext *deviceContext;
    IDXGISwapChain1 *swapChain;
    void DrawTriangle(int width, int height,
                              ID3D11Device* device,
                              ID3D11DeviceContext* device_context,
                              IDXGISwapChain1* swap_chain);

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

    ID3D12Resource* vertex_buffer = nullptr;
private:
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void WaitForLastSubmittedFrame();
    void FlushGPU();
    FrameContext* WaitForNextFrameResources();
    ID3D12Resource* DrawTriangle(int width, int height,
                      ID3D12Device* device,
                      ID3D12GraphicsCommandList* graphics_command_list,
                      ID3D12CommandQueue* command_queue,
                      IDXGISwapChain3* swap_chain, FrameContext* frameCtx, ID3D12Resource* vertex_buffer);
public:

#else
#error "You should set either USE_DX11 or USE_DX12"
#endif

    D3DContext();
    void reposition(const RECT& position);
    ~D3DContext() override;
};