---
name: roadside-stroll-cpp-architecture
description: >-
  Guides C++ console character-pixel work for «沿途公路信步»: four-layer
  architecture, DTO boundaries, IRandom/Seed, FrameTimer FPS+deltaTime,
  non-blocking KeyboardRawListen, CharPixel symbol+color_attr in Representation.
  Skills: ConsoleDriver, FrameTimer, KeyboardRawListen, RandomUtil, MapGridRule,
  GameStateMachine, GameMainLoop, ViewSnapshotAssemble, SceneLayerRender,
  ParallaxScroll, ThemeStyleRender, etc. Use for refactor, CMake, or 沿途公路信步.
---

# 《沿途公路信步》控制台工程架构

基于项目计划的四向分层与画面规范；实现或审查代码时**优先遵守本节依赖与目录约定**。

## AI Agent Skills 列表（按四层归类）

以下 **Skill 名为能力域标签**：实现或拆任务时，先判定属于哪一层、哪一个原子能力，再把代码放进对应包（`infrastructure` / `domain` / `application` / `representation`），**禁止**为省事跨层直连。

### 一、Infrastructure（底层原子能力）

| Skill | 职责 |
|-------|------|
| **ConsoleDriver** | 控制台底层驱动：清屏、光标隐藏、光标坐标定位；控制台文字颜色 / 主题色调底层配置（与 ThemeStyleRender 分工：此处为终端 API/ANSI，主题为表现层常量与绘制策略）。 |
| **FrameTimer** | 高精度帧计时：**稳定 FPS**（如 30 / 60），避免 `while(true)` 空转**占满 CPU**；每帧向 **Application**（或主循环编排层）提供 **`deltaTime`**，供视差滚动平滑位移、节奏型动画与逻辑积分。实现常用 `sleep_until`/单调时钟差分；若需可抽象为 **`IClock`/`IFramePhase`** 由 Domain 仅见接口、Infra 实现。 |
| **KeyboardRawListen** | 原生键盘**非阻塞**监听（**强制**）：原始码上下左右、确认、取消、日志快捷键。**禁止**把主循环建立在会**挂起线程**的 `getch()` / 默认阻塞 `cin>>` 上——否则无法稳定帧、也谈不上「实时行走」。**Win32**：`_kbhit()`、`GetAsyncKeyState` 等；**Unix**：`termios` 非规范模式 + `select`/轮询等。该实现选择直接决定体验是**伪实时行走**还是退化为**回合式**；代码归属 **Infrastructure**。语义键仍由 **KeyMappingTransform**（Rep）产出 `GameCommand`。 |
| **RandomUtil** | **Infrastructure** 内具体 RNG / 哈希流等**实现**；领域通过 Domain 端口 **`IRandom`** 取数（见下节 **Seed**）。不得把「可复现性」完全 opaque 在 Infra：地图/会话种子等由 **Domain 的 `Seed` 值对象**显式建模，便于**确定性单测**与跑档复现。 |
| **FilePersistence** | 落盘存档：旅途日志读写、任务进度保存、NPC 相遇记录持久化（仅 I/O 与序列化，不写业务规则）。 |
| **AssetPathConfig** | 资源路径配置：文案库、对话库、场景模板路径加载与解析入口。 |
| **BgmCtrl** | 背景音乐 / 氛围音：启停、切换「公路晚风」等氛围音（实现体在 Infra；若存在 `IBgmPlayer` 端口则 Domain/App 只依赖抽象）。 |

### RandomUtil、`IRandom` 与 `Seed`（分层细节）

- **`IRandom`**：**接口声明于 Domain**（端口）；算法、熵源、平台细节**零**放在 Domain。
- **`RandomUtil`**：**实现于 Infrastructure**，注入为 `IRandom` 的具体适配器。
- **`Seed`**：**Domain** 内简单**值对象/实体**（如地图种子、会话种子、跑档种子），由用例或 `World` 构造显式持有并传入规则；单测固定 `Seed` + Fake `IRandom` 即可对生成/游荡/场景块等做**稳定断言**。玩法随机仍**只经 `IRandom`**，不绕过端口。

### 二、Domain（游戏世界规则原子能力）

| Skill | 职责 |
|-------|------|
| **Seed** | **可复现性载体**（非 Infra Skill）：地图/会话等种子，**无 I/O**；与 `IRandom` 实现组合，支撑确定测试与回放语义（见上节）。 |
| **PlayerModelManage** | 玩家数据模型：坐标、行程、状态、履历等**领域数据**维护（无控制台、无文件）。 |
| **NpcModelManage** | NPC 实体模型：生成、坐标、人设、台词集、交互状态等**领域状态**维护。 |
| **TaskModelManage** | 任务领域模型：创建、状态流转、条件定义、与 NPC 绑定等**纯规则与结构**。 |
| **MapGridRule** | 二维网格可走规则：坐标合法性、障碍碰撞、行走边界校验（权威网格语义，对齐下 1/4 **PlayfieldGrid**）。 |
| **NpcSpawnRule** | NPC 生成与游荡：沿路刷新、随机游走、停留待机等**规则**（随机数经端口）。 |
| **InteractJudgeRule** | 玩家–NPC 交互判定：邻格距离、触发对话条件等**校验规则**。 |
| **SceneGenRule** | 沿途场景生成规则：公路、行道树、长椅、便利店等场景块随机/组合生成（无 I/O；读表由 Infra 读入后注入）。 |

### 三、Application（流程编排与状态调度）

| Skill | 职责 |
|-------|------|
| **GameStateMachine** | 游戏状态机：菜单 / 漫游 / 对话 / 任务 / 日志等状态切换与流转。 |
| **GameMainLoop** | 主循环帧调度：每帧配合 **FrameTimer** 的 **`deltaTime`** 与 FPS 节拍；顺序为 **非阻塞** 输入→逻辑更新→行为推演→触发渲染数据产出（不直接画屏）。 |
| **PlayerMoveFlow** | 玩家移动编排：接按键语义→坐标更新意图→调用 Domain 规则校验→回写领域模型。 |
| **NpcBehaviorFlow** | NPC 行为编排：待机 / 游荡 / 靠近玩家检测等**一帧式**调度顺序。 |
| **DialogFlowCtrl** | 对话业务组装：触发对话→分页展示→选项分支→关联任务挂载（状态与数据流在 App，**不**负责字符绘制）。 |
| **TaskProcessFlow** | 任务全流程：接取→条件监听→进度更新→完成结算→日志归档编排。 |
| **ViewSnapshotAssemble** | **画面快照组装**：聚合玩家、NPC、地图、背景等只读数据，输出 **`RenderSnapshot`**（及与 UI 相关的 **`OverlayModel`** 由本层或并列用例组装，Rep 只消费 DTO）。 |

### 四、Representation（视觉与输入映射）

| Skill | 职责 |
|-------|------|
| **SceneLayerRender** | 分层画面合成：下半可走网格 + 上半远景（布局可对齐「石河伦吾」式上下分区）。合成缓冲以 **`CharPixel`** 栅格为单元（见下节）。 |
| **ParallaxScroll** | 视差滚动渲染：远景慢偏移、地面/可玩带相对滚动；**位移量与 `deltaTime` 结合**，保证平滑连续滚动感（系数可与 Domain 规则侧参数配合，**像素级输出**在此层）。 |
| **KeyMappingTransform** | 键位映射：**KeyboardRawListen 的原始码** → 上/下/左/右/交互/打开日志等 **`GameCommand` 语义**。 |
| **ThemeStyleRender** | 主题色与文艺 UI：黄昏/夜空、对话框样式、封面与文艺文案排版；通过改写 **`CharPixel::color_attr`**（或等价查找表）做**全局换肤**，**不改** `symbol` 所依赖的 **Domain** 语义与规则。 |
| **DialogUIPop** | 对话弹窗渲染：对白面板、分页文案刷新、选项按钮绘制（消费 `OverlayModel`）。 |
| **TravelLogUI** | 旅途日志界面：记录列表、任务履历、文艺结语等**界面侧**渲染更新。 |

### CharPixel（逻辑格「像素」）

- **定义**：一个逻辑控制台格对应 **`CharPixel`**，例如 `struct { char symbol; int color_attr; }`（`color_attr` 与 Win32 属性字、ANSI SGR 索引或内部调色板 id 对齐即可，由 **ConsoleDriver** 最终解释）。
- **用途**：**SceneLayerRender** / **FrameComposer** 在内存中先填满 `CharPixel` 矩阵，再整屏刷新；**ThemeStyleRender** 仅调整 **`color_attr`** 即可切换「黄昏」「冷夜」等主题，**无需修改任何 Domain 逻辑**或领域侧符号决策。

### 分层对照（快速落位）

```text
ConsoleDriver, FrameTimer, KeyboardRawListen, RandomUtil, FilePersistence,
AssetPathConfig, BgmCtrl
  → infrastructure/

Seed, PlayerModelManage, NpcModelManage, TaskModelManage, MapGridRule, NpcSpawnRule,
InteractJudgeRule, SceneGenRule
  → domain/

GameStateMachine, GameMainLoop, PlayerMoveFlow, NpcBehaviorFlow,
DialogFlowCtrl, TaskProcessFlow, ViewSnapshotAssemble
  → application/

SceneLayerRender, ParallaxScroll, KeyMappingTransform, ThemeStyleRender,
DialogUIPop, TravelLogUI
  → representation/
```

**输入链**：`KeyboardRawListen`（Infra，**非阻塞**）→ 原始事件进入循环 → `KeyMappingTransform`（Rep）→ `GameCommand` → Application 各 Flow / FSM。  
**时间链**：`FrameTimer`（Infra）→ **`deltaTime` + FPS 节流** → `GameMainLoop` / 视差与需要积分的用例。  
**输出链**：Domain 更新 → `ViewSnapshotAssemble`（App）→ `RenderSnapshot`/`OverlayModel`（以**符号/逻辑**为主即可）→ Rep 合成 **`CharPixel`** → `SceneLayerRender` / `ParallaxScroll` / `ThemeStyleRender` / `DialogUIPop` / `TravelLogUI` → `ConsoleDriver`（Infra）。

---

## 依赖方向（编译期）

```text
Representation → Application → Domain
Infrastructure → Domain（实现端口，可 include Domain 头）
```

- **禁止** `Representation` 源码 `#include` Domain 路径或直接调用领域实体逻辑；表现层只消费 **Application** 暴露的 DTO（如 `RenderSnapshot`、`OverlayModel`），通过 `GameCommand` 等调用用例。
- **Domain**：无 WinAPI、无具体文件/网络实现；端口仅抽象（如 `IAuditSink`、`IClock`、`IRandom`；终端能力接口若放 Domain 则保持抽象）。**`Seed`** 与 **`IRandom`** 配合保证随机相关规则**可测、可复现**。
- **Application**：状态机与每 tick 编排；**禁止** include 控制台头、禁止直接 `std::cout` 刷屏；审计经注入端口。
- **Infrastructure**：实现端口与 OS/文件/音频/终端；**不含**游戏状态机业务规则。

## 画面与坐标（横版 16:9）

- 用逻辑 **`Cols` × `Rows`** 近似 16:9（例如先定 `Rows`，再 `Cols = round(Rows * 16/9)` 并偶数对齐）；答辩说明为构图比例。
- **上 3/4**（`row < Rows * 3/4`）：仅背景；`ParallaxBackground` 等多层视差，各层独立 **scroll 系数**；**不参与可走碰撞**。
- **下 1/4**（`row >= Rows * 3/4`）：**PlayfieldGrid** 权威碰撞与占用；玩家与 NPC **仅在此带**生成、更新、相邻/交互判定。
- 玩家逻辑格 `(gx, gy)`：**`gy` 限制在下带行范围**；四向中 **`gx` 为主位移**，`gy` 小范围变化。
- **对话/任务 UI**：不占地图格；**Application** 组装 **`OverlayModel`**，**Representation** 的 **FrameComposer** 最后绘制浮层；对话态是否冻结步进由 **Application 状态机** 决定。

## 帧合成顺序

**FrameComposer** 建议顺序：各背景视差层 → 下带路面与实体 →（可选 HUD）→ **Overlay**。（与 **SceneLayerRender**、**ParallaxScroll**、**DialogUIPop** 职责一致：可合并为同一模块内子步骤。）

## 推荐目录与 CMake

- `src/domain/` — 实体、规则、`PlayfieldGrid`、`ParallaxBackground` 生成上带缓冲的规则侧数据（若纯算法无 I/O 可放 Domain）、**端口声明**（含 **`IRandom`**）、**`Seed`** 值对象、扩展接口如 `INpcBehavior` / 任务规则基类。
- `src/application/` — `GameApplication`、状态（封面/漫游/对话/日志等）、用例、**`RenderSnapshot` / `OverlayModel` / `GameCommand`**。
- `src/representation/` — `KeyMappingTransform`、`FrameComposer`（内含 SceneLayerRender / ParallaxScroll 等）、`Theme`；调用 Infra 的 **`ITerminal` / `IConsoleSurface`**（名称以仓库为准）。
- `src/infrastructure/` — `FileAuditSink`、`BgmCtrl`、**`FrameTimer`**、`ConsoleDriver`、**非阻塞** `KeyboardRawListen`、`RandomUtil`、路径与持久化；实现 Domain 声明的端口。
- `src/main.cpp` — 依赖注入组装。

目标：**四库或四 include 根 + 单 exe**，链接关系体现上述依赖；C++17+。

## 审计与 BGM

- 端口在 **Domain**（或全项目统一的一处 **Application** 边界）**仅声明**；**Infrastructure** **实现**。
- **Application** 在用例关键点写审计；Domain 若需记录不变式，可返回事件列表由 App 写出，避免 Domain 依赖文件。
- BGM 初始化在 **main 或 Application 启动** 经 Infra；可选编译开关关闭 BGM 以应对课程限制。

## 初版 MVP 范围

- 可运行主循环；**漫游**：下带四向移动与边界碰撞；上带至少一层视差占位；整屏字符刷新；可选黄昏/冷夜主题；**文件审计必开**；BGM 可选。
- 窗口化/SFML：若需要，仅在 **Representation** 增加适配器，消费同一 **`RenderSnapshot`**，不改 Domain/Application 边界。

## 第二阶段（勿在初版必交中过度展开）

流式场景块、多层视差细化、全量 NPC/对话/四类文艺任务等——见原始计划；改动时避免污染 MVP 边界。

## 实现前自检

- [ ] 新增 `#include` 是否违反 Rep→App→Dom / Inf→Dom？
- [ ] 碰撞与实体逻辑是否只发生在下带网格？
- [ ] 绘制与输入是否留在 Representation，业务状态机是否在 Application？
- [ ] 带 I/O 的代码是否在 Infrastructure？
- [ ] 当前改动对应上表哪一个 **Skill 名**，是否落在正确包内？
- [ ] **键盘路径是否全程非阻塞**？主循环是否被 `getch()`/阻塞 `cin` 拖死？
- [ ] 是否有 **FrameTimer / FPS 节流**，避免 CPU 空转；需要平滑位移处是否传入 **`deltaTime`**？
- [ ] 随机性是否经 **`IRandom` + `Seed`** 建模，避免 Domain 单测飘红？
- [ ] 主题切换是否只动 **`CharPixel::color_attr`**（或等价映射），而不改 Domain 符号语义？

## 原始计划位置（本地）

用户机器上的完整计划：`C:/Users/12073/.cursor/plans/沿途公路信步架构_14bf9d47.plan.md`（含 mermaid 图与风险说明）。若与本 skill 冲突，以**仓库内已落地代码**与**该计划文件**协商为准。
