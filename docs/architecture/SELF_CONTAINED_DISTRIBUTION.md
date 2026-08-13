# VMP-ui 自包含发行模式

> 日期：2026-08-12
> 状态：P0 骨架已落地（静态合并 + 包裁剪 + 门禁 + Object 库拆分）；P1 为 DLL 导出宏
> 决策依据：用户确认目标为「自包含发行模式」——库以 header-only 为主，非头文件库静态链接进产物；动态库产出单 DLL，静态库产出单文件，外部不暴露任何第三方库信息。

---

## 1. 目标契约

消费者体验恒定，且**不需要知道任何第三方库的存在**：

```cmake
find_package(VMPUI CONFIG REQUIRED)
target_link_libraries(app PRIVATE VMPUI::ui)
```

```cpp
#include <ui.hpp>   // 不需要任何第三方 include path
```

### 包结构

```
静态模式（单文件）：
  include/ui/**                ← 公共头（零第三方依赖）
  lib/VMPUI.lib                ← ui + SDL3 + freetype + harfbuzz + yogacore + 资源表 合并产物
  lib/cmake/VMPUI/*.cmake      ← 无任何 find_dependency

动态模式（单 DLL，P1 完善导出宏）：
  include/ui/**                ← 公共头（需 VMP_UI_API 导出标记）
  bin/VMPUI.dll                ← 全部第三方实现编译进一个 DLL
  lib/VMPUI.lib                ← 导入库
  lib/cmake/VMPUI/*.cmake
```

## 2. 审计结论（2026-08-12）

- **公共头已零第三方**：`include/ui/**` 全部 33 个公共头的第三方 include 仅 `include/ui/WindowsMacroShield.hpp` 的 `<Windows.h>`（操作系统平台 SDK，非第三方，保留）。
- `ui::Vec2/Vec4/Rect` 已是自包含公共类型（`include/ui/MathTypes.hpp`），内部经 `EigenConversions.hpp` 与 Eigen 互转。
- 遗留过时文档已修正：`Vec2 = Eigen::Vector2f`（旧）、"基于 ASIO"（旧，实际为自研 MPSC EventLoop）、单一 `ui_tests`（旧，已拆四目标）。

## 3. P0 落地内容

### 3.1 CMake 骨架（`CMakeLists.txt` / `src/CMakeLists.txt`）

| 改动 | 说明 |
|------|------|
| `ENABLE_LTO` 默认 `OFF` | 合并库含第三方对象，开启 LTO 会要求消费者同配置；需要时 `-DENABLE_LTO=ON` |
| `add_library(ui ...)` 尊重 `BUILD_SHARED_LIBS` | 静态默认；`BUILD_SHARED_LIBS=ON` 产出 VMPUI.dll（实验性，导出宏 P1） |
| `OUTPUT_NAME VMPUI` | 发行产物统一命名 |
| `UI_RESOURCE_BACKEND` 默认 `STD_EMBED` | 资源表生成 .cpp 直接编进库，无 cmrc 依赖；CMRC 仅开发/测试 |
| 静态库 POST_BUILD 合并 | `lib.exe`/`llvm-lib` 合并 SDL3/yogacore/freetype/harfbuzz（CMRC 含 ui_fonts）为 `VMPUI.lib`；GNU 走 `ar -M` MRI 脚本 |
| 系统库 PUBLIC `$<LINK_ONLY:...>` | Windows 系统库（winmm/dxgi/d3d11/d3d12/...）随 VMPUITargets 导出，消费者零感知 |
| install 裁剪 | 只装 `ui`（合并库）+ `include/` + config；不再安装 cmrc 目标/头 |

### 3.2 Object 库拆分（编译优化，2026-08-12 新增）

`src/CMakeLists.txt` 将 ui 按模块拆为 7 个 `OBJECT` 库，再聚合为 `ui` 静态库：

| Object 库 | 内容 | PCH | JOB_POOL 变量 |
|---|---|---|---|
| `ui_core_objects` | core/*、services/TextEditingService、common/ErrorCodes | ON | `UI_POOL_CORE` |
| `ui_systems_objects` | systems/*.cpp | ON | `UI_POOL_SYSTEMS` |
| `ui_render_objects` | systems/render/*.cpp | ON | `UI_POOL_RENDER` |
| `ui_managers_objects` | managers/*.cpp | ON | `UI_POOL_MANAGERS` |
| `ui_renderers_objects` | renderers/TableRenderer.cpp | OFF | `UI_POOL_RENDERERS` |
| `ui_api_objects` | api/*.cpp（20 个薄转发） | OFF | `UI_POOL_API` |
| `ui_resources_objects` | STD_EMBED 生成的资源表 | OFF | `UI_POOL_RESOURCES` |

要点：

- **PCH 策略**：core/systems/render/managers 开启 PCH（摊薄 EnTT/Eigen/SDL/Yoga 模板解析）；api/renderers/resources 为薄转发或巨型数据，不开 PCH，避免每个 object 库重复生成 PCH 的成本。`UI_ENABLE_PCH=OFF` 可整体关闭。
- **模块级并行度**：`UI_POOL_*` 变量（CACHE，默认空 = 继承全局 `JOB_POOL_COMPILE`）可为每个模块独立设置并发数，例如重型模板模块限制 1～2、轻量 api 放开 4～6，让 Ninja 在重模块编译时用轻模块填充空闲。
- **共享编译配置**：`ui_configure_object_library()` 统一应用 include 目录、编译选项（`_UI_COMMON_COMPILE_OPTIONS`）、资源后端/平台宏、第三方 header-only include path。
- **对外不变**：聚合库 `ui`（`VMPUI::ui`）仅以 `$<TARGET_OBJECTS:...>` 拼装 + PUBLIC include，`OUTPUT_NAME VMPUI`、静态合并 POST_BUILD、系统库导出、install 全部保持不变。

### 3.3 包配置（`cmake/VMPUIConfig.cmake.in`）

- 删除全部 7 个 `find_dependency`（SDL3/yoga/Freetype/harfbuzz/EnTT/Eigen/spdlog）。
- 仅 `include(VMPUITargets.cmake)` + `check_required_components`。

### 3.3 门禁

- 新增 `tools/check_public_headers_self_contained.py`：扫描 `include/ui/**` + `include/ui.hpp`，拒绝第三方头（Eigen/entt/SDL3/spdlog/yoga/freetype/harfbuzz/cmrc/...）与 `src/` 内部路径；接入 `ui_public_headers_self_contained_check`（ALL）。
- 现有 `ui_public_header_check`（编译级）与 `tests/install_consumer/`（纯净消费者）继续作为发行验证。

## 4. 已知风险与限制

| 风险 | 处理 |
|------|------|
| 合并库含 freetype/harfbuzz 全局符号 | 消费者不得再链接其它版本的 freetype/harfbuzz（自包含的本质代价，文档说明） |
| LTO | 发布构建默认关；启用 LTO 时消费者链接需同配置 |
| D3D12 运行时 DXIL | SDL3 默认动态加载 `dxil.dll`（`SDL_DYNAMIC_DXC`）；分发时若使用 D3D12 GPU 后端需附带；Vulkan 后端不需要 |
| `Windows.h` 出现在公共头 | 系统平台 SDK，非第三方；`WindowsMacroShield` 防宏污染，保留 |
| Linux/X11 | 静态模式下 X11 系统库仍以 PRIVATE 链接，合并后消费者需系统 X11 开发库（平台级，非第三方） |
| 多配置生成器（VS） | 合并逻辑按 `$<TARGET_FILE:ui>` 输出名工作；VS 多配置需验证 Debug/Release 变体命名 |

## 5. P1 计划（未实施）

- [ ] 公共 API 全量加 `VMP_UI_API` 导出宏（复用 `src/api/export.hpp`，移至公共头或生成公共 `ui/Export.hpp`）。
- [ ] `BUILD_SHARED_LIBS=ON` 全链路验证：example / install_consumer / 单 DLL 导出符号检查。
- [ ] 多配置生成器（Visual Studio）下合并与 IMPORTED 变体验证。
- [ ] Linux 静态合并（`ar -M` MRI 路径）实测。

## 6. 验证步骤（回归手册）

```bash
# 1. 门禁
python tools/check_public_headers_self_contained.py --root .
python tools/check_architecture_boundaries.py --root .

# 2. 自包含构建（默认 STD_EMBED + LTO 关）
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
ninja -C build

# 3. 安装到临时前缀
cmake --install build --prefix <prefix>

# 4. 纯净消费者验证（无任何第三方 find_package）
cmake -G Ninja -B _consumer -S tests/install_consumer \
  -DCMAKE_PREFIX_PATH=<prefix>
ninja -C _consumer
```
