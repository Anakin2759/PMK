# 公开数学类型与 Eigen 边界决策

- 日期：2026-07-18
- 状态：accepted
- 影响范围：WP3、WP9，以及 Controls/Animation/Canvas/Utils/Factory/Image 公共 API

## 决策

VMP-ui 的公开 API 不再暴露 Eigen 类型。Eigen 仅作为内部运算实现依赖；数据进入 Eigen 运算前显式转换，运算结果返回公开边界前显式转换。

## 公共类型约束

- 引入 VMP-ui 自有 `ui::Vec2` 和 `ui::Rect`。
- 类型不依赖 Eigen、SDL、EnTT、Runtime 或内部组件。
- 类型必须是 standard-layout、trivially-copyable，并使用稳定的 `float` 标量布局。
- `Rect` 延续当前左上角位置加尺寸的模型，以及 `contains()` 的闭区间语义。
- 公共头禁止包含 Eigen，也禁止通过 alias、模板参数、返回值或 callback 载荷暴露 Eigen 类型。
- 只提供公共 API 实际需要的最小运算，不把 Eigen 的完整表达式接口复制到公共类型。

## 转换规则

- Eigen 转换函数放在内部实现层，不放入公共头。
- API 输入在进入矩阵、变换或向量计算时转换为内部 Eigen 类型。
- 运算结果离开内部层时转换回 `ui::Vec2`、`ui::Rect` 或其他公共值类型。
- 不允许通过公共 vector-like 隐式转换重新形成第三方 ABI 耦合；兼容构造若确有需要必须显式且不改变公共布局。

## 迁移顺序

1. 固定 `ui::Vec2`/`ui::Rect` 类型契约与测试。
2. 增加内部 Eigen 转换适配器并迁移运算边界。
3. 迁移 Controls、Animation、Canvas、Utils、Factory、Image 的公开签名和头路径。
4. 清理 `src/common/Types.hpp` 中公开 alias 的职责。
5. 将 Eigen 从 `ui` 的 PUBLIC 依赖收紧为 PRIVATE。
6. 使用独立 install/export consumer 验证公共头无需 Eigen。

## 兼容性

当前 `ui::Vec2 = Eigen::Vector2f` 的具体类型将发生变化，属于下一主版本允许的源码/ABI 收敛。迁移文档需列出 Eigen 特有调用（例如表达式模板和特定成员 API）的替代写法。内部算法可继续使用 Eigen，不要求重写数学实现。
