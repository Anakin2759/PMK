# Golden 基准图目录（P1-5 截图回归）

本目录存放截图回归的基准 PNG：

- `deterministic_scene.png`：offscreen + software 渲染的确定性场景基准。

## 如何生成/更新基准

首次运行 `ui_screenshot_tests` 时若缺少 golden，测试会 **SKIP** 并输出实际帧到：

```
build/tests/screenshot/screenshot-output/deterministic_scene.png
```

人工确认输出正确后，复制到本目录并入库：

```powershell
Copy-Item build/tests/screenshot/screenshot-output/deterministic_scene.png tests/golden/
```

之后 CI 与本地都会逐像素比较（每通道容差 4）。

## 注意事项

- 基准图与渲染分辨率、DPI 相关，生成时使用固定窗口尺寸（200×120）与 offscreen 软件渲染，
  避免硬件差异导致的抖动。
- 修改渲染输出时（如新增元素、改配色），需同步更新基准：先确认新输出正确，再复制入库。
