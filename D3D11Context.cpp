#include "D3DContext.h"

#include "d3dcompiler.h"

#include <string>

struct Vertex { float x, y; };

void draw_triangle(int width, int height,
                   ID3D11Device* device,
                   ID3D11DeviceContext* device_context,
                   IDXGISwapChain1* swap_chain) {

    Vertex vertices[]{
            { 0.0f, 0.5f},
            { 0.5f,-0.5f},
            {-0.5f, -0.5f}
    };

    D3D11_BUFFER_DESC vb_desc;
    ZeroMemory(&vb_desc, sizeof(vb_desc));
    vb_desc.ByteWidth = sizeof(vertices);
    vb_desc.Usage = D3D11_USAGE_DEFAULT;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vb_desc.CPUAccessFlags = 0;
    vb_desc.MiscFlags = 0;
    vb_desc.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA vb_data;
    ZeroMemory(&vb_data, sizeof(vb_data));
    vb_data.pSysMem = vertices;

    ID3D11Buffer* vertex_buffer;
    device->CreateBuffer(&vb_desc, &vb_data, &vertex_buffer);

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;

    device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);

    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader* pixel_shader;
    ID3DBlob* result_blob;
    ID3DBlob* error_blob;

    //HRESULT hr = D3DCompileFromFile(L"D:\\Devel\\Projects\\sdl2-d3d9-dcomp\\PixelShader.hlsl", NULL, NULL, "main", "ps_4_0", D3DCOMPILE_DEBUG, 0, &result_blob, &error_blob);
    std::string pixel_shader_code = std::string(
            "float4 main() : SV_Target\n"
            "{\n"
            "    return float4(0.0f, 1.0f, 0.0f, 1.0f);\n"
            "}");

    hr_check(D3DCompile2(pixel_shader_code.c_str(), pixel_shader_code.length(),
                             nullptr,
                             NULL, NULL, "main", "ps_4_0", D3DCOMPILE_DEBUG, 0,
                             0, nullptr, 0,
                             &result_blob, &error_blob));

    device->CreatePixelShader(result_blob->GetBufferPointer(), result_blob->GetBufferSize(), nullptr, &pixel_shader);
    device_context->PSSetShader(pixel_shader, nullptr, 0);

    std::string vertex_shader_code = std::string(
            "float4 main(float2 pos : Position) : SV_Position\n"
            "{\n"
            "    return float4(pos.x, pos.y, 0.0f, 1.0f);\n"
            "}");

    hr_check(D3DCompile2(vertex_shader_code.c_str(), vertex_shader_code.length(),
                     nullptr,
                     NULL, NULL, "main", "vs_4_0", D3DCOMPILE_DEBUG, 0,
                     0, nullptr,  0,
                     &result_blob, &error_blob));

    device->CreateVertexShader(result_blob->GetBufferPointer(), result_blob->GetBufferSize(), nullptr, &vertex_shader);
    device_context->VSSetShader(vertex_shader, nullptr, 0);

    ID3D11InputLayout *input_layout;

    D3D11_INPUT_ELEMENT_DESC element_desc[] =
            {
                    {"Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
            };

    device->CreateInputLayout(element_desc, 1, result_blob->GetBufferPointer(), result_blob->GetBufferSize(), &input_layout);

    device_context->IASetInputLayout(input_layout);

    result_blob->Release();
    if (error_blob != nullptr) error_blob->Release();

    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_VIEWPORT viewport;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = width;
    viewport.Height = height;
    device_context->RSSetViewports(1u, &viewport);

    // Render to the target
    {
        ID3D11Resource *buffer;
        ID3D11RenderTargetView *rtv;

        FLOAT color[] = {0.0f, 0.2f, 0.4f, 1.0f};
        hr_check(swap_chain->GetBuffer(0, IID_PPV_ARGS(&buffer)));
        hr_check(device->CreateRenderTargetView(buffer, nullptr, &rtv));

        // After this point and before swapChin->Present(), we should render as fast as possible
        device_context->ClearRenderTargetView(rtv, color);

        device_context->OMSetRenderTargets(1, &rtv, NULL);
        device_context->Draw(3, 0);

        buffer->Release();
        rtv->Release();
    }

    vertex_buffer->Release();
    vertex_shader->Release();
    pixel_shader->Release();
    input_layout->Release();
}


D3DContext::D3DContext(): device(nullptr), deviceContext(nullptr), swapChain(nullptr) {
    // Create the DXGI factory.
    IDXGIFactory2* dxgi;
    hr_check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)));

    // Create the D3D device.
    hr_check(D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &deviceContext));

    // Create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 scd = {};
    // Just use a minimal size for now. WM_NCCALCSIZE will resize when necessary.
    scd.Width = 1;
    scd.Height = 1;
    scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    // TODO: Determine if PRESENT_DO_NOT_SEQUENCE is safe to use with SWAP_EFFECT_FLIP_DISCARD.
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    hr_check(dxgi->CreateSwapChainForComposition(device, &scd, nullptr, &swapChain));
}

void D3DContext::resize(unsigned int width, unsigned int height) {
    // A real app might want to compare these dimensions with the current swap chain
    // dimensions and skip all this if they're unchanged.
    hr_check(swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));
    /*if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        // You have to destroy the device, swapchain, and all resources and
        // recreate them to recover from this case. The device was hardware reset,
        // physically removed, or the driver was updated and/or restarted
        throw std::logic_error("Device recreation not supported yet");
    }*/

    // Do some minimal rendering to prove this works.
//    ID3D11Resource* buffer;
//    ID3D11RenderTargetView* rtv;
//    FLOAT color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
//    hr_check(swapChain->GetBuffer(0, IID_PPV_ARGS(&buffer)));
//    hr_check(device->CreateRenderTargetView(buffer, nullptr, &rtv));
//    deviceContext->ClearRenderTargetView(rtv, color);
//
//    D3D11_VIEWPORT viewport;
//    viewport.MinDepth = 0;
//    viewport.MaxDepth = 1;
//    viewport.TopLeftX = 0;
//    viewport.TopLeftY = 0;
//    viewport.Width = width;
//    viewport.Height = height;
//    deviceContext->RSSetViewports(1u, &viewport);
//    deviceContext->OMSetRenderTargets( 1, &rtv, NULL );

    draw_triangle(width, height, device, deviceContext,  swapChain);

    //buffer->Release();
    //rtv->Release();


    // Discard outstanding queued presents and queue a frame with the new size ASAP.
    hr_check(swapChain->Present(0, DXGI_PRESENT_RESTART));
    //Sleep(500);
    // Wait for a vblank to really make sure our frame with the new size is ready before
    // the window finishes resizing.
    // TODO: Determine why this is necessary at all. Why isn't one Present() enough?
    // TODO: Determine if there's a way to wait for vblank without calling Present().
    // TODO: Determine if DO_NOT_SEQUENCE is safe to use with SWAP_EFFECT_FLIP_DISCARD.
    hr_check(swapChain->Present(1, DXGI_PRESENT_DO_NOT_SEQUENCE));
}

D3DContext::~D3DContext() {
    if (swapChain) { swapChain->SetFullscreenState(false, nullptr); swapChain->Release(); swapChain = nullptr; }
    if (deviceContext) { deviceContext->Release(); deviceContext = nullptr; }
    if (device) { device->Release(); device = nullptr; }
}