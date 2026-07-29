# 非第三方 TODO 标记清单

> 扫描日期：2026-07-28  
> 扫描范围：仓库内项目自有文件，排除 `third_party/`。  
> 统计口径：大小写不敏感匹配独立单词 `TODO`，仅将源码中的待实现标记计入清单；文档叙述和 Doxygen 配置说明单独列出。

## 汇总

- TODO 标记位置：4 处
- 涉及源码文件：3 个
- 展开的待办事项：6 项

## 源码 TODO

### 1. 批次优化

- 位置：`src/managers/BatchManager.hpp:335`
- 所在函数：`BatchManager::optimize()`
- 当前状态：函数仅调用 `flushBatch()`，后续优化尚未实现。
- 待办事项：
  1. 按纹理排序，减少纹理切换。
  2. 合并相邻且使用相同纹理的批次。
  3. 按 Z-order 排序并正确处理透明度。

### 2. 白色纹理创建

- 位置：`src/managers/DeviceManager.hpp:197`
- 所在函数：`DeviceManager::getWhiteTexture()`
- 原标记：`TODO: Implement white texture creation`
- 当前状态：函数固定返回 `nullptr`。
- 待办事项：创建、持有并返回可供渲染使用的白色 GPU 纹理，同时纳入设备资源生命周期管理。

### 3. 纹理图集扩容时保留内容

- 位置：`src/managers/TextureAtlas.hpp:348`
- 所在函数：`TextureAtlas::expandAtlas()`
- 原标记：`TODO: 理想情况下应该复制旧纹理内容到新纹理`
- 当前状态：扩容后释放旧纹理，并清空字形映射和 shelf 布局；已有字形需要重新添加和上传。
- 待办事项：通过 SDL3 GPU 复制流程将旧图集内容迁移到新纹理，保留有效字形与布局状态。

### 4. 位图上传到纹理图集

- 位置：`src/managers/TextureAtlas.hpp:380`
- 所在函数：`TextureAtlas::uploadBitmap()`
- 原标记：`TODO: 完整实现需要 CommandBuffer + CopyPass + TransferBuffer`
- 当前状态：函数未执行 GPU 上传，只记录警告后返回 `true`，可能掩盖上传失败并导致图集内容不可用。
- 待办事项：使用 SDL3 GPU 的 `CommandBuffer`、`CopyPass` 和 `TransferBuffer` 完成灰度位图到 R8 图集纹理的上传，并返回真实执行结果。

## 未计入待办的 TODO 文本

以下匹配只是配置说明、历史描述或路径名称，不是当前源码待实现标记：

- `Doxyfile:771-772`：说明 Doxygen 的 TODO 列表配置。
- `docs/architecture/ARCHITECTURE_REVIEW_AND_ROADMAP_2026-07-11.md:277,317,497`：讨论历史 todo 文档及其状态。
- `docs/pm/run-20260717-0006-WP5-public-queued-event固定阶段.md:80`：任务边界中提到 todo 文档。
- `docs/todo/UI_FRAMEWORK_GAP_ANALYSIS_2026-05.md:32`：描述已删除的旧 TODO 存根。

## 建议处理顺序

1. `TextureAtlas::uploadBitmap()`：当前返回成功但未上传，直接影响纹理图集正确性。
2. `DeviceManager::getWhiteTexture()`：当前始终返回空指针，可能阻断无纹理图元的通用渲染路径。
3. `TextureAtlas::expandAtlas()`：补齐扩容内容迁移，避免扩容后已有字形失效。
4. `BatchManager::optimize()`：属于性能和渲染顺序优化，宜在正确性问题处理后实施。
