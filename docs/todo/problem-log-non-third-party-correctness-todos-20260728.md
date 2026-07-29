# 非第三方 TODO 正确性闭环问题日志

## 2026-07-28 23:58 | build
- 工具：execute `cmake --build build --config Debug --target example_ui_demo`
- Preset/Target：无 preset / `example_ui_demo` / Debug
- 失败摘要：PowerShell 短暂未解析到 `cmake` 命令，未进入实际构建。
- 关键日志：
  ```text
  CategoryInfo          : ObjectNotFound: (cmake:String) [], CommandNotFoundException
  FullyQualifiedErrorId : CommandNotFoundException
  ```
- 初步定位：终端环境/PATH 短暂异常；随后 `Get-Command cmake` 已解析为 `D:\cmake\bin\cmake.exe`。
- 处理动作：环境前置缺失
- 修复变更：无源码变更；后续使用 CMake 绝对路径复跑。
- 复跑结果：未复跑
- 关联条目：M4 Debug 全量验证

## 2026-07-29 00:02 | build
- 工具：execute `cmake --build build --config Debug --target ui_tests`
- Preset/Target：无 preset / `ui_tests` / Debug
- 失败摘要：`ui_unit_tests.exe` 链接成功，但构建期 GoogleTest 用例发现进程超过 5 秒超时。
- 关键日志：
  ```text
  Error running test executable.
  Path: 'D:/code/VMP-ui/build/tests/unittest/ui_unit_tests.exe'
  Result: Process terminated due to timeout
  TEST_DISCOVERY_TIMEOUT=5
  ```
- 初步定位：`ui_unit_tests` 启动/`--gtest_list_tests` 阶段；尚无证据指向 C1～C3 测试正文。
- 处理动作：已记录待人工
- 修复变更：无；CMake 修改被任务边界明确禁止。
- 复跑结果：仍失败
- 关联条目：M4 Debug 全量验证

### 复跑与替代验证补充（2026-07-29 00:08）

- 第 2 次 `ui_tests` 聚合构建复跑仍在 `ui_unit_tests` 的 5 秒发现阶段超时，按失败策略停止修复尝试。
- `ui_unit_tests.exe --gtest_list_tests` 可直接完成；直接执行全部测试程序共 180 项：176 通过、4 个 GPU 用例跳过、0 失败。
- `ui_ecs_tests`、`ui_api_tests`、`ui_fallback_lifecycle_tests` 同样完成链接，但各自构建后置发现均受相同 5 秒超时影响。
- GPU 跳过原因：D3D12、Vulkan 均因 `Video subsystem not initialized` 未建立设备。

### 复跑补充（2026-07-28 23:59）

- 使用 `D:\cmake\bin\cmake.exe --build build --config Debug --target example_ui_demo` 复跑通过。

## 2026-07-29 00:00 | unit/integration
- 工具：execute CTest 发现与全量执行命令
- Preset/Target：build / 全部 CTest / Debug
- 失败摘要：终端接收到带多余字符的 PowerShell 命令，解析阶段失败，未执行测试。
- 关键日志：
  ```text
  u& 'D:\cmake\bin\ctest.exe' ... --output-on-failurem& ...
  不允许使用与号(&)。
  FullyQualifiedErrorId : AmpersandNotAllowed
  ```
- 初步定位：终端输入污染；不是测试或业务代码失败。
- 处理动作：环境前置缺失
- 修复变更：无源码变更；拆分为单条、无调用运算符的命令复跑。
- 复跑结果：未复跑
- 关联条目：M4 Debug 全量验证

## 2026-07-29 00:12 | build
- 工具：execute `cmake --build build --config Debug --target ui_tests`
- Preset/Target：无 preset / `ui_tests` / Debug
- 失败摘要：GoogleTest discovery 固定 5 秒超时仍存在，本次首先阻塞于 `ui_ecs_tests`。
- 关键日志：
  ```text
  Error running test executable.
  Path: 'D:/code/VMP-ui/build/tests/unittest/ui_ecs_tests.exe'
  Result: Process terminated due to timeout
  TEST_DISCOVERY_TIMEOUT=5
  ninja: build stopped: subcommand failed.
  ```
- 初步定位：`tests/unittest/CMakeLists.txt` 的 `gtest_discover_tests` 使用默认 5 秒 discovery timeout；测试程序直接执行可在超时之外正常完成。
- 处理动作：已记录待人工
- 修复变更：无；调整 CMake 明确超出本工作包范围，需要用户授权。
- 复跑结果：仍失败
- 关联条目：上一轮 build discovery timeout

## 2026-07-29 00:13 | unit
- 工具：execute GPU 重点用例直接执行命令
- Preset/Target：`ui_unit_tests` / 4 个 GPU 用例
- 失败摘要：首次直接执行命令的 PowerShell `Push-Location` 未被当前终端会话解析，测试未启动。
- 关键日志：
  ```text
  Push-Location : 无法将“Push-Location”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。
  .\ui_unit_tests.exe : 无法将“.\ui_unit_tests.exe”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。
  ```
- 初步定位：终端会话命令解析异常；与测试、SDL 或 GPU 后端无关。
- 处理动作：环境前置缺失
- 修复变更：无源码变更；改用测试程序绝对路径复跑。
- 复跑结果：通过
- 关联条目：A8/A9 GPU fixture 验证

## 2026-07-29 00:15 | unit/integration
- 工具：execute `ctest --test-dir build -C Debug --output-on-failure`
- Preset/Target：build / 全部 CTest / Debug
- 失败摘要：构建期 discovery 超时未生成测试注册文件，CTest 仅得到 4 个 `*_NOT_BUILT` 占位项。
- 关键日志：
  ```text
  ui_unit_tests_NOT_BUILT .................***Not Run
  ui_ecs_tests_NOT_BUILT ..................***Not Run
  ui_api_tests_NOT_BUILT ..................***Not Run
  ui_fallback_lifecycle_tests_NOT_BUILT ...***Not Run
  0% tests passed, 4 tests failed out of 4
  ```
- 初步定位：标准 CTest 路径被既有 `gtest_discover_tests` 5 秒超时连带阻塞，并非测试正文失败。
- 处理动作：已记录待人工
- 修复变更：无；CMake 修改越界，需要用户授权后调整 discovery timeout。
- 复跑结果：未复跑
- 关联条目：2026-07-29 00:12 build

## 2026-07-29 | unit/integration
- 工具：execute `ctest --test-dir build -C Debug --output-on-failure`
- Preset/Target：无 preset / build / 全部 CTest / Debug
- 失败摘要：终端再次混入先前构建命令残留字符，PowerShell 在解析阶段失败，CTest 未启动。
- 关键日志：
  ```text
  u& 'D:\cmake\bin\cmake.exe' --build build --config Debug --target ui_ ...
  不允许使用与号(&)。
  FullyQualifiedErrorId : AmpersandNotAllowed
  ```
- 初步定位：终端输入污染；与 discovery timeout 修复及测试正文无关。
- 处理动作：环境前置缺失
- 修复变更：无；改用不含 PowerShell 调用运算符的绝对路径命令复跑。
- 复跑结果：未复跑
- 关联条目：2026-07-29 00:00 unit/integration

## 2026-07-29 | unit/integration
- 工具：execute `ctest --test-dir build -C Debug --output-on-failure` 汇总复验
- Preset/Target：无 preset / build / 全部 CTest / Debug
- 失败摘要：终端会话在汇总复验时未解析此前可执行的 `D:\cmake\bin\ctest.exe` 绝对路径，CTest 未启动。
- 关键日志：
  ```text
  D:\cmake\bin\ctest.exe : 无法将“D:\cmake\bin\ctest.exe”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。
  FullyQualifiedErrorId : CommandNotFoundException
  ```
- 初步定位：终端会话命令解析异常；第一次有效 CTest 已执行至已记录的 180 项测试序列，但其工具输出末尾被截断。
- 处理动作：环境前置缺失
- 修复变更：无。
- 复跑结果：未复跑
- 关联条目：本轮上一条 unit/integration
