# ListView 测试闭环

- [x] 1. 重新配置（ENABLE_BUILD_TESTS=ON + BENCHMARKS=ON）
- [x] 2. 构建 ui_ecs_tests（初轮编译错误→修复）
- [x] 3. 运行 ListViewTest.*（5 个通过；初轮 2 个失败→修复 Factory.cpp 后全过）
- [x] 4. 最小修复并重跑（test_ListView.cpp 打印问题 + Factory.cpp texts 双写）
- [x] 5. 写测试报告与问题日志（docs/test-reports/run-20260816-1921.md；问题日志 +2 条）
