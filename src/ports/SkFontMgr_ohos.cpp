/*
 * Copyright 2026
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkData.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontScanner.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkStream.h"
#include "include/ports/SkFontMgr_ohos.h"
#include "src/core/SkOSFile.h"
#include "src/ports/SkFontMgr_custom.h"
#include "src/ports/SkTypeface_FreeType.h"
#include "src/utils/SkOSPath.h"

#include <native_drawing/drawing_text_typography.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

struct OhosGenericAlias {
    SkString alias;
    SkString family;
    int weight = 400;
};

struct OhosFontConfigData {
    std::vector<SkString> fontRoots;
    std::vector<OhosGenericAlias> genericAliases;
    std::vector<SkString> fallbackFamilies;
    SkString defaultFamily;
};

bool contains_string(const std::vector<SkString>& values, const SkString& needle) {
    for (const auto& value : values) {
        if (value.equals(needle)) {
            return true;
        }
    }
    return false;
}

void push_unique(std::vector<SkString>* values, const SkString& candidate) {
    if (!candidate.isEmpty() && !contains_string(*values, candidate)) {
        values->push_back(candidate);
    }
}

SkString parent_directory(const char* path) {
    if (!path || path[0] == '\0') {
        return SkString();
    }

    const char* slash = strrchr(path, '/');
    if (!slash || slash == path) {
        return SkString();
    }
    return SkString(path, slash - path);
}

void append_native_drawing_aliases(const OH_Drawing_FontConfigInfo* info, OhosFontConfigData* config) {
    if (!info || !config) {
        return;
    }

    for (size_t i = 0; i < info->fontGenericInfoSize; ++i) {
        const auto& generic = info->fontGenericInfoSet[i];
        if (!generic.familyName || generic.familyName[0] == '\0') {
            continue;
        }

        SkString genericFamily(generic.familyName);
        if (config->defaultFamily.isEmpty()) {
            config->defaultFamily = genericFamily;
        }

        for (size_t j = 0; j < generic.aliasInfoSize; ++j) {
            const auto& aliasInfo = generic.aliasInfoSet[j];
            if (!aliasInfo.familyName || aliasInfo.familyName[0] == '\0') {
                continue;
            }

            OhosGenericAlias alias;
            alias.alias = SkString(aliasInfo.familyName);
            alias.family = genericFamily;
            alias.weight = aliasInfo.weight > 0 ? aliasInfo.weight : 400;
            config->genericAliases.push_back(std::move(alias));
        }
    }
}

void append_native_drawing_fallbacks(const OH_Drawing_FontConfigInfo* info, OhosFontConfigData* config) {
    if (!info || !config) {
        return;
    }

    for (size_t i = 0; i < info->fallbackGroupSize; ++i) {
        const auto& group = info->fallbackGroupSet[i];
        for (size_t j = 0; j < group.fallbackInfoSize; ++j) {
            const auto& fallback = group.fallbackInfoSet[j];
            if (fallback.familyName && fallback.familyName[0] != '\0') {
                push_unique(&config->fallbackFamilies, SkString(fallback.familyName));
            }
        }
    }
}

bool load_native_drawing_font_config(OhosFontConfigData* config) {
    if (!config) {
        return false;
    }

    OH_Drawing_FontConfigInfoErrorCode errorCode = SUCCESS_FONT_CONFIG_INFO;
    OH_Drawing_FontConfigInfo* info = OH_Drawing_GetSystemFontConfigInfo(&errorCode);
    if (!info || errorCode != SUCCESS_FONT_CONFIG_INFO) {
        if (info) {
            OH_Drawing_DestroySystemFontConfigInfo(info);
        }
        return false;
    }

    for (size_t i = 0; i < info->fontDirSize; ++i) {
        if (info->fontDirSet[i] && info->fontDirSet[i][0] != '\0') {
            push_unique(&config->fontRoots, SkString(info->fontDirSet[i]));
        }
    }

    append_native_drawing_aliases(info, config);
    append_native_drawing_fallbacks(info, config);
    OH_Drawing_DestroySystemFontConfigInfo(info);
    return true;
}

bool load_native_drawing_font_parser(OhosFontConfigData* config) {
    if (!config) {
        return false;
    }

    OH_Drawing_FontParser* parser = OH_Drawing_CreateFontParser();
    if (!parser) {
        return false;
    }

    size_t fontCount = 0;
    char** fontNames = OH_Drawing_FontParserGetSystemFontList(parser, &fontCount);
    if (!fontNames || fontCount == 0) {
        if (fontNames) {
            OH_Drawing_DestroySystemFontList(fontNames, fontCount);
        }
        OH_Drawing_DestroyFontParser(parser);
        return false;
    }

    bool loaded = false;
    for (size_t i = 0; i < fontCount; ++i) {
        if (!fontNames[i] || fontNames[i][0] == '\0') {
            continue;
        }

        OH_Drawing_FontDescriptor* descriptor =
                OH_Drawing_FontParserGetFontByName(parser, fontNames[i]);
        if (!descriptor) {
            continue;
        }

        if (descriptor->path && descriptor->path[0] != '\0') {
            push_unique(&config->fontRoots, parent_directory(descriptor->path));
            loaded = true;
        }
        if (config->defaultFamily.isEmpty() && descriptor->fontFamily && descriptor->fontFamily[0] != '\0') {
            config->defaultFamily = SkString(descriptor->fontFamily);
        }

        OH_Drawing_DestroyFontDescriptor(descriptor);
    }

    OH_Drawing_DestroySystemFontList(fontNames, fontCount);
    OH_Drawing_DestroyFontParser(parser);
    return loaded;
}

OhosFontConfigData load_ohos_font_config(const char* requestedDirectory) {
    OhosFontConfigData config;

    if (requestedDirectory && requestedDirectory[0] != '\0') {
        config.fontRoots.push_back(SkString(requestedDirectory));
    } else {
        const char* envDir = std::getenv("SKIA_OHOS_FONT_DIR");
        if (envDir && envDir[0] != '\0') {
            config.fontRoots.push_back(SkString(envDir));
        } else {
            load_native_drawing_font_config(&config);
            load_native_drawing_font_parser(&config);
        }
    }

    if (config.fontRoots.empty()) {
        config.fontRoots.push_back(SkString("/system/fonts"));
        config.fontRoots.push_back(SkString("/system/font"));
        config.fontRoots.push_back(SkString("/data/fonts/files"));
    }

    if (config.defaultFamily.isEmpty()) {
        config.defaultFamily = SkString("HarmonyOS Sans");
    }

    push_unique(&config.fallbackFamilies, SkString("HarmonyOS Sans SC"));
    push_unique(&config.fallbackFamilies, SkString("HarmonyOS Sans Naskh Arabic UI"));
    push_unique(&config.fallbackFamilies, SkString("Noto Serif Tibetan"));
    push_unique(&config.fallbackFamilies, SkString("Noto Sans"));
    return config;
}

class OhosSystemFontLoader : public SkFontMgr_Custom::SystemFontLoader {
public:
    explicit OhosSystemFontLoader(std::vector<SkString> roots) : fFontRoots(std::move(roots)) {}

    void loadSystemFonts(const SkFontScanner* scanner,
                         SkFontMgr_Custom::Families* families) const override {
        for (const auto& root : fFontRoots) {
            load_fonts_from_root(scanner, root, families);
        }

        if (families->empty()) {
            SkFontStyleSet_Custom* family = new SkFontStyleSet_Custom(SkString());
            families->push_back().reset(family);
            family->appendTypeface(sk_make_sp<SkTypeface_Empty>());
        }
    }

private:
    static SkFontStyleSet_Custom* find_family(SkFontMgr_Custom::Families& families,
                                              const char familyName[]) {
        for (int i = 0; i < families.size(); ++i) {
            if (families[i]->getFamilyName().equals(familyName)) {
                return families[i].get();
            }
        }
        return nullptr;
    }

    static bool directory_exists(const SkString& path) {
        SkOSFile::Iter iter(path.c_str());
        SkString ignored;
        return iter.next(&ignored, true) || iter.next(&ignored, false);
    }

    static void load_directory_fonts(const SkFontScanner* scanner,
                                     const SkString& directory,
                                     const char* suffix,
                                     SkFontMgr_Custom::Families* families) {
        SkOSFile::Iter iter(directory.c_str(), suffix);
        SkString name;

        while (iter.next(&name, false)) {
            SkString filename(SkOSPath::Join(directory.c_str(), name.c_str()));
            std::unique_ptr<SkStreamAsset> stream = SkStream::MakeFromFile(filename.c_str());
            if (!stream) {
                continue;
            }

            int numFaces;
            if (!scanner->scanFile(stream.get(), &numFaces)) {
                continue;
            }

            for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
                int numInstances;
                if (!scanner->scanFace(stream.get(), faceIndex, &numInstances)) {
                    continue;
                }

                for (int instanceIndex = 0; instanceIndex <= numInstances; ++instanceIndex) {
                    bool isFixedPitch;
                    SkString realname;
                    SkFontStyle style;
                    if (!scanner->scanInstance(stream.get(),
                                               faceIndex,
                                               instanceIndex,
                                               &realname,
                                               &style,
                                               &isFixedPitch,
                                               nullptr,
                                               nullptr)) {
                        continue;
                    }

                    SkFontStyleSet_Custom* addTo = find_family(*families, realname.c_str());
                    if (!addTo) {
                        addTo = new SkFontStyleSet_Custom(realname);
                        families->push_back().reset(addTo);
                    }
                    addTo->appendTypeface(sk_make_sp<SkTypeface_File>(
                            style,
                            isFixedPitch,
                            true,
                            realname,
                            filename.c_str(),
                            (instanceIndex << 16) + faceIndex));
                }
            }
        }

        SkOSFile::Iter dirIter(directory.c_str());
        while (dirIter.next(&name, true)) {
            if (name.startsWith(".")) {
                continue;
            }
            SkString dirname(SkOSPath::Join(directory.c_str(), name.c_str()));
            load_directory_fonts(scanner, dirname, suffix, families);
        }
    }

    static void load_fonts_from_root(const SkFontScanner* scanner,
                                     const SkString& root,
                                     SkFontMgr_Custom::Families* families) {
        if (!directory_exists(root)) {
            return;
        }

        load_directory_fonts(scanner, root, ".ttf", families);
        load_directory_fonts(scanner, root, ".ttc", families);
        load_directory_fonts(scanner, root, ".otf", families);
        load_directory_fonts(scanner, root, ".pfb", families);
    }

    std::vector<SkString> fFontRoots;
};

class SkFontMgr_OHOS final : public SkFontMgr {
public:
    explicit SkFontMgr_OHOS(const char* dir)
        : fConfig(load_ohos_font_config(dir))
        , fBaseFontMgr(sk_make_sp<SkFontMgr_Custom>(OhosSystemFontLoader(fConfig.fontRoots))) {}

protected:
    int onCountFamilies() const override {
        return fBaseFontMgr ? fBaseFontMgr->countFamilies() : 0;
    }

    void onGetFamilyName(int index, SkString* familyName) const override {
        if (fBaseFontMgr && familyName) {
            fBaseFontMgr->getFamilyName(index, familyName);
        }
    }

    sk_sp<SkFontStyleSet> onCreateStyleSet(int index) const override {
        return fBaseFontMgr ? fBaseFontMgr->createStyleSet(index) : nullptr;
    }

    sk_sp<SkFontStyleSet> onMatchFamily(const char familyName[]) const override {
        return fBaseFontMgr ? fBaseFontMgr->matchFamily(this->map_family_name(familyName)) : nullptr;
    }

    sk_sp<SkTypeface> onMatchFamilyStyle(const char familyName[],
                                         const SkFontStyle& style) const override {
        return fBaseFontMgr ? fBaseFontMgr->matchFamilyStyle(this->map_family_name(familyName), style) : nullptr;
    }

    sk_sp<SkTypeface> onMatchFamilyStyleCharacter(const char familyName[],
                                                  const SkFontStyle& style,
                                                  const char*[],
                                                  int,
                                                  SkUnichar character) const override {
        if (!fBaseFontMgr) {
            return nullptr;
        }

        if (const char* mapped = this->map_family_name(familyName)) {
            if (auto face = fBaseFontMgr->matchFamilyStyle(mapped, style); contains_glyph(face, character)) {
                return face;
            }
        }

        for (const auto& family : this->special_fallbacks(character)) {
            if (auto face = fBaseFontMgr->matchFamilyStyle(family.c_str(), style); contains_glyph(face, character)) {
                return face;
            }
        }

        for (const auto& family : fConfig.fallbackFamilies) {
            if (auto face = fBaseFontMgr->matchFamilyStyle(family.c_str(), style); contains_glyph(face, character)) {
                return face;
            }
        }

        return nullptr;
    }

    sk_sp<SkTypeface> onMakeFromData(sk_sp<SkData> data, int ttcIndex) const override {
        return fBaseFontMgr ? fBaseFontMgr->makeFromData(std::move(data), ttcIndex) : nullptr;
    }

    sk_sp<SkTypeface> onMakeFromStreamIndex(std::unique_ptr<SkStreamAsset> stream, int ttcIndex) const override {
        return fBaseFontMgr ? fBaseFontMgr->makeFromStream(std::move(stream), ttcIndex) : nullptr;
    }

    sk_sp<SkTypeface> onMakeFromStreamArgs(std::unique_ptr<SkStreamAsset> stream,
                                           const SkFontArguments& args) const override {
        return fBaseFontMgr ? fBaseFontMgr->makeFromStream(std::move(stream), args) : nullptr;
    }

    sk_sp<SkTypeface> onMakeFromFile(const char path[], int ttcIndex) const override {
        return fBaseFontMgr ? fBaseFontMgr->makeFromFile(path, ttcIndex) : nullptr;
    }

    sk_sp<SkTypeface> onLegacyMakeTypeface(const char familyName[], SkFontStyle style) const override {
        return fBaseFontMgr ? fBaseFontMgr->legacyMakeTypeface(this->map_family_name(familyName), style) : nullptr;
    }

private:
    static bool contains_glyph(const sk_sp<SkTypeface>& typeface, SkUnichar character) {
        return typeface && typeface->unicharToGlyph(character) != 0;
    }

    const char* map_family_name(const char* familyName) const {
        if (!familyName || familyName[0] == '\0') {
            return fConfig.defaultFamily.isEmpty() ? nullptr : fConfig.defaultFamily.c_str();
        }
        for (const auto& alias : fConfig.genericAliases) {
            if (alias.alias.equals(familyName)) {
                return alias.family.c_str();
            }
        }
        return familyName;
    }

    std::vector<SkString> special_fallbacks(SkUnichar character) const {
        std::vector<SkString> families;
        if (character >= 0x4E00 && character <= 0x9FFF) {
            families.push_back(SkString("HarmonyOS Sans SC"));
        }
        if (character == 0x0626) {
            families.push_back(SkString("HarmonyOS Sans Naskh Arabic UI"));
        }
        if (character == 0x0F56) {
            families.push_back(SkString("Noto Serif Tibetan"));
        }
        return families;
    }

    OhosFontConfigData fConfig;
    sk_sp<SkFontMgr> fBaseFontMgr;
};

}  // namespace

sk_sp<SkFontMgr> SkFontMgr_New_OHOS(const char* dir) {
    return sk_make_sp<SkFontMgr_OHOS>(dir);
}
