# 沿途公路信步（roadside_stroll）MVP

独立 **SFML 窗口** 的字符像素演示：四向分层（Domain / Application / Representation / Infrastructure），标题 → 漫游（下带行走 + 上带视差占位）、文件审计。

## 环境

- Windows 10+（推荐；字体默认尝试 `C:/Windows/Fonts/consola.ttf`）。
- C++17、CMake 3.16+、MSVC 或 MinGW。
- **SFML 2.5+**（`graphics` / `window` / `system` / `main`）。未安装时 CMake 可自动拉取 2.6.1（`ROADSIDE_STROLL_FETCH_SFML=ON`，默认开启）。

## 构建与运行

在仓库根目录：

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\roadside_stroll.exe
```

Visual Studio 多配置生成器请始终带 `--config Release`；单配置生成器可省略。

若已用 vcpkg 等安装 SFML，可关闭自动下载：

```powershell
cmake -S . -B build -DROADSIDE_STROLL_FETCH_SFML=OFF -DCMAKE_PREFIX_PATH=<SFML 安装前缀>
```

运行后弹出独立窗口（约 864×486 逻辑格 48×27、每格 18px），**不要**在集成终端里当控制台游戏运行。

审计日志：`logs/game_audit.log`（退出时刷盘）。

## 操作

- **Enter**：标题进入漫游。
- **方向键**：可玩条带内移动。
- **T**：黄昏 / 冷夜主题。
- **Esc** 或关闭窗口：退出。

## 分层

- `src/domain/`、`src/application/`：规则与 `RenderSnapshot`。
- `src/representation/`：`FrameComposer`、`SfmlGameWindow`（窗口绘制与 SFML 输入）。
- `src/infrastructure/`：计时、审计、RNG；`Win32ConsoleTerminal` 保留作旧控制台适配，主程序不再使用。
