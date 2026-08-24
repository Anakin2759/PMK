# P2-1 ListView 构建验证

## 阶段
- [x] 1. Configure（纳入新增组件）— cmd /c D:\cmake\bin\cmake.exe 配置成功（tests+benchmarks 开启）
- [x] 2. Build all（example_ui_demo + ui_ecs_tests）— 退出码 0
- [x] 3. 架构门禁 + 公共头自包含门禁 — 均通过
- [x] 4. 编译错误最小修复 — 无编译错误；移除死代码 ApplyListItemVisualState（-Wunused-function）
- [x] 5. 测试运行（相关单测）— ui_ecs_tests 125/125 通过
- [x] 6. 报告 — docs/test-reports/run-20260816-1914.md

## 结果
- 架构基线：Factory.cpp `entt::entity` 21→28（+7 净增，P2-1 新增 8 处、死代码移除 1 处）；`UiRuntime::current()` 17 未变
- 修复文件：tools/check_architecture_boundaries.py、src/api/Factory.cpp（删死代码）
- 未提交、未推送
