/*
 * Copyright 2026
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cstdint>
#include <iostream>

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include "include/ports/SkFontMgr_ohos.h"

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
    std::cerr << "usage: " << argv0 << " [font_dir]\n";
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 2) {
        return usage(argv[0]);
    }

    const char* fontDir = argc == 2 ? argv[1] : nullptr;

    sk_sp<SkFontMgr> fontMgr = SkFontMgr_New_OHOS(fontDir);
    if (!fontMgr || fontMgr->countFamilies() == 0) {
        std::cerr << "failed to load fonts from OHOS font manager";
        if (fontDir) {
            std::cerr << ": " << fontDir;
        }
        std::cerr << "\n";
        return 3;
    }

    sk_sp<SkTypeface> typeface = fontMgr->legacyMakeTypeface(nullptr, SkFontStyle());
    if (!typeface) {
        std::cerr << "failed to create default typeface from OHOS font manager\n";
        return 4;
    }

    sk_sp<SkTypeface> sansAlias = fontMgr->matchFamilyStyle("HarmonyOS-Sans", SkFontStyle());
    sk_sp<SkTypeface> serifAlias = fontMgr->matchFamilyStyle("serif", SkFontStyle());
    sk_sp<SkTypeface> cjkFallback =
            fontMgr->matchFamilyStyleCharacter(nullptr, SkFontStyle(), nullptr, 0, 0x4E2D);

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(640, 200));
    if (!surface) {
        std::cerr << "failed to create raster surface\n";
        return 5;
    }

    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetRGB(0x12, 0x34, 0x56));

    SkPaint accentPaint;
    accentPaint.setAntiAlias(true);
    accentPaint.setColor(SkColorSetRGB(0xD0, 0xE8, 0xFF));
    canvas->drawRect(SkRect::MakeXYWH(24, 24, 592, 72), accentPaint);

    SkFont titleFont(typeface, 42.0f);
    titleFont.setSubpixel(true);
    titleFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    canvas->drawString("HarmonyOS Skia", 36.0f, 78.0f, titleFont, textPaint);

    SkPaint bodyPaint;
    bodyPaint.setAntiAlias(true);
    bodyPaint.setColor(SK_ColorBLACK);

    SkFont bodyFont(typeface, 26.0f);
    bodyFont.setSubpixel(true);
    canvas->drawString("OHOS font directory smoke", 36.0f, 132.0f, bodyFont, bodyPaint);
    canvas->drawString(u8"\u4e2d\u6587\u5b57\u4f53\u6e32\u67d3\u9a8c\u8bc1",
                       36.0f,
                       168.0f,
                       bodyFont,
                       bodyPaint);

    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(surface->imageInfo())) {
        std::cerr << "failed to allocate bitmap for readback\n";
        return 6;
    }
    if (!surface->readPixels(bitmap.pixmap(), 0, 0)) {
        std::cerr << "failed to read back rendered pixels\n";
        return 7;
    }

    std::cout << "font_families=" << fontMgr->countFamilies() << "\n";
    std::cout << "alias_harmonyos_sans=" << (sansAlias ? 1 : 0) << "\n";
    std::cout << "alias_serif=" << (serifAlias ? 1 : 0) << "\n";
    std::cout << "fallback_cjk=" << (cjkFallback ? 1 : 0) << "\n";
    std::cout << "pixel_checksum=" << pixel_checksum(bitmap) << "\n";
    return 0;
}
