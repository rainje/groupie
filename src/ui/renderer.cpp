#include "ui/renderer.h"
#include "util/log.h"

Renderer& Renderer::Instance() {
    static Renderer instance;
    return instance;
}

bool Renderer::Init() {
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory1),
        reinterpret_cast<void**>(factory_.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr)) {
        LOG_INFO(L"D2D1CreateFactory failed: 0x%08X", hr);
        return false;
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwriteFactory_.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr)) {
        LOG_INFO(L"DWriteCreateFactory failed: 0x%08X", hr);
        return false;
    }

    hr = dwriteFactory_->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_REGULAR,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f, L"en-us",
        tabFont_.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) {
        LOG_INFO(L"CreateTextFormat failed: 0x%08X", hr);
        return false;
    }

    tabFont_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    tabFont_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    LOG_INFO(L"Renderer initialized");
    return true;
}

void Renderer::Shutdown() {
    tabFont_.Release();
    dwriteFactory_.Release();
    factory_.Release();
}
