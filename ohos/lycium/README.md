# Lycium 适配说明

这份目录用于同步当前 `Skia` 在 `ho-thirdparty-porting` 中使用的 `lycium` 侧改动。

注意：

- 这里不是 `Skia` 本体源码改动
- 这里记录的是配套的 `recipe / SHA512SUM / 构建策略`
- 这些内容应应用到 `ho-thirdparty-porting`，不是直接应用到 upstream `Skia` 仓库树

## 对应位置

目标仓库中的对应路径是：

- `tpc_c_cplusplus/community/skia/HPKBUILD`
- `tpc_c_cplusplus/community/skia/SHA512SUM`

## 当前 patch 说明

当前 patch 反映的是截至 `2026-03-25` 的有效状态，包含：

- `lycium-first` 的 Skia 构建路线
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

当前 OHOS 已验证通过的配置方向是：

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

## 备注

如果后续 `ho-thirdparty-porting` 里的 `HPKBUILD` 再更新，这里的 patch 也应该同步更新。
