# WP4-B GPU 生命周期问题日志

> 文档状态：RECORD（问题记录，不是执行规划）
> 最后复核：2026-08-27
> 当前结论：文中的早期失败均有后续标准 CTest 或直接测试复跑记录并通过；历史“待复跑”文字仅保留为当时快照。

## 2026-07-30 | unit/integration
- 工具：直接执行 `ui_unit_tests` Vulkan 定向压力测试
- Preset/Target：build / ui_unit_tests / GpuWindowLifecycleStressTest.VulkanCompletesOneHundredRealWindowLifecycles
- 失败摘要：测试在已由环境强制 `SDL_GPU_DRIVER=vulkan` 时，错误地断言同名 SDL hint 再次设置必须返回 true，导致生命周期循环未启动。
- 关键日志：
  ```text
  Value of: SDL_SetHint("SDL_GPU_DRIVER", std::string(expectedDriver).c_str())
    Actual: false
  Expected: true
  [  FAILED  ] GpuWindowLifecycleStressTest.VulkanCompletesOneHundredRealWindowLifecycles
  ```
- 初步定位：`tests/unittest/test_DeviceClaimState.cpp` / `RunGpuWindowLifecycleStress`
- 处理动作：自动修复
- 修复变更：改为核验当前生效的 `SDL_GPU_DRIVER` hint 与要求后端一致，不重复覆盖环境优先级 hint。
- 复跑结果：历史快照；已由后续 integration rerun 记录验证通过
- 关联条目：无

## 2026-07-30 | integration rerun
- 工具：标准 CTest 定向执行
- Preset/Target：build / CTest tests 46、47
- 失败摘要：无；两个 WP4-B 用例均由生成的 CTest 清单发现并通过。
- 关键日志：
  ```text
  GpuWindowLifecycleStressTest.Direct3D12CompletesOneHundredRealWindowLifecycles ... Passed 46.17 sec
  GpuWindowLifecycleStressTest.VulkanCompletesOneHundredRealWindowLifecycles ... Passed 39.35 sec
  100% tests passed, 0 tests failed out of 1
  ```
- 初步定位：标准 CTest 测试发现与执行路径正常
- 处理动作：环境前置缺失
- 修复变更：测试支持无外部 hint 时自行强制对应后端，并始终核验实际 driver。
- 复跑结果：通过
- 关联条目：上一条 VS Code 测试适配器未发现用例

## 2026-07-30 | integration rerun
- 工具：直接执行 `ui_unit_tests` Vulkan 定向压力测试
- Preset/Target：build / ui_unit_tests / GpuWindowLifecycleStressTest.VulkanCompletesOneHundredRealWindowLifecycles
- 失败摘要：无；关联测试笔误修复后复跑通过。
- 关键日志：
  ```text
  WP4B SUMMARY driver=vulkan passed=100 failed=0 skipped=0 totalLateReleaseSkipped=0 SDLVideoAfterFinalShutdown=0
  [  PASSED  ] 1 test.
  ```
- 初步定位：`tests/unittest/test_DeviceClaimState.cpp` / `RunGpuWindowLifecycleStress`
- 处理动作：自动修复
- 修复变更：无新增修复。
- 复跑结果：通过
- 关联条目：上一条 Vulkan hint 断言笔误

## 2026-07-30 | unit/integration
- 工具：VS Code 测试适配器定向执行
- Preset/Target：`test_DeviceClaimState.cpp` / 两个 GpuWindowLifecycleStressTest
- 失败摘要：测试适配器未发现该 CMake/GoogleTest 目标中的用例。
- 关键日志：
  ```text
  No tests found in the files. Ensure the correct absolute paths are passed to the tool.
  ```
- 初步定位：当前 VS Code 测试适配器发现配置；非测试代码或生命周期失败
- 处理动作：环境前置缺失
- 修复变更：无；改用项目已生成的 CTest 清单定向执行。
- 复跑结果：历史快照；已由后续 integration rerun 记录验证通过
- 关联条目：无
