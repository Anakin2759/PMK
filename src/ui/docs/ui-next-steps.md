# UI 模块下一步更新计划

## 1. Icon 系统全面落地 (Priority: High)

目前 Icon 系统已有初步架构，但尚未完全跑通。

- [X] **IconManager 补完**: 实现从 TTF 码点生成位图并缓存到 GPU 纹理的逻辑。
- [X] **IconRenderer 集成**: 恢复 `RenderSystem` 中被注释掉的 `IconRenderer`，实现基于矩形和白色纹理（或 Icon 图集）的图标渲染。
- [X] **资源整合**: 扫描 `ui/assets/icons` 下的图标库并自动加载。

## 2. 交互体验增强 (Priority: Medium)

- [ ] **滚动条完善**: 补全 `ScrollBarRenderer` 与 `StateSystem` 中的水平滚动条逻辑。
- [ ] **状态反馈**: 为滚动条滑块 (Thumb) 添加 Hover 和 Active 视觉状态反馈。
- [ ] **Tooltips 基础**: 基于 `ZOrder` 实现浮动提示层（Tooltip）的逻辑框架。

## 3. 渲染效率优化 (Priority: Medium)

- [ ] **Batching 深度优化**: 在 `BatchManager::optimize()` 中实现按纹理和裁剪区域 (Scissor) 进行的批次排序与合并，显著降低 Draw Call。
- [ ] **缓冲区池化**: 在 `CommandBuffer` 中引入缓冲区池，避免每一帧重复创建和销毁 GPU 资源。

## 4. 动画系统雏形 (Priority: Low)

- [ ] **补间组件**: 引入 `Tween` 组件，支持对 `Alpha`、`Position` 等组件进行时间轴插值。
- [ ] **交互动效**: 实现按钮点击时的缩放或颜色渐变效果。

## 5. 主题系统 (Priority: Low)

- [ ] **样式抽离**: 将常用的颜色、圆角、间距等硬编码参数整合为 `Theme` 资源。
- [ ] **动态切换**: 支持在运行时一键切换全局主题方案（如 Dark/Light Mode）。

## 6.CPU渲染支持 (Priority: Low)

- [ ] **引入SwiftShader**: cpu模拟vulkan。
- [ ] 提供一个全局状态组件，管理渲染后端策略
- [ ] 默认策略：按照责任链遍历，谁先能够渲染就选谁
- [ ] VULKAN策略 使用VULKAN
- [ ] D3D12策略 使用D3D12
- [ ] 后备策略，使用cpu模拟vulkan的dll(打包在资源文件中)走vulkan的路径，但是仍然算单独策略

## 7.终极目标：实现DSL (Priority: Low)

- [ ] **引入一种简单的json-like DSL用来描述界面**。
- [ ] 写一个DSL解析工具 动态调用工厂函数/工具函数/回调函数
