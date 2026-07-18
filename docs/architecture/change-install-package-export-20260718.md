# VMP-ui 安装、导出与独立 Consumer 修改规划

- **日期**：2026-07-18
- **输入来源**：用户要求；当前根 `CMakeLists.txt`、`src/CMakeLists.txt`、各 vendored 依赖 CMake 安装/导出规则、CMRC `ui_fonts` 生成规则、公共 `include/` 树
- **作用范围**：安装目标、公共头安装、CMake package/export、vendored 静态依赖闭包、安装树独立 consumer；不改变 C++ 公共 API，不修改运行时实现
- **改动类型**：构建/发布接口扩展；若遗漏静态链接闭包则属于安装包破坏性缺陷

## 1. 当前状态与关键结论

1. `ui` 是 `STATIC`，当前没有自身的 `install(TARGETS)`、公共头安装、`VMPUIConfig.cmake` 或 `VMPUITargets.cmake`。
2. `ui` 的公共头已经做到自包含边界：`include/ui/**` 未直接包含 SDL、Yoga、FreeType、HarfBuzz、EnTT、Eigen、spdlog、CMRC 或 stb 头。因此第三方依赖不应升级为 `PUBLIC` 编译依赖。
3. `ui` 当前 `PRIVATE` 链接：
   - 归档目标：`SDL3::SDL3`（当前静态构建最终为 SDL 静态目标）、`yogacore`、`freetype`、`harfbuzz`、默认 CMRC 后端下的 `ui_fonts`；
   - header-only/interface 目标：`EnTT::EnTT`、`Eigen3::Eigen`、`spdlog::spdlog_header_only`；
   - Windows 系统库：`dwmapi`、`comctl32`，MinGW 另有 `uxtheme`；Linux 条件目标可能有 `X11::X11`。
4. 默认 CMRC 路径下，`ui_fonts` 是独立静态库，不会被物理合并进 `ui`。只安装 `ui.lib`/`libui.a` 必然可能在最终 consumer 链接时缺少 `ui_fonts` 符号。
5. CMRC 创建的 `ui_fonts` 又以 `PUBLIC` 依赖 `cmrc-base`。若导出 `ui_fonts`，`cmrc-base` 也必须存在于某个 export set；否则 CMake 安装导出检查会报告依赖 target 未导出。
6. 多个第三方项目已有原生安装包规则，但作为子项目时部分规则默认关闭：SDL 的 `SDL_INSTALL`、EnTT 的 `ENTT_INSTALL`、spdlog 的 `SPDLOG_INSTALL` 需要由根工程明确开启。FreeType、HarfBuzz、Eigen、Yoga 当前已有安装规则。
7. Yoga 当前安装接口值得在实施时先修正：`yogacore` 的 `INSTALL_INTERFACE` 使用了 `${CMAKE_INSTALL_PREFIX}/include/yoga`，不利于可重定位，且与构建接口根目录语义不一致；应使用相对 `${CMAKE_INSTALL_INCLUDEDIR}`，由 package prefix 解析。

## 2. 静态库 `PRIVATE` 依赖的 `LINK_ONLY` 行为与风险

对静态库而言，`PRIVATE` 只表示依赖的**编译用法要求**不公开，不表示最终链接不需要依赖。CMake 会把静态库的私有链接项写入导出目标的 `INTERFACE_LINK_LIBRARIES`，通常表现为：

```cmake
$<LINK_ONLY:dependency-target>
```

其含义是：consumer 编译 `ui` 公共头时不继承该依赖的 include/definitions，但链接最终可执行文件或共享库时仍把依赖加入链接闭包。这正符合当前公共头零第三方 include 债的边界。

主要风险：

- `VMPUITargets.cmake` 中出现 `$<LINK_ONLY:SDL3::SDL3-static>`、`$<LINK_ONLY:yoga::yogacore>`、`$<LINK_ONLY:freetype>`、`$<LINK_ONLY:harfbuzz::harfbuzz>`、`$<LINK_ONLY:VMPUI::ui_fonts>` 等名字，而加载 Targets 前这些 target 不存在，consumer 会在生成阶段失败。
- 仅把依赖改成 `PRIVATE`、手工删除导出接口或使用裸 `.lib/.a` 路径，会掩盖问题但丢失平台系统库、配置映射和可重定位性。
- 将实现依赖改成 `PUBLIC` 不会解决“target 不存在”，反而会把第三方 include/宏污染公共编译契约。
- 静态链接是按需抽取对象；仅包含 `<ui.hpp>` 的编译测试不足以证明链接闭包完整，consumer 必须调用一个落在 `ui` 归档中的非 inline API。

## 3. 推荐方案：同前缀 vendored 安装 + 原生依赖包 + `find_dependency`

### 3.1 包名与目标命名

- CMake package：`VMPUI`
- 安装目录：`${CMAKE_INSTALL_LIBDIR}/cmake/VMPUI`
- export set：`VMPUITargets`
- namespace：`VMPUI::`
- 主消费目标：`VMPUI::ui`
- 构建树同时提供 `add_library(VMPUI::ui ALIAS ui)`，保证 build-tree/install-tree 用法一致。
- 根 `project()` 增加明确 `VERSION`，首个可安装版本建议 `0.1.0`；若仓库已有发布版本规范，以该规范替换，不从某个头文件注释推断版本。

### 3.2 安装哪些目标

VMPUI 自身 export set 安装：

| 条件 | 目标 | 类型 | 原因 |
|---|---|---:|---|
| 始终 | `ui` | STATIC | 主库 |
| `UI_RESOURCE_BACKEND=CMRC` | `ui_fonts` | STATIC | 资源符号位于独立归档，最终 consumer 必须链接 |
| `UI_RESOURCE_BACKEND=CMRC` | `cmrc-base` | INTERFACE | `ui_fonts` 的导出接口引用它；避免“target 不在任何 export set” |
| `UI_RESOURCE_BACKEND=STD_EMBED` | 无额外资源目标 | — | 资源表已编译进 `ui` 源集 |

随同一安装前缀安装、但保留各自原生 package/export 的 vendored 依赖：

- SDL3 静态库、SDL3 headers 与 `SDL3Config.cmake`；关闭 SDL test library、tests、examples、CPack/docs。
- Yoga `yogacore`、头文件与 `yogaConfig.cmake`。
- FreeType `freetype`、其 interface compatibility target、头文件与 config。
- HarfBuzz 仅 `harfbuzz` 主库、头文件与 config；建议 `HB_BUILD_SUBSET=OFF`、utils/gobject/icu/cairo 等保持 OFF，避免安装无关库。
- EnTT headers/interface target 与 `EnTTConfig.cmake`。
- Eigen headers/interface target 与 `Eigen3Config.cmake`。
- spdlog headers/header-only target 与 `spdlogConfig.cmake`；无需安装/要求 consumer 链接 compiled spdlog 库，但上游规则可能同时安装其静态库，可作为后续瘦身项，不阻塞首版。

不安装：stb（仅 `ui` 私有源码编译 include，未进入链接接口）、测试库、示例程序、`ui_shaders` 自定义目标、原始字体/着色器文件（它们已嵌入资源归档或 `ui`）。

### 3.3 vendored 与 `find_dependency` 的关系

推荐不是让 consumer 自行寻找任意系统版本，也不是把所有第三方 target 强塞进一个 `VMPUITargets`：

1. 构建 VMP-ui 时仍使用仓库锁定的 vendored 源码。
2. 安装时调用这些依赖各自已有的原生 install/export 规则，将**同一次构建产生的归档、头和 config** 安装到 VMPUI 相同 prefix。
3. `VMPUIConfig.cmake` 在 include `VMPUITargets.cmake` 之前调用 `find_dependency(... CONFIG)`，从该 prefix 加载 target。

建议顺序：

1. `find_dependency(SDL3 CONFIG)`
2. `find_dependency(yoga CONFIG)`
3. `find_dependency(freetype CONFIG)`
4. `find_dependency(harfbuzz CONFIG)`（其导出接口当前引用 raw `freetype` target，故须后加载）
5. `find_dependency(EnTT CONFIG)`
6. `find_dependency(Eigen3 CONFIG)`
7. `find_dependency(spdlog CONFIG)`
8. 条件 Linux/X11 构建若导出接口实际含 `X11::X11`，再 `find_dependency(X11)`；是否需要以生成的 `VMPUITargets.cmake` 为准
9. 最后 `include("${CMAKE_CURRENT_LIST_DIR}/VMPUITargets.cmake")`

这样既保持安装包自带的精确依赖产物，又让 CMake 在读取 `$<LINK_ONLY:...>` 前建立所需 imported targets。系统库如 Windows SDK 库由名字直接解析；SDL 原生静态 package 负责 SDL 自身平台依赖。

### 3.4 如何避免“导出不存在 target”

实施时必须同时满足：

- `ui`、`ui_fonts`、`cmrc-base` 放进 `VMPUITargets`；不能只导出 `ui`。
- 对每个 `ui` 私有 target 依赖，要么它在 `VMPUITargets`，要么它在另一个已安装 export set，并由 `VMPUIConfig.cmake` 的 `find_dependency` 先加载。
- 根工程在 `add_subdirectory` 前以 cache 变量明确开启 SDL/EnTT/spdlog 安装，避免依赖 target 存在于构建树、却没有安装 export。
- 不在 `VMPUIConfig.cmake` 创建同名伪 imported target；这会绕过真实归档和 usage requirements。
- 安装后检查 `VMPUITargets.cmake` 不得包含 `D:/test/VMP-ui`、`third_party/`、构建目录绝对路径或未定义 target 名。
- 修复 Yoga 的相对安装 include 接口，所有 `INSTALL_INTERFACE` 保持 prefix-relative。
- 若 CMake 因某依赖 target 同时被多个上游 export 规则引用而在 configure/install-export 阶段报错，优先让该依赖只留在其**原生 export set**，不要复制进 `VMPUITargets`；`ui_fonts`/`cmrc-base` 是 VMPUI 自有生成目标，例外地随主 export。

## 4. 公共头与安装布局

安装整个稳定公共树，而不是维护易漏项的 `UI_HEADERS` 手工安装列表：

- `include/ui.hpp` → `${CMAKE_INSTALL_INCLUDEDIR}/ui.hpp`
- `include/ui/*.hpp` → `${CMAKE_INSTALL_INCLUDEDIR}/ui/*.hpp`
- `include/ui/api/*.hpp` → `${CMAKE_INSTALL_INCLUDEDIR}/ui/api/*.hpp`

推荐使用 `install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})` 作为首版最小方案。当前 `UI_HEADERS` 清单已遗漏 `MathTypes.hpp`、多个 API 头，不能作为安装真源。后续如需 CMake file sets，可单独迁移，不应阻塞 WP3 收口。

建议安装布局：

```text
<prefix>/
  include/ui.hpp
  include/ui/**/*.hpp
  lib/ui[...].lib|libui.a
  lib/ui_fonts[...].lib|libui_fonts.a        # CMRC 后端
  lib/cmake/VMPUI/VMPUIConfig.cmake
  lib/cmake/VMPUI/VMPUIConfigVersion.cmake
  lib/cmake/VMPUI/VMPUITargets*.cmake
  lib/cmake|share/.../<dependency configs>   # 各上游既有位置
  lib/<vendored dependency archives>
```

## 5. 独立 consumer 最小工程

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.31)
project(vmpui_install_consumer LANGUAGES CXX)
find_package(VMPUI CONFIG REQUIRED)
add_executable(vmpui_install_consumer main.cpp)
target_link_libraries(vmpui_install_consumer PRIVATE VMPUI::ui)
```

### `main.cpp`

```cpp
#include <cstddef>
#include <span>
#include <ui.hpp>

int main(int argc, char** argv)
{
    auto app = ui::factory::CreateApplication(
        std::span<char*>{argv, static_cast<std::size_t>(argc)});
    return app ? 0 : 1;
}
```

consumer 的验收重点是**成功编译并链接**，不要求运行；创建应用可能访问 SDL/平台环境，不适合作为无显示 CI 的运行冒烟。该调用比空 `main` 或仅调用 inline API 更能迫使链接器抽取 `ui` 对象并验证实现依赖闭包。

## 6. 精确文件清单与修改规划

| 优先级 | 文件 | 操作 | 内容 |
|---:|---|---|---|
| P0 | `CMakeLists.txt` | 修改 | 引入 `GNUInstallDirs`/`CMakePackageConfigHelpers`；确定项目版本；在添加依赖前固定最小安装选项；生成并安装 `VMPUIConfig.cmake`、version、`VMPUITargets`；保持 tests/examples 不进入发布闭包 |
| P0 | `src/CMakeLists.txt` | 修改 | 增加 `VMPUI::ui` build alias；安装 `ui`；CMRC 条件下安装 `ui_fonts`、`cmrc-base` 到同一 export set；安装整个 `include/` 公共树；设置 archive/runtime/library 目的地 |
| P0 | `cmake/VMPUIConfig.cmake.in` | 新增 | `@PACKAGE_INIT@`、按正确顺序 `find_dependency`、最后 include `VMPUITargets.cmake`、`check_required_components(VMPUI)` |
| P0 | `third_party/yoga/yoga/CMakeLists.txt` | 修改 | 将 `INSTALL_INTERFACE` 改为相对、可重定位且与已安装头布局一致的 include 根目录 |
| P0 | `tests/install_consumer/CMakeLists.txt` | 新增 | 只使用 `find_package(VMPUI CONFIG REQUIRED)` 和 `VMPUI::ui` |
| P0 | `tests/install_consumer/main.cpp` | 新增 | 调用 `CreateApplication`，触发真实静态链接闭包 |
| P1 | `cmake/RunInstallConsumerTest.cmake` | 新增（推荐） | 用全新 build/prefix 安装，再配置/构建独立 consumer；不引用源码 targets |
| P1 | `CMakeLists.txt` | 修改 | 在 `ENABLE_BUILD_TESTS` 下注册安装树测试，或增加显式 `check_install_consumer` target |

不需要修改任何 `src/*.cpp`、`include/*.hpp` 或公共 API。

## 7. 实施顺序

1. **依赖安装开关与瘦身**：开启 SDL/EnTT/spdlog install，关闭 SDL tests/test library/examples/docs/CPack，关闭 HarfBuzz subset/utils；固定 FreeType 可选依赖策略，避免偶然拾取系统库。
2. **修正 Yoga 可重定位接口**。
3. **主目标与资源目标安装**：先做到 `cmake --install` 成功，解决所有“不在 export set”诊断。
4. **生成 VMPUI package config**：先 `find_dependency`，后 include targets。
5. **独立 consumer**：单独 source/build 目录，仅传安装 prefix。
6. **跨配置/平台补齐**：至少当前 Windows clang-cl Debug；发布前补 Release。Linux 重点观察 X11 target 是否进入导出接口。

## 8. 验证步骤

以下是实施后的命令级验收顺序（目录名可调整）：

1. 清洁配置发布构建，禁止示例和单测干扰：
   - `cmake -S . -B build-install -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_EXAMPLES=OFF -DENABLE_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=<abs-prefix>`
2. 构建：`cmake --build build-install`
3. 安装：`cmake --install build-install --config Debug`
4. 静态扫描安装树：
   - 确认公共头、`ui`、CMRC 下 `ui_fonts`、各依赖归档和 package config 存在；
   - 搜索 `<abs-prefix>` 外的源码/构建绝对路径，尤其 `D:/test/VMP-ui`、`third_party`；结果必须为零；
   - 检查 `VMPUITargets.cmake` 的全部 `$<LINK_ONLY:...>` target 均由 VMPUI 自身 targets 或前置 `find_dependency` 定义。
5. 在仓库构建树之外配置 consumer：
   - `cmake -S tests/install_consumer -B <external-consumer-build> -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=<abs-prefix>`
6. 构建 consumer：`cmake --build <external-consumer-build>`；必须成功链接，且命令行不得出现仓库源码/原构建目录 include/lib 路径。
7. 将仓库原 `build-install` 临时改名或移走后，删除 consumer build 并重复第 5–6 步，证明只依赖安装树。
8. 对 Release 重复 1–7，验证配置文件和 Debug postfix 不串配置。
9. 额外执行现有 `build Debug (CMake)` 与公共头检查，确认安装改造未影响 build-tree consumer。
10. Linux CI 重复安装 consumer；若生成导出含 `X11::X11`，验证 `VMPUIConfig.cmake` 已条件加载 `FindX11`。

## 9. 风险与验证建议

| 风险 | 等级 | 处理 |
|---|---:|---|
| 静态 `PRIVATE` 依赖通过 `LINK_ONLY` 泄漏为最终链接要求 | 高 | 不删除；完整安装归档，并在加载主 targets 前 `find_dependency` |
| `ui_fonts` 未安装或 `cmrc-base` 未导出 | 高 | CMRC 条件下三目标同一 `VMPUITargets`；独立 consumer 链接验证 |
| 上游子项目 install 默认关闭 | 高 | 根 CMake 在 `add_subdirectory` 前强制发布所需开关，并用清洁 build 验证 |
| HarfBuzz 导出引用 raw `freetype` target | 中 | `find_dependency(freetype CONFIG)` 必须先于 harfbuzz；不要擅自改上游导出名 |
| Yoga 绝对/错误 include 安装接口 | 高 | 修正为 prefix-relative include 根；扫描安装 targets 绝对路径 |
| FreeType/SDL 偶然发现系统可选依赖，导致包不自足 | 中 | 固定依赖选项；记录并检查生成 targets 的链接项 |
| MSVC `/MT` 与 consumer runtime 不一致 | 中 | package 文档明确当前静态 runtime 契约；Debug/Release 分别链接验证。后续可将 runtime 选择改成显式 package 选项，但不阻塞本次 |
| 安装上游全部附加库导致包膨胀 | 低 | 首版先保证正确；关闭已知 subset/test/util，剩余瘦身后置 |

## 10. 备选方案（不推荐首版）

将 `ui_fonts` 改成 CMRC `OBJECT` library，并把 `$<TARGET_OBJECTS:ui_fonts>` 物理并入 `ui`，可减少一个安装归档和 `cmrc-base` 导出。但这会改变资源归档拓扑、增量构建和链接行为，还需单独处理 CMRC 私有 include，风险高于直接安装现有 `ui_fonts`，不符合本次最小风险目标。

另一个看似简单但不推荐的方案是要求 consumer 在系统中自行安装 SDL/Yoga/FreeType 等，并只用 `find_dependency`。这会使 vendored 构建与 consumer 解析到的 ABI、编译选项和版本不一致，尤其在静态 CRT、SDL 后端和 FreeType 可选能力上风险较高。

## 11. 待确认问题

1. 首个 package 版本是否接受 `0.1.0`，还是仓库已有未写入 CMake 的正式版本号？
2. 安装包目标是“开发 SDK（含 co-installed vendored 依赖）”还是未来还需要制作可分发压缩包/CPack？本规划只覆盖前者。
3. 是否要求 Linux 安装树在首轮同时验收？若仅 Windows 收口，可先实现条件逻辑，但 Linux/X11 必须在后续 CI 实测。
