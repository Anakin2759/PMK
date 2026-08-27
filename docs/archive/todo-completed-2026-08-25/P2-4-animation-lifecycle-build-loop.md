# P2-4 动画生命周期测试构建闭环

- [x] 1. 重新配置 CMake（ENABLE_BUILD_TESTS=ON, ENABLE_BUILD_BENCHMARKS=ON）
- [x] 2. 构建 ui_ecs_tests
- [x] 3. 运行 AnimationLifecycleTest.* 测试
- [x] 4. 验证 5 个测试全部通过
- [x] 5. 失败则最小修复并重跑
- [x] 6. 生成测试报告 + 问题日志

结果：AnimationLifecycleTest 5/5 通过（6ms）；修复 5 处编译/运行问题；报告 `docs/test-reports/run-20260816-2135.md`；问题日志新增 2 条。