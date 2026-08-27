# P2-2 ContextMenu/ModalDialog 构建验证

- [x] 1. 确认测试文件与构建注册状态
- [x] 2. 重新配置 CMake（-DENABLE_BUILD_TESTS=ON -DENABLE_BUILD_BENCHMARKS=ON）
- [x] 3. 构建 ui_ecs_tests
- [x] 4. 运行 ContextMenuModalTest.*（预期 7 通过）→ 7/7 通过
- [x] 5. 失败则最小修复并重跑 → 无需修复
- [x] 6. 写测试报告 → docs/test-reports/run-20260816-1755.md
- [x] 7. 收尾全量回归（不做 clang-tidy）：all 构建退出码 0、架构/公共头门禁通过；ecs 125/125（ContextMenuModalTest 7/7）、unit 95/95、screenshot 3/3、interaction 2/2 → 报告 docs/test-reports/run-20260816-1803.md
- [x] 8. 修复项：架构守卫 baseline 同步（Factory.cpp entt::entity 14→21、UiRuntime::current() 13→17，见问题日志 2026-08-16 17:59）