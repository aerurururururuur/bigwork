---
name: roadside-stroll-cpp-architecture
description: >-
  Guides C++ pixel top-down combat MVP: four-layer architecture, DTO boundaries,
  IRandom/Seed, FrameTimer FPS+deltaTime, non-blocking input. Domain: TileBase,
  CombatEntity, World, IEnemyBehavior + combat ports. Application: RenderSnapshot
  only to Rep. Representation: FrameComposer (tiles), data-driven JSON sprites
  (SpriteSheetConfig / PlayerSpriteAnimator), SFML present order. Continuous
  actor coords + grid walkable. Legacy console ITerminal optional.
---

# 俯视角像素战斗（仓库沿用 skill 路径）

实现或审查代码时**优先遵守本节依赖与目录约定**。产品形态为 **单房间俯视角射击 MVP**：标题 → 战斗 → 胜利/失败 → 标题；**Representation 只读 `RenderSnapshot`**。战斗中 **瞄准方向由 Application 根据鼠标与格宽合成单位向量写入 `PlayerIntent`**，子弹朝向由快照中的 `rotation_deg` 在 SFML 层旋转绘制。

## 领域可扩展类型（基类体系）

- **`TileBase`**（`src/domain/tile_base.hpp`）：地板 / 墙 / 障碍等**地形语义**的公共根；具体类 `FloorTile`、`WallTile`、`ObstacleTile`；`PlayfieldGrid` 存 `TileKind` 枚举并通过 `tilePrototype(TileKind)` 解析为只读 `TileBase&`（便于新增瓦片子类而不改网格存储）。
- **`CombatEntity`**（`src/domain/combat_entities.hpp`）：**玩家 / 敌人 / 子弹** 等动态体的公共根；具体类 `PlayerActor`、`EnemyActor`、`BulletActor`；`World::simulateStep` 统一调度 `step(World&, double dt)`，新实体类型通过**继承 `CombatEntity`** 接入同一套步进与碰撞扩展点。
- **`IEnemyBehavior` + `makeEnemyBehavior`**（`src/domain/enemy_behavior.*`）：敌人 AI **策略对象**，按 `EnemyArchetype` 装配；与 `World` 的 **`IBulletFirePort` / `IMeleeEngagePort`**（`combat_ports.hpp`）组合注入，避免在行为里硬编码 `World` 细节，便于单测与换 AI。
- **`BulletActor` 体系**：`PlayerBulletActor` / `EnemyBulletActor` 共享弹道步进，命中策略由 `IBulletHitPolicy`（`bullet_hit_policy.*`）分派系解耦。

## 高解耦与可复用模板（优先复用）

以下模式已在仓库落地或推荐沿用；**换美术资源时优先改 JSON / 配置路径，少改 C++**。

### 数据驱动精灵（Representation）

| 组件 | 路径 | 职责 |
|------|------|------|
| **`SpriteSheetConfig`** | `representation/sprite_sheet_config.*` | 从 **`assets/sprites/player_sheet.json`** 解析图集：`texture`、`columns`、`rows`、`scale_cells`、`clips`（idle/run/death 段列表）。失败则 `valid == false`，调用方回退占位图。 |
| **`PlayerSpriteAnimator`** | `representation/player_sprite_animator.*` | **只读 `RenderSnapshot`**：idle/run/death 状态机、`texture_rect`、`apply_to_sprite`（脚底原点、与敌圆直径下限 `2*pr` 取 max、可选全局倍率、水平翻转由 `player_vx`）。**不写死行列**：换角改 JSON。 |
| **`SfmlGameWindow`** | `representation/sfml_game_window.*` | 构造期加载配置与纹理；`drawPlayer` 内 `update(dt,snap)` 再绘制；失败回退 `CircleShape`。 |

**可复用要点**：同一套 **`SpriteSheetConfig::load_from_file` + `SpriteLinearClip`** 可拷贝思路做 **`enemy_sheet.json` / `enemy_visuals.json`**（敌人多为单图 idle/walk 路径表，不必与玩家同 schema）。解析器可用 **轻量 regex 子集**（见 `sprite_sheet_config.cpp`），避免为 MVP 引入重型 JSON 依赖。

### 连续坐标 vs 网格权威

- **Domain**：玩家 / 敌人 / 子弹用 **浮点世界单位**（格宽 1.0）；**可走性、撞墙** 仍由 **`PlayfieldGrid::walkable`** + 圆采样（`World::terrainCircleWalkable` 等）判定。
- **Application**：`RenderSnapshot` 提供 `player_world_*`、`player_vx/vy`、`EnemyView` 浮点坐标等 **只读 DTO**；不重复积分物理。
- **Representation**：逻辑像素 = 世界坐标 × `cell_px`；**勿**在 Rep 里改 Domain 状态。

### 职业（AI）与外观（物种）分离（推荐）

- **`EnemyArchetype`**：近战 / 远程 / 精英混合等 **规则与数值**（`configureForArchetype`、分数等）。
- **物种 / 皮肤 ID**（若引入 `EnemySpriteId` 等）：**仅影响显示与资源路径**，与 `EnemyArchetype` **正交**；刷怪表 `(sprite_id → archetype)` 写在 Domain 或数据文件，避免用「职业枚举」硬编码贴图文件夹名。

### 可选运行时配置

- 仓库根提供 **`game_config.example.ini`**：可复制为 `game_config.ini` 置于工作目录（如 `build/Debug`），用于背景图等 **非核心玩法** 配置；**核心战斗随机与关卡权威仍在 Domain**。

## AI Agent Skills 列表（按四层归类）

以下 **Skill 名为能力域标签**：实现或拆任务时，先判定属于哪一层、哪一个原子能力，再把代码放进对应包（`infrastructure` / `domain` / `application` / `representation`），**禁止**为省事跨层直连。

### 一、Infrastructure（底层原子能力）

| Skill | 职责 |
|-------|------|
| **ConsoleDriver** | 控制台底层驱动（可选）：清屏、光标、ANSI/Win32。主路径为 **SFML** 时可弱化。 |
| **FrameTimer** | 帧计时：**稳定 FPS**；每帧提供 **`deltaTime`** 给 Application / Domain 步进（移动冷却、子弹积分等）。 |
| **KeyboardRawListen** | **非阻塞** 原始输入（SFML 路径下在 `SfmlGameWindow::pollInput` 聚合 `RawInputSnapshot`）。禁止阻塞 `getch()` 主循环。 |
| **RandomUtil** | `StdRandom` 等实现；领域只依赖端口 **`IRandom`**。 |
| **FileAuditSink** | 审计落盘（缓冲策略在实现内，避免卡帧）。 |
| **BgmCtrl** | 可选 BGM / 音效；经端口或 Infra 直调，**不**阻塞 `tick`。 |

### RandomUtil、`IRandom` 与 `Seed`

- **`IRandom`**：声明于 **Domain**；实现于 **Infrastructure**。
- **`Seed`**：Domain 值对象；与 `IRandom` 组合保证可复现生成（障碍散布、敌人生成位等）。

### 二、Domain（游戏世界规则）

| Skill | 职责 |
|-------|------|
| **TileBase / TileKind** | 地形可走性、`glyph()`；扩展新瓦片子类。 |
| **PlayfieldGrid** | 房间网格权威：`walkable` / `tile` / `setKind`。 |
| **CombatEntity** | 动态实体步进契约；子类实现具体 AI、弹道、射击冷却。 |
| **World** | 会话状态：`resetBattle`、`simulateStep`、`BattleOutcome`；持有 `IRandom&`；敌弹生成与剔除。 |
| **Seed** | 可复现种子。 |

### 三、Application（编排）

| Skill | 职责 |
|-------|------|
| **GameStateMachine** | `Title` / `Battle` / `Victory` / `Defeat` 迁移。 |
| **CombatTickFlow** | 每帧：`PlayerIntent` 聚合 → `World::setIntent` → `simulateStep(dt)` → 读结果写审计。 |
| **ViewSnapshotAssemble** | 组装 **`RenderSnapshot`**（`enemies` / `bullets` / `player_hp` 等）与 **`OverlayModel`**。 |

### 四、Representation（表现）

| Skill | 职责 |
|-------|------|
| **SceneLayerRender** | `FrameComposer`：`RenderSnapshot` → **`FrameCell` 仅铺地/墙/障碍**；**不再**用格网画玩家/敌人（避免与浮点精灵错位）。 |
| **KeyMappingTransform** | `RawInputSnapshot` → `GameCommand`（含 **`Fire`**）。 |
| **ThemeStyleRender** | `color_id` → `colorFromId`；主题只改配色，**不改** Domain 语义。 |
| **DialogUIPop** | `SfmlGameWindow` 最后绘制 `OverlayModel`（标题 / 结束面板）。 |
| **SpriteSheetLoad** | `SpriteSheetConfig` + JSON；失败回退几何占位。 |
| **ActorSpriteAnim** | `PlayerSpriteAnimator`：快照驱动动画与朝向。 |

### 逻辑格与屏幕像素

- **Domain**：`ScreenLayout::kCols` × `kRows`，**`kPlayfieldFraction = 1`** 时整屏为房间（`sky_rows = 0`）。
- **Representation**：`kScreenPixelsPerLogicalCell` 控制 SFML 正方形色块尺寸。
- **天空微格**：`sky_rows == 0` 时主循环不走微格细分；保留常量以兼容旧构图。

### 分层对照（快速落位）

```text
FrameTimer, KeyboardRawListen, StdRandom, FileAuditSink, BgmCtrl
  → infrastructure/

TileBase, PlayfieldGrid, CombatEntity, World, Seed, IRandom 端口
  → domain/

GameApplication, RenderSnapshot, OverlayModel, GameCommand, GameState
  → application/

FrameComposer, KeyMapping, ThemePalette, SfmlGameWindow, SpriteSheetConfig,
  PlayerSpriteAnimator, movement_particles, combat_vfx_particles
  → representation/
```

**输入链**：`RawInputSnapshot`（Rep 采集）→ `GameCommand` → `GameApplication::tick`。  
**时间链**：`FrameTimer` → **`deltaTime`** → `World::simulateStep`。  
**输出链**：Domain → **`RenderSnapshot`** → `FrameComposer` → `SfmlGameWindow`。

---

## 依赖方向（编译期）

```text
Representation → Application → Domain
Infrastructure → Domain
```

- **禁止** Representation `#include` Domain 实体或调用领域逻辑；只消费 DTO。
- **Domain**：无 SFML/文件实现；随机经 **`IRandom`**。
- **Application**：状态机与审计；不直接刷屏。
- **Infrastructure**：端口实现与 OS 细节。

## 画面与坐标（当前 MVP）

- 逻辑 **`Cols` × `Rows`** 近似 16:9；**整屏单房间**：墙体围边界，随机 `Obstacle`；**玩家与敌人为连续坐标**（速度积分 + 格上网格碰撞）；子弹同连续坐标。
- **Overlay**：不占玩法格；由 Application 填 `OverlayModel`。
- **资源路径**：`assets/sprites/`、`assets/fonts/`；运行 **工作目录** 需能解析相对路径（IDE 常从 `build/Debug` 启动时需自带 `assets` 或拷贝）。

## 帧合成顺序（SFML `present`）

1. `FrameComposer`：仅 **天空/地表瓦片** 色块。  
2. 粒子（脚尘、受击 VFX 等）。  
3. **敌人**（圆或精灵）→ **子弹** → **玩家**（最上层，含血条 HUD）。  
4. **Overlay**（标题 / 结算）。  

与早期「格网覆盖画敌/玩家」不同：**动态体一律在 SFML 层按世界坐标绘制**。

## 目录与 CMake（稳定骨架）

- `src/domain/` — `tile_base.*`、`combat_entities.*`、`playfield.*`、`world.*`、`enemy_behavior.*`、`bullet_hit_policy.*`、`score_state.*`、端口、`seed.hpp`
- `src/application/` — `game_application.*`、`render_snapshot.hpp`、`overlay_layout.*`、`game_command.hpp`、`game_state.hpp`
- `src/representation/` — `frame_composer.*`、`sfml_game_window.*`、`key_mapping.*`、`theme_palette.*`、`sprite_sheet_config.*`、`player_sprite_animator.*`、`movement_particles.*`、`combat_vfx_particles.*`
- `src/infrastructure/` — `frame_timer.*`、`file_audit_sink.*`、`std_random.*` 等
- `src/main.cpp` — 注入 `StdRandom` → `World`

## 审计与可选音频

- **Application** 在状态迁移、胜负时写审计。
- BGM/音效：可选；经 Infra，不阻塞模拟步。

## MVP 范围

- 标题 → 战斗 → 胜利/失败；四向移动 + **Space/左键开火**；敌人游荡/追踪；子弹撞墙与命中；接触伤害；**`RenderSnapshot` 单帧一致**。

## 实现前自检

- [ ] `#include` 是否违反 Rep→App→Dom / Inf→Dom？
- [ ] 新敌种/新瓦片是否落在 **`CombatEntity` / `TileBase` 子类** 或同层扩展点？
- [ ] 多实体与弹道是否只通过 **`RenderSnapshot`** 暴露给 Rep？
- [ ] **`World::simulateStep` 是否使用 `dt`**（冷却、子弹位移）？
- [ ] 随机是否只经 **`IRandom`**？
- [ ] 主题是否只改 **`color_id` 映射**？
- [ ] 输入是否**非阻塞**、主循环是否有 **FrameTimer**？
- [ ] 新精灵是否 **JSON 可换路径**；Rep 是否在加载失败时 **回退占位**而不崩？
- [ ] Domain 与 Rep **是否重复同一魔法数**（如近战距离阈值）；若必须重复是否在注释中写明「须与 `enemy_behavior` 同步」或抽到 **Domain 头文件常量** 由 Application 写入快照？

## 注意事项（反模式与边界）

1. **不要在 Representation 里 `#include` `world.hpp` 或改写 `EnemyActor`**；动画与贴图只依赖快照字段与本地时间累加器。  
2. **`EnemyArchetype` 扩展（如预留 `Boss`）**：必须同步 `makeEnemyBehavior`、`configureForArchetype`、分数等所有 `switch`，避免未处理枚举；未实装前可用占位行为并标 `TODO`。  
3. **子弹派系**：用 `BulletFaction` / `BulletFactionView` 区分绘制与命中策略，敌弹特殊贴图用 **快照枚举或 `uint8_t` 子类型**，避免在 Rep 里 `dynamic_cast` 跨 DLL/静态库边界（优先虚函数 `enemyBulletSprite()` 一类接口）。  
4. **Letterbox / 鼠标瞄准**：逻辑坐标用 `mapPixelToCoords`；瞄准向量在 **Application** 写入 `PlayerIntent`，与精灵朝向解耦（朝向可读 `player_vx` 或后续快照字段）。  
5. **CMake**：新增 `representation` 下 `.cpp` 须加入根 **`CMakeLists.txt`** 的 `roadside_representation` 源列表。

## 原始计划（本地）

早期公路企划：`C:/Users/12073/.cursor/plans/沿途公路信步架构_14bf9d47.plan.md`。战斗改造计划：`C:/Users/12073/.cursor/plans/元气骑士风战斗改造_f7b79992.plan.md`。与 skill 冲突时以**仓库已落地代码**为准。
