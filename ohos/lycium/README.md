# Lycium 适配说明

这个目录用于同步当前 `ho-thirdparty-porting` 中 `Skia` 的 `lycium` 侧改动。

注意：
- 这里不是 `Skia` 本体源码改动
- 这里记录的是配套的：
  - `recipe`
  - `SHA512SUM`
  - `lycium` 构建策略
  - 当前有效配置说明
- 这些内容应该应用到 `ho-thirdparty-porting`
- 它们不是 upstream `Skia` 仓库本体的一部分

## 对应位置

主仓中的对应路径：

- `tpc_c_cplusplus/community/skia/HPKBUILD`
- `tpc_c_cplusplus/community/skia/SHA512SUM`

当前同步副本：

- `HPKBUILD.current`
- `SHA512SUM.current`
- `skia-lycium.patch`

## 当前 patch 覆盖内容

截至 `2026-03-26`，当前 `lycium` 同步内容覆盖：

- `lycium-first` 的 `Skia` 构建路线
- 从 `libs/Skia` 同步当前 OHOS 工作树文件
- `freetype`
- `harfbuzz`
- `icu`
- `libpng`
- `zlib`
- `ohos_egl_smoke`
- `ohos_text_smoke`
- `ohos_shaper_smoke`
- 依赖缓存移出 `builddir`

## 当前有效方向

当前 OHOS 已验证通过的主配置是：

- `skia_use_ohos=true`
- `skia_use_freetype=true`
- `skia_use_harfbuzz=true`
- `skia_use_icu=true`
- `skia_use_bidi=false`
- `skia_use_egl=true`
- `skia_use_gl=true`
- `skia_use_fontconfig=false`
- `skia_use_x11=false`
- `skia_use_vulkan=false`

## Phase 4 新增点

当前除了 `Phase 3` 的 shaping 路线外，还新增了 `Phase 4` 第一项源码级平台工作：

- `SkFontMgr_ohos`
- `OHOS NativeDrawing` 官方字体接口优先

当前主线含义：

- `lycium` 侧继续负责把 `SkFontMgr_ohos` 和相关 `BUILD.gn / skia.gni / smoke tools` 同步进构建工作树
- `Skia` 本体当前已经不再把“手读系统配置文件”作为字体系统主路径
- 目前优先使用：
  - `OH_Drawing_GetSystemFontConfigInfo`
  - `OH_Drawing_CreateFontParser`
  - `OH_Drawing_FontParserGetSystemFontList`
  - `OH_Drawing_FontParserGetFontByName`

## 备注

如果后续 `ho-thirdparty-porting` 里的 `HPKBUILD`、`SHA512SUM` 或 `lycium` 策略再次更新，这里的：

- `HPKBUILD.current`
- `SHA512SUM.current`
- `skia-lycium.patch`

也应该同步更新。
## 2026-03-26 最新同步

- `SkFontMgr_ohos` 已继续推进到语言感知 fallback。
- 当前除了 `OHOS NativeDrawing` 官方字体接口优先外，还新增了：
  - `groupName + familyName` 更细匹配
  - `bcp47` 语言感知 fallback
- 当前真机 `ohos_text_smoke` 最新结果：

```text
font_families=235
alias_harmonyos_sans=1
alias_serif=1
fallback_cjk=1
fallback_arabic_lang=1
fallback_tibetan_lang=1
pixel_checksum=18319541926308614285
```

- 说明当前 `lycium` 同步内容已经不只是 Phase 3 的 shaping 路线，也包含了 Phase 4 字体管理增强。
