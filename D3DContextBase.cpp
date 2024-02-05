#include "D3DContext.h"

bool D3DContextBase::checkRECTsIntersect(const RECT& r1, const RECT& r2) {
	return r1.left < r2.right && r2.left < r1.right &&
		   r1.top < r2.bottom && r2.top < r1.bottom;
}

bool D3DContextBase::checkRECTContainsPoint(const RECT& r, LONG x, LONG y) {
	return r.left < x && r.right > x &&
		   r.top < y && r.bottom > y;
}

D3DContextBase::D3DContextBase() {
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
void D3DContextBase::syncIntelGPU(const RECT& position) const {
	if (intelAdapter != nullptr) {
		IDXGIOutput* intelAdapterOutput;
		for (UINT i = 0; intelAdapter->EnumOutputs(i, &intelAdapterOutput) != DXGI_ERROR_NOT_FOUND; i++) {
			DXGI_OUTPUT_DESC outputDesc;
			hr_check(intelAdapterOutput->GetDesc(&outputDesc));
			if (checkRECTContainsPoint(outputDesc.DesktopCoordinates, position.right, position.bottom)) {
				// If our window intersects this output, we have to wait for the output's VBlank
				hr_check(intelAdapterOutput->WaitForVBlank());
			}
			intelAdapterOutput->Release();
		}
	}
}

D3DContextBase::~D3DContextBase() {
	if (dxgiFactory != nullptr) dxgiFactory->Release();
	if (intelAdapter != nullptr) intelAdapter->Release();
}
