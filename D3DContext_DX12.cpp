#include "D3DContext.h"
//#include "DDSTextureLoader12.h"

#include <d3dcompiler.h>

#include <vector>
#include <string>
#include <iostream>

//using namespace DirectX;

// The debug layer is broken and crashes the app. Don't enable it
//#ifdef _DEBUG
//#define DX12_ENABLE_DEBUG_LAYER
//#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif


void D3DContext::DrawTriangle(int width, int height,
                   ID3D12Device* device,
                   ID3D12GraphicsCommandList* graphics_command_list,
                   ID3D12CommandQueue* command_queue,
                   IDXGISwapChain3* swap_chain,
                   ID3D12Resource* mainRenderTargetResource,
                   D3D12_CPU_DESCRIPTOR_HANDLE& mainRenderTargetDescriptor,
				   FrameContext* frameCtx,
                   D3DContext::DrawingCache* drawing_cache,
                   std::shared_ptr<GraphicContents> contents) {
    auto vertices = contents->getVertices();

    D3D12_RESOURCE_DESC vb_desc = {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = sizeof(RGBAVertex) * vertices.size(),
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = { .Count = 1, .Quality = 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    if (drawing_cache->vertex_buffer == nullptr) {
        D3D12_HEAP_PROPERTIES heap_props = {
                .Type = D3D12_HEAP_TYPE_UPLOAD
        };

        hr_check(device->CreateCommittedResource(
                &heap_props,
                D3D12_HEAP_FLAG_NONE,
                &vb_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&drawing_cache->vertex_buffer)));
    }

    {
        void *gpu_data = nullptr;
        D3D12_RANGE read_range = {0, 0}; // CPU isn't going to read this data, only write
        hr_check(drawing_cache->vertex_buffer->Map(0, &read_range, &gpu_data));
        memcpy(gpu_data, &vertices[0], vb_desc.Width);
		drawing_cache->vertex_buffer->Unmap(0, nullptr);
    }

    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {
            .BufferLocation = drawing_cache->vertex_buffer->GetGPUVirtualAddress(),
            .SizeInBytes = static_cast<UINT>(sizeof(RGBAVertex) * vertices.size()),
            .StrideInBytes = sizeof(RGBAVertex)
    };

    ID3D12RootSignature *rootSignature;

    {
        ID3DBlob *vs, *vs_error;
        ID3DBlob *ps, *ps_error;

        std::string shader_code = contents->getShader();

        HRESULT hr;
        hr = D3DCompile2(shader_code.c_str(), shader_code.length(),
                             nullptr,
                             nullptr, nullptr, "PSMain", "ps_4_0", D3DCOMPILE_DEBUG, 0,
                             0, nullptr, 0,
                             &ps, &ps_error);
        if ( FAILED(hr) )
        {
            if ( ps_error )
            {
                std::cerr << "Pixel Shader Compilation Failed: " << (char*)ps_error->GetBufferPointer() << std::endl;
                ps_error->Release();
            }
            hr_check(hr);
        }

        hr = D3DCompile2(shader_code.c_str(), shader_code.length(),
                             nullptr,
                             nullptr, nullptr, "VSMain", "vs_4_0", D3DCOMPILE_DEBUG, 0,
                             0, nullptr, 0,
                             &vs, &vs_error);
        if ( FAILED(hr) )
        {
            if ( ps_error )
            {
                std::cerr << "Vertex Shader Compilation Failed: " << (char*)ps_error->GetBufferPointer() << std::endl;
                ps_error->Release();
            }
            hr_check(hr);
        }

        D3D12_INPUT_ELEMENT_DESC vertexFormat[] =
        {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        const D3D12_RENDER_TARGET_BLEND_DESC defaultBlendState = {
                .BlendEnable = FALSE,
                .LogicOpEnable = FALSE,

                .SrcBlend = D3D12_BLEND_ONE,
                .DestBlend = D3D12_BLEND_ZERO,
                .BlendOp = D3D12_BLEND_OP_ADD,

                .SrcBlendAlpha = D3D12_BLEND_ONE,
                .DestBlendAlpha = D3D12_BLEND_ZERO,
                .BlendOpAlpha = D3D12_BLEND_OP_ADD,

                .LogicOp = D3D12_LOGIC_OP_NOOP,
                .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        };

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {
                .Version = D3D_ROOT_SIGNATURE_VERSION_1_0,
                .Desc_1_0 = {
                        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
                },
        };

        {
            ID3DBlob* serializedDesc = nullptr;
            hr_check(D3D12SerializeVersionedRootSignature(&desc, &serializedDesc, nullptr));

            hr_check(device->CreateRootSignature(0,
                                                 serializedDesc->GetBufferPointer(),
                                                 serializedDesc->GetBufferSize(),
                                                 IID_PPV_ARGS(&rootSignature)));

            serializedDesc->Release();
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {
                .pRootSignature = rootSignature,
                .VS = {
                        .pShaderBytecode = vs->GetBufferPointer(),
                        .BytecodeLength = vs->GetBufferSize(),
                },
                .PS = {
                        .pShaderBytecode = ps->GetBufferPointer(),
                        .BytecodeLength = ps->GetBufferSize(),
                },
                .StreamOutput = {0},
                .BlendState = {
                        .AlphaToCoverageEnable = FALSE,
                        .IndependentBlendEnable = FALSE,
                        .RenderTarget = {defaultBlendState},
                },
                .SampleMask = 0xFFFFFFFF,
                .RasterizerState = {
                        .FillMode = D3D12_FILL_MODE_SOLID,
                        .CullMode = D3D12_CULL_MODE_BACK,
                        .FrontCounterClockwise = FALSE,
                        .DepthBias = 0,
                        .DepthBiasClamp = 0,
                        .SlopeScaledDepthBias = 0,
                        .DepthClipEnable = TRUE,
                        .MultisampleEnable = FALSE,
                        .AntialiasedLineEnable = FALSE,
                        .ForcedSampleCount = 0,
                        .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
                },
                .DepthStencilState = {
                        .DepthEnable = FALSE,
                        .StencilEnable = FALSE,
                },
                .InputLayout = {
                        .pInputElementDescs = vertexFormat,
                        .NumElements = sizeof(vertexFormat) / sizeof(D3D12_INPUT_ELEMENT_DESC)//vertexFormat_count,
                },
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
                .DSVFormat = DXGI_FORMAT_UNKNOWN,
                .SampleDesc = {
                        .Count = 1,
                        .Quality = 0,
                },
        };

		if (drawing_cache->pipeline == nullptr) {
			hr_check(device->CreateGraphicsPipelineState(
					&pipelineStateDesc, IID_PPV_ARGS(&drawing_cache->pipeline)));
		}

        vs->Release();
        ps->Release();
    }

    // Render to the target
    {
        hr_check(frameCtx->CommandAllocator->Reset());
        hr_check(graphics_command_list->Reset(frameCtx->CommandAllocator, drawing_cache->pipeline));

        D3D12_VIEWPORT viewport;
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = (float) width;
        viewport.Height = (float) height;

        const D3D12_RECT scissorRect = {
                .left = 0,
                .top = 0,
                .right = width,
                .bottom = height,
        };

        graphics_command_list->SetGraphicsRootSignature(rootSignature);
        graphics_command_list->RSSetViewports(1, &viewport);
        graphics_command_list->RSSetScissorRects(1, &scissorRect);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = mainRenderTargetResource; //g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        //graphics_command_list->Reset(frameCtx->CommandAllocator,  );
        graphics_command_list->ResourceBarrier(1, &barrier);

        graphics_command_list->OMSetRenderTargets(1, &mainRenderTargetDescriptor, FALSE, nullptr);

        FLOAT color[] = {0.0f, 0.2f, 0.4f, 1.0f};
        graphics_command_list->ClearRenderTargetView(mainRenderTargetDescriptor,
                                                     color /*clear_color_with_alpha*/, 0, nullptr);
		graphics_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //graphics_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);//D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        graphics_command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);

        // Finally drawing the bloody triangle!
		graphics_command_list->DrawInstanced((UINT)vertices.size(), 1, 0, 0);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        graphics_command_list->ResourceBarrier(1, &barrier);
        graphics_command_list->Close();

        command_queue->ExecuteCommandLists(1, (ID3D12CommandList *const *) &graphics_command_list);
    }

	rootSignature->Release();
}


static void bool_check(bool res)
{
    if (res) return;
    while (true) __debugbreak();
}


bool D3DContext::CreateDeviceD3D(/*HWND hWnd*/)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 scd = {};
    // Just use a minimal size for now. WM_NCCALCSIZE will resize when necessary.
    scd.Width = 1920;
    scd.Height = 1080;
    scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = NUM_BACK_BUFFERS;
    // TODO: Determine if PRESENT_DO_NOT_SEQUENCE is safe to use with SWAP_EFFECT_FLIP_DISCARD.
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&device)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK)// ||
        /*g_pd3dCommandList->Close() != S_OK)*/
        return false;

    g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
    g_pd3dCommandList->Close();

    if (device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;
        if (dxgiFactory->CreateSwapChainForComposition(g_pd3dCommandQueue, &scd, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        swapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        //g_hSwapChainWaitableObject = swapChain->GetFrameLatencyWaitableObject();
        //g_hSwapChainWaitableObject = swapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void D3DContext::CleanupDeviceD3D()
{
    CleanupRenderTarget();
	if (imageTextureView) { imageTextureView->Release(); imageTextureView = nullptr; }
	if (swapChain != nullptr) { swapChain->SetFullscreenState(false, nullptr); swapChain->Release(); swapChain = nullptr; }
    if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
    for (auto & i : g_frameContext) {
        if (i.CommandAllocator) {
            i.CommandAllocator->Release();
            i.CommandAllocator = nullptr;
        }
    }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (device) { device->Release(); device = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void D3DContext::CreateRenderTarget()
{
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        swapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        device->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void D3DContext::CleanupRenderTarget()
{
    FlushGPU();

    for (auto & i : g_mainRenderTargetResource)
        if (i) { i->Release(); i = nullptr; }
}

void D3DContext::FlushGPU() {
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];
    for (int i = 0; i < NUM_BACK_BUFFERS; i++) {

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;

        WaitForLastSubmittedFrame();
    }
}

void D3DContext::WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue)
        return;

    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

D3DContext::FrameContext* D3DContext::WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;

    g_hSwapChainWaitableObject = swapChain->GetFrameLatencyWaitableObject();
    HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, nullptr };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        waitableObjects[1] = g_fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
}

D3DContext::D3DContext(std::shared_ptr<GraphicContents> contents): D3DContextBase(contents), swapChain(nullptr), descriptorHeap(nullptr) {
    bool_check(CreateDeviceD3D());

//	hr_check(LoadDDSTextureFromFile( this->device, L"grass.dds", &imageTextureView, ddsData, subresources ));
//	HRESULT __cdecl LoadDDSTextureFromFile(
//			_In_ ID3D12Device* d3dDevice,
//			_In_z_ const wchar_t* szFileName,
//			_Outptr_ ID3D12Resource** texture,
//			std::unique_ptr<uint8_t[]>& ddsData,
//			std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
//			size_t maxsize = 0,
//			_Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr,
//			_Out_opt_ bool* isCubeMap = nullptr);

	this->reposition(getFullDisplayRECT());
}

void D3DContext::reposition(const RECT& position) {
	int width = position.right - position.left;
	int height = position.bottom - position.top;

	lookForIntelOutput(position);
	CleanupRenderTarget();
	checkDeviceRemoved(swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE));//0/*DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT*/));
    CreateRenderTarget();

    FrameContext* frameCtx = WaitForNextFrameResources();

	UINT backBufferIdx = swapChain->GetCurrentBackBufferIndex();
	DrawTriangle(width, height, device, this->g_pd3dCommandList, this->g_pd3dCommandQueue, swapChain,
								 g_mainRenderTargetResource[backBufferIdx],
								 g_mainRenderTargetDescriptor[backBufferIdx],
								 frameCtx, &this->drawingCache, contents);

	syncIntelOutput();

	// Discard outstanding queued presents and queue a frame with the new size ASAP.
    checkDeviceRemoved(swapChain->Present(0, DXGI_PRESENT_RESTART));

	UINT64 fenceValue = g_fenceLastSignaledValue + 1;
    g_pd3dCommandQueue->Signal(g_fence, fenceValue);
    g_fenceLastSignaledValue = fenceValue;
    frameCtx->FenceValue = fenceValue;

	// Wait for a vblank to really make sure our frame with the new size is ready before
    // the window finishes resizing.
    // TODO: Determine why this is necessary at all. Why isn't one Present() enough?
    // TODO: Determine if there's a way to wait for vblank without calling Present().
    // TODO: Determine if DO_NOT_SEQUENCE is safe to use with SWAP_EFFECT_FLIP_DISCARD.
    checkDeviceRemoved(swapChain->Present(1, DXGI_PRESENT_DO_NOT_SEQUENCE));
}

D3DContext::~D3DContext() {
    CleanupDeviceD3D();
}