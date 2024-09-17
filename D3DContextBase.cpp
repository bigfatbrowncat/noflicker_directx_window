#include "D3DContext.h"

#include <iostream>
#include <utility>

bool D3DContextBase::checkRECTsIntersect(const RECT& r1, const RECT& r2) {
	return r1.left < r2.right && r2.left < r1.right &&
		   r1.top < r2.bottom && r2.top < r1.bottom;
}

bool D3DContextBase::checkRECTContainsPoint(const RECT& r, LONG x, LONG y) {
	return r.left < x && r.right > x &&
		   r.top < y && r.bottom > y;
}

void D3DContextBase::checkDeviceRemoved(HRESULT hr) const {
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost: reason code 0x%08X\n",
                  (hr == DXGI_ERROR_DEVICE_REMOVED) ? device->GetDeviceRemovedReason() : hr);
        std::cerr << buff << std::endl;
#endif
        // If the device was removed for any reason, a new device
        // and swap chain will need to be created.
        // TODO HandleDeviceLost();
    }
    else
    {
        // Any other failed result is a fatal fast-fail
        hr_check(hr);
    }
}

D3DContextBase::D3DContextBase(std::shared_ptr<GraphicContents> contents) : contents(std::move(contents)), device(nullptr) {
	// Create the DXGI factory.
	hr_check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

	// Look for the adapters (and specifically an Intel GPU) in the system
	const UINT INTEL_VENDOR_ID = 0x8086;
	IDXGIAdapter* adapter;
	for (UINT i = 0; dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
		DXGI_ADAPTER_DESC desc;
		adapters.push_back(adapter);
		hr_check(adapter->GetDesc(&desc));
		if (desc.VendorId == INTEL_VENDOR_ID) {
			this->intelAdapter = adapter;
			return;
		}
	}

	dxgiFactory->Release();
}

RECT D3DContextBase::getFullDisplayRECT() const {
	// Calculating the whole virtual desktop size
	IDXGIOutput* output;
	LONG leftmost = 0, topmost = 0, rightmost = 0, bottommost = 0;
	for (IDXGIAdapter* a : adapters) {
		for (UINT i = 0; a->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; i++) {
			DXGI_OUTPUT_DESC outputDesc;
			hr_check(output->GetDesc(&outputDesc));
			if (outputDesc.DesktopCoordinates.left < leftmost) leftmost = outputDesc.DesktopCoordinates.left;
			if (outputDesc.DesktopCoordinates.top < topmost) topmost = outputDesc.DesktopCoordinates.top;
			if (outputDesc.DesktopCoordinates.right > rightmost) rightmost = outputDesc.DesktopCoordinates.right;
			if (outputDesc.DesktopCoordinates.bottom > bottommost) bottommost = outputDesc.DesktopCoordinates.bottom;
			output->Release();
		}
	}

	return { leftmost, topmost, rightmost, bottommost };
}

// If there is an Intel adapter in the system, we have to synchronize with it manually,
// because we can face flickering instead. No idea, why, but "immediate"
// rendering on Intel GPU is still not immediate.
//
// This one should be called between the drawing function and the swapChain->Present() call
void D3DContextBase::lookForIntelOutput(const RECT& position) {
	if (intelAdapterFirstOutput != nullptr) {
		intelAdapterFirstOutput->Release();
		intelAdapterFirstOutput = nullptr;
	}
	if (intelAdapter != nullptr) {
		IDXGIOutput* output;
		for (UINT i = 0; intelAdapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; i++) {
			DXGI_OUTPUT_DESC outputDesc;
			hr_check(output->GetDesc(&outputDesc));
			if (outputDesc.Monitor != nullptr /*&&
				outputDesc.DesktopCoordinates.left <= position.right &&
				outputDesc.DesktopCoordinates.top <= position.bottom &&
				outputDesc.DesktopCoordinates.right >= position.right &&
				outputDesc.DesktopCoordinates.bottom >= position.bottom*/) {

				intelAdapterFirstOutput = output;
				//std::cout << i << std::endl;
				break;
			} else {
				output->Release();
			}
		}
	}
}

void D3DContextBase::syncIntelOutput() const {
	if (intelAdapterFirstOutput != nullptr) {
		hr_check(intelAdapterFirstOutput->WaitForVBlank());
	}
}

D3DContextBase::~D3DContextBase() {
	if (dxgiFactory != nullptr) dxgiFactory->Release();
    if (intelAdapterFirstOutput != nullptr) { intelAdapterFirstOutput->Release(); }
	for (IDXGIAdapter* a : adapters) { a->Release(); }
}

