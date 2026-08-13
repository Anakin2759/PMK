# WP4-A GPU 资源代际验证问题日志

## 2026-07-29 22:30 | test
- 工具：execute（列出 GTest/CTest 测试）
- Preset/Target：build / ui_unit_tests
- 失败摘要：复用终端残留输入污染 PowerShell 命令，导致解析失败；未执行测试二进制。
- 关键日志：
  ```text
  所在位置 行:1 字符: 56
  + ucmake --build build --config Debug --target ui_testsm& .\build\test ...
  +                                                        ~
  不允许使用与号(&)。
  FullyQualifiedErrorId : AmpersandNotAllowed
  ```
- 初步定位：终端会话残留输入/命令拼接，不涉及源码
- 处理动作：环境前置缺失
- 修复变更：无；后续改用不带调用运算符的独立进程启动方式复跑
- 复跑结果：通过（改用独立进程启动后成功列出测试）
- 关联条目：WP4-A M5 测试发现

## 2026-07-29 22:33 | integration
- 工具：execute（直接运行 ui_fallback_lifecycle_tests）
- Preset/Target：build / ui_fallback_lifecycle_tests
- 失败摘要：直接运行未带 CTest 注册的 `SDL_VIDEODRIVER=offscreen` 环境，实际使用 windows driver，断言失败。
- 关键日志：
  ```text
  Expected equality of these values:
    SDL_GetCurrentVideoDriver()
      Which is: "windows"
    "offscreen"
  [  FAILED  ] 1 test.
  ```
- 初步定位：tests/unittest/test_FallbackWindowLifecycle.cpp:35；执行环境与 CTest 属性不一致
- 处理动作：环境前置缺失
- 修复变更：无；按 CTest 同等环境设置 `SDL_VIDEODRIVER=offscreen` 后复跑
- 复跑结果：通过（offscreen 环境下 1/1）
- 关联条目：WP4-A M5 FallbackWindowLifecycle 定向测试

## 2026-07-29 22:40 | test
- 工具：execute（标准 CTest 结果确认复跑）
- Preset/Target：build / CTest all
- 失败摘要：新终端会话 PATH 未包含 CMake bin，`ctest` 命令未启动。
- 关键日志：
  ```text
  ctest : 无法将“ctest”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。
  FullyQualifiedErrorId : CommandNotFoundException
  ```
- 初步定位：终端 PATH 环境，不涉及源码或测试
- 处理动作：环境前置缺失
- 修复变更：无；改用 `D:\cmake\bin\ctest.exe` 绝对路径复跑
- 复跑结果：通过（绝对路径成功启动 CTest）
- 关联条目：WP4-A M5 标准 CTest

## 2026-07-29 22:40 | integration
- 工具：CTest（Debug 标准全量，209 项）
- Preset/Target：build / CTest all
- 失败摘要：FallbackWindowLifecycle 的 CTest 生成项未包含源码声明的 offscreen 环境，实际使用 windows driver；208/209 通过。
- 关键日志：
  ```text
  99% tests passed, 1 tests failed out of 209
  The following tests FAILED:
    209 - FallbackWindowLifecycleTest.CreatesAndClosesOneHundredOffscreenSoftwareWindows
  build/tests/unittest/ui_fallback_lifecycle_tests[1]_tests.cmake 未生成 ENVIRONMENT 属性
  ```
- 初步定位：tests/unittest/CMakeLists.txt:gtest_discover_tests(ui_fallback_lifecycle_tests) 属性参数排列；生成测试缺失 ENVIRONMENT/LABELS/RUN_SERIAL/RESOURCE_LOCK/TIMEOUT
- 处理动作：已记录待人工（CMake 明确禁止修改，越出本轮 A1～A19）
- 修复变更：无；仅在进程环境显式设置 `SDL_VIDEODRIVER=offscreen` 后复跑全量 CTest
- 复跑结果：通过（显式 `SDL_VIDEODRIVER=offscreen` 后 209/209）；标准无外部环境运行仍失败，待 CMake 边界外修复
- 关联条目：WP4-A M5 标准 CTest；C9

## 2026-07-29 22:59 | test
- 工具：execute（生成 CTest 属性与长日志检视）
- Preset/Target：build / CTest property inspection
- 失败摘要：持久 PowerShell 会话两次未识别内置语句起始 token，属性检视/日志尾部命令未完整执行；不涉及 configure、build 或 CTest 本体。
- 关键日志：
  ```text
  Get-ChildItem : 无法将“Get-ChildItem”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。
  $path : 无法将“$path”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。
  ```
- 初步定位：持久终端会话输入状态；生成文件与测试目标无异常
- 处理动作：环境前置缺失
- 修复变更：无；改用 .NET 文件 API 与无局部赋值的独立命令检视，并以 CTest quiet + LastTest.log 复核统计
- 复跑结果：通过（fallback 属性完整；裸标准 CTest exit=0，209/209）
- 关联条目：条目 1、条目 4；WP4-A M5 最终复验
