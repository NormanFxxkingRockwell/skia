/*
 * Copyright 2026
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cstdint>
#include <cstring>
#include <iostream>

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTextBlob.h"
#include "include/ports/SkFontMgr_ohos.h"
#include "modules/skshaper/include/SkShaper.h"
#include "modules/skshaper/include/SkShaper_harfbuzz.h"
#include "modules/skshaper/include/SkShaper_skunicode.h"
#include "modules/skunicode/include/SkUnicode.h"
#include "modules/skunicode/include/SkUnicode_icu.h"

namespace {

uint64_t pixel_checksum(const SkBitmap& bitmap) {
    const auto* bytes = static_cast<const uint8_t*>(bitmap.getPixels());
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

sk_sp<SkTextBlob> shape_text(SkShaper& shaper,
                             sk_sp<SkUnicode> unicode,
                             sk_sp<SkFontMgr> fontMgr,
                             const char* text,
                             size_t textBytes,
                             const SkFont& font) {
    std::unique_ptr<SkShaper::BiDiRunIterator> bidi =
            SkShapers::unicode::BidiRunIterator(unicode, text, textBytes, 0xfe);
    if (!bidi) {
        bidi = std::make_unique<SkShaper::TrivialBiDiRunIterator>(0xfe, textBytes);
    }
    if (!bidi) {
        return nullptr;
    }

    std::unique_ptr<SkShaper::LanguageRunIterator> language =
            std::make_unique<SkShaper::TrivialLanguageRunIterator>("zh-CN", textBytes);
    if (!language) {
        return nullptr;
    }

    const SkFourByteTag undeterminedScript = SkSetFourByteTag('Z', 'y', 'y', 'y');
    std::unique_ptr<SkShaper::ScriptRunIterator> script =
            SkShapers::HB::ScriptRunIterator(text, textBytes, undeterminedScript);
    if (!script) {
        script = std::make_unique<SkShaper::TrivialScriptRunIterator>(undeterminedScript,
                                                                      textBytes);
    }
    if (!script) {
        return nullptr;
    }

    std::unique_ptr<SkShaper::FontRunIterator> fontRuns =
            SkShaper::MakeFontMgrRunIterator(text, textBytes, font, fontMgr);
    if (!fontRuns) {
        return nullptr;
    }

    SkTextBlobBuilderRunHandler builder(text, {40.0f, 120.0f});
    shaper.shape(text,
                 textBytes,
                 *fontRuns,
                 *bidi,
                 *script,
                 *language,
                 nullptr,
                 0,
                 820.0f,
                 &builder);
    return builder.makeBlob();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 2) {
        return usage(argv[0]);
    }

    const char* fontDir = argc == 2 ? argv[1] : nullptr;
    const char* text = u8"\u4e2d\u6587\u5b57\u4f53\u6e32\u67d3\u9a8c\u8bc1 ABC 123";
    const size_t textBytes = strlen(text);

    sk_sp<SkFontMgr> fontMgr = SkFontMgr_New_OHOS(fontDir);
    if (!fontMgr || fontMgr->countFamilies() == 0) {
        std::cerr << "failed to load fonts from OHOS font manager";
        if (fontDir) {
            std::cerr << ": " << fontDir;
        }
        std::cerr << "\n";
        return 3;
    }

    sk_sp<SkTypeface> typeface = fontMgr->makeFromFile("/system/fonts/HarmonyOS_Sans_SC.ttf");
    if (!typeface) {
        typeface = fontMgr->legacyMakeTypeface(nullptr, SkFontStyle());
    }
    if (!typeface) {
        std::cerr << "failed to create default typeface from OHOS font manager\n";
        return 4;
    }

    sk_sp<SkUnicode> unicode;
#if defined(SK_UNICODE_ICU_IMPLEMENTATION)
    unicode = SkUnicodes::ICU::Make();
#endif
    if (!unicode) {
#if defined(SK_UNICODE_BIDI_IMPLEMENTATION)
        unicode = SkUnicodes::Bidi::Make();
#endif
    }
    if (!unicode) {
        std::cerr << "failed to create unicode helper\n";
        return 5;
    }

    auto shaperDriven = SkShapers::HB::ShaperDrivenWrapper(unicode, fontMgr);
    auto shapeThenWrap = SkShapers::HB::ShapeThenWrap(unicode, fontMgr);
    if (!shaperDriven && !shapeThenWrap) {
        std::cerr << "failed to create harfbuzz shaper\n";
        return 6;
    }

    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(900, 220));
    if (!surface) {
        std::cerr << "failed to create raster surface\n";
        return 7;
    }

    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    SkPaint panelPaint;
    panelPaint.setAntiAlias(true);
    panelPaint.setColor(SkColorSetRGB(0xE3, 0xF1, 0xFF));
    canvas->drawRect(SkRect::MakeXYWH(20, 20, 860, 70), panelPaint);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorBLACK);

    SkFont font(typeface, 38.0f);
    font.setSubpixel(true);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);

    sk_sp<SkTextBlob> blob;
    const char* shaperName = nullptr;
    if (shaperDriven) {
        blob = shape_text(*shaperDriven, unicode, fontMgr, text, textBytes, font);
        if (blob) {
            shaperName = "shaper_driven_wrapper";
        }
    }
    if (!blob && shapeThenWrap) {
        blob = shape_text(*shapeThenWrap, unicode, fontMgr, text, textBytes, font);
        if (blob) {
            shaperName = "shape_then_wrap";
        }
    }
    if (!blob) {
        std::cerr << "failed to create shaped text blob\n";
        return 12;
    }

    canvas->drawTextBlob(blob.get(), 0.0f, 0.0f, textPaint);

    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(surface->imageInfo())) {
        std::cerr << "failed to allocate bitmap for readback\n";
        return 13;
    }
    if (!surface->readPixels(bitmap.pixmap(), 0, 0)) {
        std::cerr << "failed to read back rendered pixels\n";
        return 14;
    }

    std::cout << "font_families=" << fontMgr->countFamilies() << "\n";
    std::cout << "shaper=" << shaperName << "\n";
    std::cout << "pixel_checksum=" << pixel_checksum(bitmap) << "\n";
    return 0;
}
