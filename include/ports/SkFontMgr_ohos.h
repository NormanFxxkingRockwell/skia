/*
 * Copyright 2026
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkFontMgr_ohos_DEFINED
#define SkFontMgr_ohos_DEFINED

#include "include/core/SkRefCnt.h"
#include "include/core/SkTypes.h"

class SkFontMgr;

/** Create an OHOS-oriented font manager.
 *  When dir is null or empty, the manager scans the built-in OHOS system font
 *  locations. When dir is provided, it is used as the primary font root.
 */
SK_API sk_sp<SkFontMgr> SkFontMgr_New_OHOS(const char* dir = nullptr);

#endif  // SkFontMgr_ohos_DEFINED
