# 俯视角像素战斗 MVP（roadside_stroll）

独立 **SFML 窗口**：四向分层（Domain / Application / Representation / Infrastructure），标题 → 战斗（移动 / 鼠标瞄准射击）、胜负与审计。

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

- **Enter** 或 **点击标题中的 Start 按钮**：开始战斗。
- **方向键**：移动；战斗中 **鼠标指向** 决定射击方向（360°）；**Space** 或 **按住鼠标左键**：开火。
- **T**：主题配色切换。
- **Esc** 或关闭窗口：退出。

## 分层

- `src/domain/`、`src/application/`：规则与 `RenderSnapshot`（含 `gameplay_active`、每帧 `player_move_step` / `player_step_*` 等**只读视图标记**，供表现层使用，无粒子逻辑）。
- `src/representation/`：`FrameComposer`、`SfmlGameWindow`、`movement_particles.cpp`（**脚步尘粒子**：生成/生命周期/绘制均在表现层，使用快照中的移动标记与本地 `std::mt19937`）。
- `src/infrastructure/`：计时、审计、RNG；`Win32ConsoleTerminal` 保留作旧控制台适配，主程序不再使用。
