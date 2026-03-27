#pragma once

#include <d2d1_1.h>
#include <dwrite.h>
#include "util/com_ptr.h"

class Renderer {
public:
    static Renderer& Instance();

    bool Init();
    void Shutdown();

    ID2D1Factory1* GetFactory() const { return factory_.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return dwriteFactory_.Get(); }
    IDWriteTextFormat* GetTabFont() const { return tabFont_.Get(); }

private:
    Renderer() = default;

    ComPtr<ID2D1Factory1> factory_;
    ComPtr<IDWriteFactory> dwriteFactory_;
    ComPtr<IDWriteTextFormat> tabFont_;
};
