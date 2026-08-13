# PestManKill 品牌残留清单

> 日期：2026-08-13
> 性质：P0-5 工程卫生工作产物
> 说明：以下残留点均属于「独立拆分不彻底」的品牌痕迹。改动涉及运行时行为与外部接口，需用户决策后单独排期，本次仅记录。

---

## 残留位置汇总

| # | 文件:行 | 内容 | 类型 | 改动风险 |
|---|---|---|---|---|
| 1 | `src/utils/Logger.hpp:150` | `rotating_file_sink_mt("logs/pestmankill.log", ...)` | 运行时（日志文件路径） | 低，但会改变日志落盘文件名 |
| 2 | `src/utils/Logger.hpp:155` | `std::make_shared<spdlog::logger>("PestManKill", ...)` | 运行时（logger 名，出现在所有日志前缀 `%n`） | 中，影响全部日志前缀 |
| 3 | `src/common/AppConfig.hpp:111` | 注释「默认路径 `logs/pestmankill.log`」 | 注释 | 低，随 #1 同步 |
| 4 | `src/systems/render/RenderBackend.cpp:165` | `SDL_getenv("PESTMANKILL_FORCE_FALLBACK")` | **外部接口**（环境变量名） | 高，破坏已有脚本/CI 约定 |
| 5 | `src/systems/render/RenderBackend.cpp:176` | 日志文本 `PESTMANKILL_FORCE_FALLBACK` | 日志文本 | 低，随 #4 同步 |

---

## 分项说明

### 运行时品牌（#1、#2、#3）

- **logger 名 `"PestManKill"`** 通过 spdlog 的 `%n` 占位符打印到控制台与文件每行日志前缀，即此前 `logs/*.log` 中看到的 `PestManKill: [SystemManager] ...`。
- **日志文件 `logs/pestmankill.log`** 是默认落盘路径，`AppConfig::setLogFilePath` 可覆盖。
- 二者应一并改为项目正式名（如 `VMP-ui` / `vmpui`），并同步更新 `AppConfig.hpp:111` 注释。

### 外部接口（#4、#5）

- 环境变量 `PESTMANKILL_FORCE_FALLBACK` 是调试期强制 CPU fallback 的开关，可能已被本地脚本、CI 或文档引用。
- 若改名（如 `VMPUI_FORCE_FALLBACK`），需：① 全仓检索引用；② 更新相关文档；③ 提供旧名兼容过渡或明确 breaking。

---

## 建议处理方式

| 优先级 | 动作 | 决策点 |
|---|---|---|
| P2 | 统一改为 `VMP-ui`/`vmpui` 品牌（#1/#2/#3） | 需确认正式品牌名 |
| P2 | 环境变量更名 + 兼容过渡（#4/#5） | 需确认是否保留旧名兼容 |

> 注：本次 P0-5 仅完成「日志目录治理 + 垃圾文件清理 + 残留清单」，品牌改名本身属行为变更，未擅自改动。
