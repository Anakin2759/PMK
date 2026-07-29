# 测试构建闭环报告 - 2026-07-29 00:08

## 概览

| 阶段 | 结果 | 备注 |
|---|---|---|
| Configure | 通过 | Ninja，clang-cl，`CMAKE_BUILD_TYPE=Debug`，`ENABLE_BUILD_TESTS=ON`；SDL GPU drivers 配置为 D3D12、Vulkan |
| Build | 部分失败 | `ui` 通过；`example_ui_demo` 通过；`ui_tests` 的各测试程序均完成编译链接，但构建期 `gtest_discover_tests` 固定 5 秒超时导致聚合 target 非零退出 |
| Test | 通过（含跳过） | 直接执行全部 4 个测试程序：180 个测试，176 通过、4 跳过、0 失败；CTest 因构建期发现文件未生成而不能完成标准路径 |
| Coverage | 跳过 | 用户未要求，且本轮未启用既有覆盖率配置 |
| Package | 跳过 | 未要求打包 |

## 目标构建结果

| Target | 结果 | 说明 |
|---|---|---|
| `ui` | 通过 | Debug 编译完成 |
| `example_ui_demo` | 通过 | Debug 链接生成 `build/example/ui_demo/example_ui_demo.exe` |
| `ui_tests` | 失败 | 测试程序可成功链接；后置 GoogleTest discovery 超过 `TEST_DISCOVERY_TIMEOUT=5`，Ninja 以失败结束 |
| `ui_unit_tests` | 程序已生成 | 直接执行：61 项，57 通过、4 跳过、0 失败 |
| `ui_ecs_tests` | 程序已生成 | 直接执行：103 项，103 通过 |
| `ui_api_tests` | 程序已生成 | 直接执行：15 项，15 通过 |
| `ui_fallback_lifecycle_tests` | 程序已生成 | `SDL_VIDEODRIVER=offscreen` 直接执行：1 项，1 通过 |

## GPU readback 验证

| 项目 | 后端/结果 | 通过 | 跳过 | SDL 错误 |
|---|---|---:|---:|---|
| C1 TextureAtlas R8 上传 readback | D3D12、Vulkan 均未建立设备 | 0 | 1 | `Video subsystem not initialized` |
| C3 TextureAtlas 扩容迁移 readback | D3D12、Vulkan 均未建立设备 | 0 | 1 | `Video subsystem not initialized` |
| TextureAtlas 4096 上限保持 | fixture 未建立 GPU 设备 | 0 | 1 | `Video subsystem not initialized` |
| C2 DeviceManager 白纹理生命周期/readback | D3D12、Vulkan 初始化均失败 | 0 | 1 | `Video subsystem not initialized` |
| **合计** | **真实 GPU readback 未执行** | **0** | **4** | **最终 SDL 错误同上** |

当前环境的 SDL 构建包含 D3D12、Vulkan GPU driver，但测试进程未初始化 Video 子系统，未获得真实 GPU 后端。因此不能宣称 C1～C3 GPU 正确性完全签收；需在能成功建立 D3D12 或 Vulkan 设备的环境复跑 4 个 GPU 用例。

## 全部测试统计

| 测试集 | 通过 | 跳过 | 失败 |
|---|---:|---:|---:|
| Unit | 57 | 4 | 0 |
| ECS | 103 | 0 | 0 |
| API | 15 | 0 | 0 |
| Integration / SDL lifecycle | 1 | 0 | 0 |
| **合计** | **176** | **4** | **0** |

## 失败与处理

| 阶段 | 名称 | 摘要 | 问题日志 |
|---|---|---|---|
| Build | `example_ui_demo` 首次调用 | PowerShell 短暂无法解析 `cmake`；改用绝对路径后通过 | 条目 1 |
| Test | CTest 组合诊断命令 | 终端输入污染导致 PowerShell 解析失败；拆分单命令继续 | 条目 2 |
| Build | `ui_tests` | 所有测试程序链接后的 GoogleTest discovery 固定 5 秒超时；两次复跑仍失败 | 条目 3 |

## 自动修复

- 无。未修改 CMake、业务源码、公共接口或测试源码。

## 待人工处理

- [ ] 在允许修改 CMake 的后续工作中评估提高 `gtest_discover_tests` 的 discovery timeout，或定位本机进程冷启动为何稳定超过 5 秒；本轮禁止修改 CMake。
- [ ] 在已初始化 SDL Video 且可建立 D3D12/Vulkan GPU device 的验收环境复跑 4 个 GPU 用例，要求不得全跳过。
- [ ] `example_ui_demo` 仅完成自动构建，未人工运行。其需要交互式窗口及视觉确认，不适合当前自动化闭环；白纹理渲染、文本和大量字形显示未做人工签收。

---

# 唯一失败回路复验 - 2026-07-29 00:15

## 概览

| 阶段 | 结果 | 备注 |
|---|---|---|
| Configure | 通过 | Ninja，clang-cl，Debug，`ENABLE_BUILD_TESTS=ON`，`BUILD_EXAMPLES=ON` |
| Build | 部分失败 | `ui`、`example_ui_demo`、修改后的 `ui_unit_tests` 通过；`ui_tests` 聚合 target 仍被 discovery 5 秒超时阻塞 |
| Test | 直接执行全通过 / CTest 阻塞 | 4 个程序共 180 项：180 通过、0 跳过、0 失败；标准 CTest 因注册文件未生成而失败 |
| Coverage | 跳过 | 未要求且未启用覆盖率配置 |
| Package | 跳过 | 未要求 |

## A8/A9 修复

- `tests/unittest/test_GPUTextureOwner.cpp`：`TextureAtlasGpuTest` 在 Video 尚未初始化时调用 `SDL_InitSubSystem(SDL_INIT_VIDEO)`；GPU device 先释放，且仅由 fixture 初始化时才对称调用 `SDL_QuitSubSystem`。
- `tests/unittest/test_DeviceClaimState.cpp`：新增 `DeviceManagerGpuTest` fixture，采用相同的按需初始化/所有权清理规则；不接管或关闭外部既有 SDL Video 生命周期。
- 定性：上一轮 `Video subsystem not initialized` 属于明确测试 fixture 初始化遗漏，不是真实无 GPU 后端；未修改生产源码。

## GPU readback/lifecycle 结果

| 用例 | 结果 | 后端/证据 |
|---|---|---|
| `UploadsR8BitmapAndCachesOnlySuccessfulGlyphs` | 通过 | SDL GPU device 成功建立并完成 R8 readback |
| `ExpansionMigratesPixelsAndPreservesGlyphMetadata` | 通过 | SDL GPU device 成功建立并完成迁移 readback |
| `MaximumSizeRejectionPreservesExistingGlyph` | 通过 | SDL GPU device 成功建立并保持既有纹理/字形状态 |
| `WhiteTextureBelongsToDeviceGenerationAndContainsWhitePixel` | 通过 | `DeviceManager` 两次明确锁定真实 `direct3d12` 后端，完成白像素 readback 与生命周期复建 |
| **合计** | **4 通过 / 0 跳过 / 0 失败** | **真实 GPU 后端：Direct3D 12（`direct3d12`）** |

SDL Video 初始化成功，未再出现 SDL 初始化错误；本机真实 D3D12 GPU 环境可用，不需要用户另行提供 GPU 环境。运行期间 SDL 另行输出 `Installed Vulkan doesn't implement the VK_EXT_headless_surface extension`，但未影响 D3D12 设备建立或 4 项 GPU 验证。

## 全部测试统计

| 测试集 | 通过 | 跳过 | 失败 |
|---|---:|---:|---:|
| Unit | 61 | 0 | 0 |
| ECS | 103 | 0 | 0 |
| API | 15 | 0 | 0 |
| Integration / SDL lifecycle | 1 | 0 | 0 |
| **合计** | **180** | **0** | **0** |

## 越界阻塞

- GoogleTest discovery 固定 5 秒超时仍存在；本轮 `ui_tests` 聚合构建首先在 `ui_ecs_tests --gtest_list_tests` 超时。
- 因 discovery 文件未生成，全量 CTest 显示 4 个 `*_NOT_BUILT` 占位项并失败；四个测试程序直接执行则全部通过。
- 本工作包禁止修改 CMake，且唯一失败回路已结束。若要求 `ui_tests` 聚合 target 与标准 CTest 路径全绿，需要用户明确授权调整 `tests/unittest/CMakeLists.txt` 的 discovery timeout。

## 本轮自动修复

- `tests/unittest/test_GPUTextureOwner.cpp`
- `tests/unittest/test_DeviceClaimState.cpp`

## 待人工处理

- [ ] 用户授权 CMake 调整后，提高或重新设计 GoogleTest discovery timeout，再复验 `ui_tests` 与全量 CTest。

---

# GoogleTest discovery timeout 修复闭环 - 2026-07-29

## 概览

| 阶段 | 结果 | 备注 |
|---|---|---|
| Configure | 通过 | 无 preset，build-dir=`build`，Debug，`ENABLE_BUILD_TESTS=ON` |
| Build | 通过 | Debug `all` 与 `ui_tests` 均成功；构建期 discovery 不再超时 |
| Test | 通过 | 标准 `ctest --output-on-failure`：180 通过、0 跳过、0 失败 |
| Coverage | 跳过 | 未要求，且未重配覆盖率 |
| Package | 跳过 | 未要求 |

## 修复

- 文件：`tests/unittest/CMakeLists.txt`
- `ui_unit_tests`、`ui_ecs_tests`、`ui_api_tests`、`ui_fallback_lifecycle_tests` 的 GoogleTest 构建期 discovery timeout：默认 **5 秒** → 显式 **60 秒**。
- 保持既有 `POST_BUILD` discovery 行为；未改为 `PRE_TEST`，未改变测试属性、业务逻辑、公共接口或构建拓扑。

## 构建验证

| Target | 结果 | 说明 |
|---|---|---|
| `all` | 通过 | Debug 全量构建完成；`ui_ecs_tests` 重新链接后的 discovery 成功 |
| `ui_tests` | 通过 | 聚合 target 成功，四个测试程序的 discovery 注册文件可用 |

## 标准 CTest

| 测试集 | 通过 | 跳过 | 失败 |
|---|---:|---:|---:|
| Unit | 61 | 0 | 0 |
| ECS | 103 | 0 | 0 |
| API | 15 | 0 | 0 |
| Integration / SDL lifecycle | 1 | 0 | 0 |
| **合计** | **180** | **0** | **0** |

- CTest 汇总：`100% tests passed, 0 tests failed out of 180`。
- 总执行时间：5.24 秒。

## 4 个真实 GPU 用例

| 用例 | CTest 结果 |
|---|---|
| `DeviceManagerGpuTest.WhiteTextureBelongsToDeviceGenerationAndContainsWhitePixel` | 通过 |
| `TextureAtlasGpuTest.UploadsR8BitmapAndCachesOnlySuccessfulGlyphs` | 通过 |
| `TextureAtlasGpuTest.ExpansionMigratesPixelsAndPreservesGlyphMetadata` | 通过 |
| `TextureAtlasGpuTest.MaximumSizeRejectionPreservesExistingGlyph` | 通过 |
| **合计** | **4 通过、0 跳过、0 失败** |

沿用上一轮已确认的真实 Direct3D 12（`direct3d12`）测试 fixture；本轮四项均通过标准 CTest 路径执行，不是直接运行替代验证。

## 失败与处理

| 阶段 | 名称 | 摘要 | 问题日志 |
|---|---|---|---|
| Test | 首次标准 CTest 调用 | 终端混入先前命令残留字符，PowerShell 解析失败；后续独立启动复跑通过 | 本轮新增条目 1 |
| Test | 汇总复验调用 | 终端会话短暂未解析 CTest 绝对路径；改由独立 `cmd.exe` 启动并保存完整日志后通过 | 本轮新增条目 2 |

## 自动修复

- `tests/unittest/CMakeLists.txt`：为四处 `gtest_discover_tests` 显式增加 `DISCOVERY_TIMEOUT 60`。

## 待人工处理

- [ ] 无。
