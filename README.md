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

- `music_bgm=`：相对或绝对路径，解析规则与 `background_image` 相同；推荐 **OGG** 流式播放（WAV 亦可）。用于 **Boss 登场前的全部杂兵阶段**（多波次清场间隙仍属该段）。
- `music_bgm_boss=`：可选；场上存在 **Boss 体型敌人**（`EnemyArchetype::Boss`）时播放。未配置时 Boss 阶段仍使用 `music_bgm`（若已配置）。
- `music_volume=`：可选，整数 **0–100**，缺省 **70**。
- **策略**：仅在 **Battle** 状态播放；标题 / 胜利 / 失败 **暂停**（两轨均暂停，各自保留进度）。Boss 出现时在两条音轨之间切换；若只配置了其中一条，则整场战斗使用该条。
- 未配置任何音乐、文件无法打开或解码失败时不会崩溃；打开失败时会在审计日志 `BOOT` 中记录说明。

示例见 [`game_config.example.ini`](game_config.example.ini)。

### 分数、连击与按分数加伤害（可选 INI）

- **HUD**：战斗中左上角显示 `Score:` 与下一行 `Damage: x…`（来自当前总分档）；连击倍率仍显示在分数行末 `xN`。其下为 **Wave x/y**（当前/杂兵总波数；波间休整时显示即将开始的波次）、**Enemies: n**（存活敌人数，含 Boss），若处于休整倒计时则多一行 **Next: xs**。
- **击杀得分**：近战 / 远程 / 精英 / Boss 各有基底分，再乘以连击倍率（1～8），并在短时窗口内维持连击层数；详见 `src/domain/score_state.cpp` 顶部注释。
- **Boss 蹭血得分**：Boss 每被玩家子弹扣 1 点实际生命，总分 +1（不计入连击）。
- **按总分加玩家输出伤害**：默认总分 **大于 100** 时普攻与 Q 技能子弹 **×1.2**，**大于 300** 时 **×1.5**（高档覆盖低档）。可在 `game_config.ini` 中覆盖，键名见 `game_config.example.ini`；若 `tier2_score ≤ tier1_score` 则整组恢复默认。

### 多波次杂兵、玩家子弹射程与胜利条件

- Boss 前杂兵波次、休整时长、每波数量与障碍散布由 `game_config.ini` 中 `wave_*` 键配置（缺省与原先内置数值一致：3 波、休整约 2 秒、生成数量随波次略增有上限）；清光非 Boss 敌人后进入休整，再随机散布障碍并刷新下一波。
- **胜利**：仅当 **Boss 已生成**（`boss_released` 语义）且场上 **无任何存活敌人** 时判定；波间场上暂时无人 **不会** 判胜。
- **玩家子弹最大飞行距离**：杂兵阶段较短，Boss 登场后自动切换为远距离档；普攻与 **Q 环形齐射** 共用 `World::spawnPlayerBullet`。具体世界单位见 `kPlayerMobBulletTravelWorld` / `kPlayerBossBulletTravelWorld`。
- **杂兵远程 / 精英远程弹**：最大飞行距离与杂兵阶段主角子弹相同（`kPlayerMobBulletTravelWorld`）；Boss 本体弹幕仍不限该距离。
- 杂兵与远程弹的 **血量、射速、移速、敌弹伤害** 等压迫向数值亦集中在 `wave_combat_tuning.hpp`，便于答辩时统一说明「难度表」。
- **可选扩展**：波次开始/ Boss 现身的短音效、对 `sf::View` 的轻微镜头抖动未默认接入，可在 `GameApplication` 或 `SfmlGameWindow` 层按快照字段自行挂接。

### 击杀掉落：红心 / 蓝心

- 击杀敌人时按概率在死亡点附近生成 **红心**（回血）或 **蓝心**（回蓝），每击杀最多掉落 **一种**（互斥随机）。杂兵与 Boss 使用不同掉落率，常量见 [`src/domain/pickup_drop_tuning.hpp`](src/domain/pickup_drop_tuning.hpp)（`kMobDropRedPct` / `kMobDropBluePct`、`kBossDrop*`、回复量 `kRedHeartHealHp` / `kBlueHeartHealMp`、拾取距离 `kPickupRadiusWorld`）。
- **拾取**：玩家脚点与掉落物中心距离小于拾取半径即可拾取。**满血**时不会吃掉红心，**满蓝**时不会吃掉蓝心（道具留在地面）。
- 掉落生成在可走地形上；若死亡点不可走，会在小范围内尝试偏移若干次。

### 开发模式与 Boss 技能热键（可选）

在 `game_config.ini` 中设置 `run_mode=development`（大小写不敏感；缺省或 `production` 为正式行为）。仅在开发模式下，**战斗中**且**无全屏遮罩**时：

- 主键盘 **X**：**当帧秒杀场上全部敌人**（含 Boss；走正常 `applyDamage`，击杀分与掉落逻辑照常）。
- 主键盘 **1–8**：单次触发当前 Boss 的对应技能（需场上存在 Boss）：

| 键 | 技能 |
|----|------|
| 1 | 环形扩散第一圈 |
| 2 | 环形扩散第二圈 |
| 3 | 定向扇形 |
| 4 | 螺旋快照弹幕 |
| 5 | 左右双向扇形 |
| 6 | 内外对向双环 |
| 7 | 软追踪散弹 |
| 8 | 侧墙横排齐射 |

弹幕几何与复用逻辑在 `domain/boss_pattern_spawn.*`；`skill.cpp` 中的 `boss*Fire` 仅做 `SkillCastContext` 校验与调参。生产模式不启用 **X** 秒杀与 **1–8** Boss 技能热键。

### Boss 血量阶段与符卡（自动）

Boss AI（`domain/enemy_behavior.cpp` 中 `BossHybridBehavior`）按 **当前 HP / maxHp** 切换模式；阈值常量见 [`domain/boss_hp_spell_constants.hpp`](src/domain/boss_hp_spell_constants.hpp)（默认 **>70%** 为前期，**≤70%** 进入中后期符卡）。

- **前期（>70%）**：仍为「双圈扩散 ↔ 扇形」交替，由 `ring_burst_rem_` 计时触发。
- **中后期（≤70%）**：`ring_burst_rem_` 到期后进入符卡链：**前摇 → 第一圈 → 等待 → 第二圈 → 短间隔 → 双向扇形 → 短间隔 → 软追踪散弹 → 短间隔 → 内外对向双环 → 虚弱**。虚弱期内 **不递减** `ring_burst_rem_`，且 Boss **承受玩家子弹伤害 ×1.5**（通过 `EnemyActor::incomingDamageMultiplier`）；虚弱结束恢复倍率并重新武装大 CD。

≤40% 与 (40%,70%] 目前共用同一套符卡；若需差异化可只改常量表与 `BossHybridBehavior` 分支。

## 操作

- **标题**：**↑ / ↓（方向键）** 在菜单项间移动；**Enter** 确认。主菜单为 **开始游戏** 与 **切换角色**；在「切换角色」上确认后进入 **选角**（Role1 / Role2），再 **Enter** 保存并回到主菜单；在「开始游戏」上 **Enter** 才进入战斗。返回标题（胜/败后）菜单会回到主界面第一项，**上次在标题选中的角色会保留**（新开局不再强制重置为 Role1）。
- **WASD**：战斗中移动；战斗中 **鼠标指向** 决定射击方向（360°）；**Space** 或 **按住鼠标左键**：普攻（不耗 MP，受射速冷却）；**Q**：全周环射；**E**：窄扇形副武器（有 MP 与冷却）。MP 随时间缓慢自然回复。
- **Tab**：战斗中在 **Role1 / Role2** 主角外观间快捷切换（调试/体验用）；**正式选角在标题界面「切换角色」中完成**。Role2 时玩家子弹为书本贴图条带（`assets/sprites/bullets/Book.png`，按列拆帧绘制）；Role1 单图集见 `assets/sprites/player_sheet.json`（纹理路径指向 `assets/sprites/role/role1/`），Role2 多条纹见 `assets/sprites/player_sheet_role2.json`（纹理在 `assets/sprites/role/role2/`）。
- **T**：主题配色切换。
- **Esc** 或关闭窗口：退出。

## 分层

- `src/domain/`、`src/application/`：规则与 `RenderSnapshot`（含 `gameplay_active`、每帧 `player_move_step` / `player_step_*` 等**只读视图标记**，供表现层使用，无粒子逻辑）。
- `src/representation/`：`FrameComposer`、`SfmlGameWindow`、`movement_particles.cpp`（**脚步尘粒子**：生成/生命周期/绘制均在表现层，使用快照中的移动标记与本地 `std::mt19937`）。
- `src/infrastructure/`：计时、审计、RNG；`Win32ConsoleTerminal` 保留作旧控制台适配，主程序不再使用。
