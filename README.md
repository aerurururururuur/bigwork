# 俯视角像素战斗 MVP（roadside_stroll）

独立 **SFML 窗口**：四向分层（Domain / Application / Representation / Infrastructure），标题 → 战斗（移动 / 鼠标瞄准射击）、胜负与审计。

## 环境

- Windows 10+（推荐；字体默认尝试 `C:/Windows/Fonts/consola.ttf`）。
- C++17、CMake 3.16+、MSVC 或 MinGW。
- **SFML 2.5+**（`graphics` / `window` / `system` / `audio` / `main`）。未安装时 CMake 可自动拉取 2.6.1（`ROADSIDE_STROLL_FETCH_SFML=ON`，默认开启）。若运行时提示缺少 **OpenAL**，请将 `OpenAL32.dll` 放到 exe 同目录（可与 SFML 预编译包中的 DLL 一并拷贝；使用 vcpkg 时通常由依赖链提供）。

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

### 自定义全屏背景图（可选）

程序使用 `loadGameConfigAuto()`：在**当前工作目录及其最多 5 层上级目录**中查找 `game_config.ini`（因此从 `build/Debug` 启动 exe 时，仍可使用**仓库根目录**下的 ini）。`background_image` 为相对路径时，会先按「ini 所在目录 + 相对路径」查找，再按「cwd 及每一级父目录 + 相对路径」查找，这样资源通常放在仓库根的 `assets/...` 也能被加载。

示例见 [`game_config.example.ini`](game_config.example.ini)。Windows 下可写 `assets/background/foo.png` 或 `assets\\background\\foo.png`。

```ini
# 注释行以 # 开头
background_image=assets/your_background.png
```

- 仅识别第一个 `background_image=`；路径可为相对或绝对；支持常见 PNG/JPG（取决于 SFML 与系统解码器）。
- 图片会**拉伸**铺满整个逻辑画布；地板与天空格子（`FrameCell` 为 `'.'`）不再画纯色块，墙 `#` 与障碍 `O` 仍覆盖在图片之上。
- 文件缺失、路径无效或加载失败时自动回退为原来的纯色背景（无崩溃）。若 ini 中写了 `background_image=`，启动时会在 `logs/game_audit.log` 的 `BOOT` 中记录解析后的绝对路径（若解析失败则可能仍记录 ini 中的原始字符串，取决于是否成功打开 ini）。

### 背景音乐 BGM（可选）

- `music_bgm=`：相对或绝对路径，解析规则与 `background_image` 相同；推荐 **OGG** 流式播放（WAV 亦可）。用于**清完第一波小怪后、Boss 出现前**的普通战斗段。
- `music_bgm_boss=`：可选；场上存在 **Boss 体型敌人**（`EnemyArchetype::Boss`）时播放。未配置时 Boss 阶段仍使用 `music_bgm`（若已配置）。
- `music_volume=`：可选，整数 **0–100**，缺省 **70**。
- **策略**：仅在 **Battle** 状态播放；标题 / 胜利 / 失败 **暂停**（两轨均暂停，各自保留进度）。Boss 出现时在两条音轨之间切换；若只配置了其中一条，则整场战斗使用该条。
- 未配置任何音乐、文件无法打开或解码失败时不会崩溃；打开失败时会在审计日志 `BOOT` 中记录说明。

示例见 [`game_config.example.ini`](game_config.example.ini)。

## 操作

- **Enter** 或 **点击标题中的 Start 按钮**：开始战斗。
- **方向键**：移动；战斗中 **鼠标指向** 决定射击方向（360°）；**Space** 或 **按住鼠标左键**：开火。
- **T**：主题配色切换。
- **Esc** 或关闭窗口：退出。

## 分层

- `src/domain/`、`src/application/`：规则与 `RenderSnapshot`（含 `gameplay_active`、每帧 `player_move_step` / `player_step_*` 等**只读视图标记**，供表现层使用，无粒子逻辑）。
- `src/representation/`：`FrameComposer`、`SfmlGameWindow`、`movement_particles.cpp`（**脚步尘粒子**：生成/生命周期/绘制均在表现层，使用快照中的移动标记与本地 `std::mt19937`）。
- `src/infrastructure/`：计时、审计、RNG；`Win32ConsoleTerminal` 保留作旧控制台适配，主程序不再使用。
