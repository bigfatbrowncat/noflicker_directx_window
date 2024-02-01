#include "DCompContext.h"

void DCompContext::unbind() {
    if (visual != nullptr) { visual->Release(); visual = nullptr; }
    if (target != nullptr) { target->Release(); target = nullptr; }
    if (dcomp != nullptr) { dcomp->Release(); dcomp = nullptr; }
}

DCompContext::DCompContext(HWND hwnd, const std::shared_ptr<D3DContext>& context): dcomp(nullptr), target(nullptr), visual(nullptr) {
// Bind our swap chain to the window.
// TODO: Determine what DCompositionCreateDevice(nullptr, ...) actually does.
// I assume it creates a minimal IDCompositionDevice for use with D3D that can't actually
// do any adapter-specific resource allocations itself, but I'm yet to verify this.
hr_check(DCompositionCreateDevice(nullptr, IID_PPV_ARGS(&dcomp)));
hr_check(dcomp->CreateTargetForHwnd(hwnd, FALSE, &target));
hr_check(dcomp->CreateVisual(&visual));
hr_check(target->SetRoot(visual));
hr_check(visual->SetContent(context->swapChain));
hr_check(dcomp->Commit());
}