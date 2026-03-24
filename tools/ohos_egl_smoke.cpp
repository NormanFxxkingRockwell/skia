/*
 * Copyright 2026
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cstdint>
#include <iostream>
#include <memory>

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "src/gpu/ganesh/gl/GrGLUtil.h"
#include "tools/ganesh/gl/GLTestContext.h"

bool gCreateProtectedContext = false;

namespace {

uint64_t pixel_checksum(const SkBitmap& bitmap) {
    const uint8_t* bytes = static_cast<const uint8_t*>(bitmap.getPixels());
    if (!bytes) {
        return 0;
    }

    uint64_t checksum = 1469598103934665603ull;
    const size_t size = bitmap.computeByteSize();
    for (size_t i = 0; i < size; ++i) {
        checksum ^= bytes[i];
        checksum *= 1099511628211ull;
    }
    return checksum;
}

int usage(const char* argv0) {
    std::cerr << "usage: " << argv0 << "\n";
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 1) {
        return usage(argv[0]);
    }

    std::unique_ptr<sk_gpu_test::GLTestContext> glContext(
            sk_gpu_test::CreatePlatformGLTestContext(kGLES_GrGLStandard));
    if (!glContext || !glContext->isValid()) {
        std::cerr << "failed to create EGL/GLES test context\n";
        return 3;
    }

    GrContextOptions options;
    sk_sp<GrDirectContext> directContext = glContext->makeContext(options);
    if (!directContext) {
        std::cerr << "failed to create Ganesh direct context\n";
        return 4;
    }

    const SkImageInfo info = SkImageInfo::MakeN32Premul(256, 256);
    sk_sp<SkSurface> surface = SkSurfaces::RenderTarget(
            directContext.get(),
            skgpu::Budgeted::kNo,
            info,
            0,
            kBottomLeft_GrSurfaceOrigin,
            nullptr);
    if (!surface) {
        std::cerr << "failed to create GPU render target surface\n";
        return 5;
    }

    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SkColorSetRGB(0x0B, 0x1D, 0x2A));

    SkPaint paint;
    paint.setAntiAlias(true);

    paint.setColor(SkColorSetRGB(0x33, 0xC3, 0x99));
    canvas->drawRect(SkRect::MakeXYWH(24, 24, 96, 176), paint);

    paint.setColor(SkColorSetRGB(0xFF, 0xB7, 0x03));
    canvas->drawCircle(176.0f, 112.0f, 52.0f, paint);

    paint.setColor(SkColorSetRGB(0xFB, 0x56, 0x7A));
    paint.setStroke(true);
    paint.setStrokeWidth(10.0f);
    canvas->drawLine(28.0f, 220.0f, 228.0f, 32.0f, paint);

    directContext->flushAndSubmit(surface.get(), GrSyncCpu::kYes);

    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(info)) {
        std::cerr << "failed to allocate readback bitmap\n";
        return 6;
    }
    if (!surface->readPixels(bitmap.pixmap(), 0, 0)) {
        std::cerr << "failed to read back GPU pixels\n";
        return 7;
    }

    std::cout << "gpu_backend=egl_gles\n";
    std::cout << "pixel_checksum=" << pixel_checksum(bitmap) << "\n";
    return 0;
}
