---
name: roadside-stroll-cpp-architecture
description: >-
  Guides C++ pixel top-down combat MVP: four-layer architecture, DTO boundaries,
  IRandom/Seed, FrameTimer FPS+deltaTime, non-blocking input. Domain extends via
  TileBase (floor/wall/obstacle) and CombatEntity (player/enemy/bullet); SFML
  square tiles from RenderSnapshot. Legacy console ITerminal optional.
---

# 俯视角像素战斗（仓库沿用 skill 路径）

实现或审查代码时**优先遵守本节依赖与目录约定**。产品形态为 **单房间俯视角射击 MVP**：标题 → 战斗 → 胜利/失败 → 标题；**Representation 只读 `RenderSnapshot`**。战斗中 **瞄准方向由 Application 根据鼠标与格宽合成单位向量写入 `PlayerIntent`**，子弹朝向由快照中的 `rotation_deg` 在 SFML 层旋转绘制。

## 领域可扩展类型（基类体系）

- **`TileBase`**（`src/domain/tile_base.hpp`）：地板 / 墙 / 障碍等**地形语义**的公共根；具体类 `FloorTile`、`WallTile`、`ObstacleTile`；`PlayfieldGrid` 存 `TileKind` 枚举并通过 `tilePrototype(TileKind)` 解析为只读 `TileBase&`（便于新增瓦片子类而不改网格存储）。
- **`CombatEntity`**（`src/domain/combat_entities.hpp`）：**玩家 / 敌人 / 子弹** 等动态体的公共根；具体类 `PlayerActor`、`EnemyActor`、`BulletActor`；`World::simulateStep` 统一调度 `step(World&, double dt)`，新实体类型通过**继承 `CombatEntity`** 接入同一套步进与碰撞扩展点。

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
| **SceneLayerRender** | `FrameComposer`：`RenderSnapshot` → `FrameCell` 色块 id（地/墙/障/敌/弹/玩家分层覆盖顺序）。 |
| **KeyMappingTransform** | `RawInputSnapshot` → `GameCommand`（含 **`Fire`**）。 |
| **ThemeStyleRender** | `color_id` → `colorFromId`；主题只改配色，**不改** Domain 语义。 |
| **DialogUIPop** | `SfmlGameWindow` 最后绘制 `OverlayModel`（标题 / 结束面板）。 |

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

FrameComposer, KeyMapping, ThemePalette, SfmlGameWindow
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

- 逻辑 **`Cols` × `Rows`** 近似 16:9；**整屏单房间**：墙体围边界，随机 `Obstacle`，玩家与敌人网格移动，子弹浮点飞行。
- **Overlay**：不占玩法格；由 Application 填 `OverlayModel`。

## 帧合成顺序

`FrameComposer`：瓦片底色 → 敌人 → 子弹 → **玩家最上**；Overlay 在 `present` 末尾。

## 目录与 CMake（稳定骨架）

- `src/domain/` — `tile_base.*`、`combat_entities.*`、`playfield.*`、`world.*`、端口、`seed.hpp`
- `src/application/` — `game_application.*`、`render_snapshot.hpp`、`overlay_layout.*`、`game_command.hpp`、`game_state.hpp`
- `src/representation/` — `frame_composer.*`、`sfml_game_window.*`、`key_mapping.*`、`theme_palette.*`
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

## 原始计划（本地）

早期公路企划：`C:/Users/12073/.cursor/plans/沿途公路信步架构_14bf9d47.plan.md`。战斗改造计划：`C:/Users/12073/.cursor/plans/元气骑士风战斗改造_f7b79992.plan.md`。与 skill 冲突时以**仓库已落地代码**为准。
