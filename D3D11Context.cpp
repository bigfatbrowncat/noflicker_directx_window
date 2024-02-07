#include "D3DContext.h"

#include "d3dcompiler.h"

#include <string>
#include <vector>
#include <stdexcept>

void D3DContext::DrawTriangle(int width, int height,
                   ID3D11Device* device,
                   ID3D11DeviceContext* device_context,
                   IDXGISwapChain1* swap_chain,
                   std::shared_ptr<GraphicContents> contents) {

    auto vertices = contents->getVertices();

    D3D11_BUFFER_DESC vb_desc;
    ZeroMemory(&vb_desc, sizeof(vb_desc));
    vb_desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
    vb_desc.Usage = D3D11_USAGE_DEFAULT;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vb_desc.CPUAccessFlags = 0;
    vb_desc.MiscFlags = 0;
    vb_desc.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA vb_data;
    ZeroMemory(&vb_data, sizeof(vb_data));
    vb_data.pSysMem = &vertices[0];

    {
        ID3D11Buffer *vertex_buffer;
        const UINT stride = sizeof(Vertex);
        const UINT offset = 0;
        device->CreateBuffer(&vb_desc, &vb_data, &vertex_buffer);
        device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);

        {
            ID3D11InputLayout *input_layout;
            ID3D11VertexShader *vertex_shader;
            ID3D11PixelShader *pixel_shader;
            {
                ID3DBlob *vs, *vs_error;
                ID3DBlob *ps, *ps_error;

                std::string shader_code = contents->getShader();

                hr_check(D3DCompile2(shader_code.c_str(), shader_code.length(),
                                     nullptr,
                                     nullptr, nullptr, "PSMain", "ps_4_0", D3DCOMPILE_DEBUG, 0,
                                     0, nullptr, 0,
                                     &ps, &ps_error));

                if (ps_error != nullptr) {
                    throw std::runtime_error("Pixel shader compilation error");
                }

                hr_check(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixel_shader));
                device_context->PSSetShader(pixel_shader, nullptr, 0);

                hr_check(D3DCompile2(shader_code.c_str(), shader_code.length(),
                                     nullptr,
                                     nullptr, nullptr, "VSMain", "vs_4_0", D3DCOMPILE_DEBUG, 0,
                                     0, nullptr, 0,
                                     &vs, &vs_error));

                if (vs_error != nullptr) {
                    throw std::runtime_error("Vertex shader compilation error");
                }

                hr_check(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertex_shader));
                device_context->VSSetShader(vertex_shader, nullptr, 0);

                D3D11_INPUT_ELEMENT_DESC element_desc[] =
                {
                        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
                };

                hr_check(device->CreateInputLayout(element_desc, sizeof(element_desc) / sizeof(D3D11_INPUT_ELEMENT_DESC), vs->GetBufferPointer(), vs->GetBufferSize(), &input_layout));

                device_context->IASetInputLayout(input_layout);

                vs->Release();
                ps->Release();
                if (vs_error != nullptr) vs_error->Release();
                if (ps_error != nullptr) ps_error->Release();
            }

            device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            {
                D3D11_VIEWPORT viewport;
                viewport.MinDepth = 0;
                viewport.MaxDepth = 1;
                viewport.TopLeftX = 0;
                viewport.TopLeftY = 0;
                viewport.Width = (float) width;
                viewport.Height = (float) height;
                device_context->RSSetViewports(1u, &viewport);
            }

            FLOAT color[] = {0.0f, 0.2f, 0.4f, 1.0f};
            // Render to the target
            {
                ID3D11RenderTargetView *rtv;
                ID3D11Resource *buffer;

                hr_check(swap_chain->GetBuffer(0, IID_PPV_ARGS(&buffer)));
                hr_check(device->CreateRenderTargetView(buffer, nullptr, &rtv));

                // After this point and before swapChin->Present(), we should render as fast as possible
                device_context->ClearRenderTargetView(rtv, color);

                device_context->OMSetRenderTargets(1, &rtv, nullptr);
                device_context->Draw(3, 0);

                buffer->Release();
                rtv->Release();
            }

            vertex_shader->Release();
            pixel_shader->Release();
            input_layout->Release();
        }
        vertex_buffer->Release();
    }
}


D3DContext::D3DContext(std::shared_ptr<GraphicContents> contents): D3DContextBase(std::move(contents)), deviceContext(nullptr), swapChain(nullptr) {
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
    hr_check(dxgiFactory->CreateSwapChainForComposition(device, &scd, nullptr, &swapChain));

	this->reposition(getFullDisplayRECT());
}

void D3DContext::reposition(const RECT& position) {
	int width = position.right - position.left;
	int height = position.bottom - position.top;

	lookForIntelOutput(position);

	// A real app might want to compare these dimensions with the current swap chain
    // dimensions and skip all this if they're unchanged.
    checkDeviceRemoved(swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));

    DrawTriangle(width, height, device, deviceContext, swapChain, contents);

	syncIntelOutput();

    // Discard outstanding queued presents and queue a frame with the new size ASAP.
    checkDeviceRemoved(swapChain->Present(0, DXGI_PRESENT_RESTART));
    //Sleep(500);
    // Wait for a vblank to really make sure our frame with the new size is ready before
    // the window finishes resizing.
    // TODO: Determine why this is necessary at all. Why isn't one Present() enough?
    // TODO: Determine if there's a way to wait for vblank without calling Present().
    // TODO: Determine if DO_NOT_SEQUENCE is safe to use with SWAP_EFFECT_FLIP_DISCARD.
    checkDeviceRemoved(swapChain->Present(1, DXGI_PRESENT_DO_NOT_SEQUENCE));
}

D3DContext::~D3DContext() {
    if (swapChain) { swapChain->SetFullscreenState(false, nullptr); swapChain->Release(); swapChain = nullptr; }
    if (deviceContext) { deviceContext->Release(); deviceContext = nullptr; }
    if (device) { device->Release(); device = nullptr; }
}