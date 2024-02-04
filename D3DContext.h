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

struct D3DContextBase {
	IDXGIAdapter* intelAdapter = nullptr;
	IDXGIFactory2* dxgiFactory = nullptr;

	static bool checkRECTsIntersect(const RECT& r1, const RECT& r2) {
		return r1.left < r2.right && r2.left < r1.right &&
			   r1.top < r2.bottom && r2.top < r1.bottom;
	}

	D3DContextBase() {
		// Create the DXGI factory.
		hr_check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

		// Look for an Intel GPU in the system
		const UINT INTEL_VENDOR_ID = 0x8086;
		IDXGIAdapter* adapter;
		for (UINT i = 0; dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
			DXGI_ADAPTER_DESC desc;
			hr_check(adapter->GetDesc(&desc));
			if (desc.VendorId == INTEL_VENDOR_ID) {
				this->intelAdapter = adapter;
				return;
			}
		}

		dxgiFactory->Release();
	}

	// If there is an Intel adapter in the system, we have to synchronize with it manually,
	// because we can face flickering instead. No idea, why, but "immediate"
	// rendering on Intel GPU is still not immediate.
	//
	// This one should be called between the drawing function and the swapChain->Present() call
	void syncIntelGPU(const RECT& position) const {
		if (intelAdapter != nullptr) {
			IDXGIOutput* intelAdapterOutput;
			for (UINT i = 0; intelAdapter->EnumOutputs(i, &intelAdapterOutput) != DXGI_ERROR_NOT_FOUND; i++) {
				DXGI_OUTPUT_DESC outputDesc;
				hr_check(intelAdapterOutput->GetDesc(&outputDesc));
				if (checkRECTsIntersect(outputDesc.DesktopCoordinates, position)) {
					// If our window intersects this output, we have to wait for the output's VBlank
					hr_check(intelAdapterOutput->WaitForVBlank());
				}
				intelAdapterOutput->Release();
			}
		}
	}

	virtual ~D3DContextBase() {
		if (dxgiFactory != nullptr) dxgiFactory->Release();
		if (intelAdapter != nullptr) intelAdapter->Release();
	}
};

struct D3DContext : public D3DContextBase {
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
    void DrawTriangle(int width, int height,
                      ID3D12Device* device,
                      ID3D12GraphicsCommandList* graphics_command_list,
                      ID3D12CommandQueue* command_queue,
                      IDXGISwapChain3* swap_chain, FrameContext* frameCtx);
public:

#else
#error "You should set either USE_DX11 or USE_DX12"
#endif

    D3DContext();
    //void bindToWindow(HWND hwnd);
    void reposition(const RECT& position);
    ~D3DContext();
};