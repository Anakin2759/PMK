# PestManKill 品牌残留清单

> 日期：2026-08-13
> 性质：P0-5 工程卫生工作产物
> 状态：**✅ 2026-08-14 已全部改名完成**（用户决策「全改」）
> 文档状态：DONE（结果记录，不再作为待办）
> 说明：以下 6 处残留已统一改为 `VMP-ui`/`vmpui`。环境变量 `PESTMANKILL_FORCE_FALLBACK` → `VMPUI_FORCE_FALLBACK`
> 为 breaking 变更，旧名不再兼容；依赖旧名的脚本/CI 需同步更新。

---

## 残留位置汇总（均已处理）

| # | 文件:行 | 旧值 → 新值 | 状态 |
|---|---|---|---|
| 1 | `src/utils/Logger.hpp:150` | `logs/pestmankill.log` → `logs/vmpui.log` | ✅ |
| 2 | `src/utils/Logger.hpp:155` | `"PestManKill"` → `"VMP-ui"` | ✅ |
| 3 | `src/common/AppConfig.hpp:111` | 注释 `logs/pestmankill.log` → `logs/vmpui.log` | ✅ |
| 4 | `src/systems/render/RenderBackend.cpp:165` | `PESTMANKILL_FORCE_FALLBACK` → `VMPUI_FORCE_FALLBACK` | ✅ |
| 5 | `src/systems/render/RenderBackend.cpp:176` | 日志文本同步 #4 | ✅ |
| 6 | `example/ui_demo/View/menu.h:30` | `"PestManKill Menu"` → `"VMP-ui Menu"` | ✅ |

---

## 分项说明（历史记录，供追溯）

### 运行时品牌（#1、#2、#3）

- **logger 名 `"PestManKill"`** 通过 spdlog 的 `%n` 占位符打印到控制台与文件每行日志前缀。
- **日志文件 `logs/pestmankill.log`** 是默认落盘路径，`AppConfig::setLogFilePath` 可覆盖。
- 已改为 `"VMP-ui"` 与 `logs/vmpui.log`，`AppConfig.hpp` 注释同步。

### 外部接口（#4、#5）

- 环境变量已从 `PESTMANKILL_FORCE_FALLBACK` 改为 `VMPUI_FORCE_FALLBACK`，旧名不再兼容（breaking）。

---

## 结果

- 全仓 `src/`、`example/`、`include/`、`tests/` 检索 `PestManKill`/`pestmankill`/`PESTMANKILL` 均为空。
- 品牌改名完成，无残留。
