=== 一、在 Unity Hub 中创建可运行工程（推荐） ===
1. 安装 Unity（建议 2021.3 LTS 或 2022.3 LTS，Windows）。
2. Unity Hub → 新建项目 → 模板选「3D (Built-in Render Pipeline)」（与脚本里 Standard/内置 UI 一致；勿选仅 URP 且未改管线时易缺材质表现）。
3. 项目创建后，将本目录下的整个 Assets 文件夹内容**合并**进新项目的 Assets：
   至少包含：Scripts、Editor、Config、Plugins（及 Plugins 内 TankBattleNative.dll）。
4. 若本仓库里已有编译好的 build_c\bin\Release\TankbattleNative.dll，复制到 新工程\Assets\Plugins\；
   否则先按「三、编译原生插件」用 CMake 生成 DLL 后再复制（Player Settings 中通常保持 Any CPU，由 Unity 在 Windows 上加载 x64 原生插件）。
5. 用 Unity 打开该项目，等待脚本编译完成。
6. 再按「二、资源与工具顺序」在编辑器里用菜单生成材质/场景/预制体并连线。
7. File → Build Settings，将要玩的场景（如 Scenes/GameScene）加入列表并设为启动；点 Play 运行；选阵营、开始游戏；操作见「四、操作说明」。

若不用菜单自动搭场：新建空场景，创建空物体挂 GameManager、TankBattleClient；主相机挂 CameraController；
在 GameManager 上拖入三阵营坦克预制体（需自行按 PrefabCreationGuide 或「二」中快速创建再赋值），保存场景并设为启动场景。

注意：tankbat 在仓库中通常只有「Assets 资源树」，没有随仓库提交的 ProjectSettings/Package 锁文件，
因此必须以「Hub 新建工程再拷入 Assets」为主流程，不能单靠复制 tankbat 根目录当完整 Unity 工程。

=== 二、资源与工具顺序、产出物与可玩条件 ===
【必须已有】编译产物 TankBattleNative.dll
  - 流程：见下文「三、编译原生插件」；产出物为 Unity 工程 Assets\Plugins\TankBattleNative.dll。
  - 作用：TankBattleClient 通过 P/Invoke 调用 C++ 做战斗与物理模拟；无 DLL 则无法正常游戏逻辑。
  - 若仓库或 build_c 里已有现成 Release DLL，可直接拷入 Plugins，不必每次重编。

【建议按顺序执行的 Unity 菜单】（顶部 Tools → 坦克大战 → …）
1）创建游戏材质
  - 对应脚本：Editor 中 MaterialCreator。
  - 主要产出：Assets/Materials/ 下 Ground、Rock、各阵营与特效等 .mat。
  - 供「设置游戏场景」中地面/障碍引用；不执行也可运行（SceneSetup 对缺材质有颜色回退，但效果较差）。

2）设置游戏场景
  - 对应脚本：Editor 中 SceneSetup。
  - 主要产出：Assets/Scenes/GameScene.unity（或另存为当前场景），含 GameManager、TankBattleClient、EffectGenerator、基础地形、灯光、Canvas、相机等骨架。
  - 将上述场景加入 Build Settings；GameManager 上部分 UI/按钮需按场景在 Inspector 中再连线（视插件生成结果而定）。

3）快速创建预制体（可选，但想「看见并操控坦克」时强烈建议）
  - 对应脚本：Editor 中 QuickPrefabCreator。
  - 主要产出：通常位于 Assets/Prefabs/ 下（坦克、子弹、爆炸、血条等，以脚本内保存路径为准）。
  - 重要：在 GameManager 上配置「苏联/美国/德国」三个坦克预制体（sovietTankPrefab / usaTankPrefab / germanyTankPrefab）。
  - 执行「设置游戏场景」时，会尝试从 Assets/Prefabs/Tanks/Soviet|USA|Germany 自动绑定第一个带 TankController 的预制体（需已用快速创建预制体生成到该路径）。
  - 若仍为空：运行时会用简单方块占位并打一次 Console 警告，仍能看见坦克；正式效果请拖入预制体。

【不必为能运行而准备】
  - Assets/Config/TankConfig.json：当前主流程不依赖其才能进 Play。
  - 网络与 wukong 服务器：本地模式不需要。

【可玩性小结】
  - 仅 DLL + 有 GameManager 与 TankBattleClient 的场景：逻辑可跑，坦克预制体未赋值则无模型。
  - 能正常看见车、能操作：DLL + 场景 + 三阵营坦克预制体已在 GameManager 中赋值 +（建议）已执行材质与场景菜单。

=== 三、编译原生插件 TankBattleNative.dll（Windows / VS） ===
启动 Developer PowerShell for VS
到 F:\linux_dev\wukong\tankbat\build_c 路径下
运行 cmake --build . --config Release
将 F:\linux_dev\wukong\tankbat\build_c\bin\Release 中的 TankbattleNative.dll 拷贝到 Unity 工程的 Assets\Plugins 下
若文件被占用需先关闭 Unity

Unity 脚本入口：Assets\Scripts\TankBattleClient.cs（与 GameManager 配合，本地模式用键盘驱动 Native）

=== 四、操作说明（游戏中） ===
  移动：WASD 或方向键
  瞄准：鼠标指向地面，或 I/J/K/L
  开火：鼠标左键 / 左 Ctrl / F
  技能：Space / 左 Shift

=== 五、遇到问题 ===
  UI 能显示但按钮点不了：场景里需要 EventSystem。重新执行「设置游戏场景」会自动创建；或菜单 Tools/坦克大战/仅补充 EventSystem（UI 可点击）。
  点开始游戏报 Tag Obstacle 未定义：菜单 Tools/坦克大战/确保 Obstacle 与 TankBody 标签；或重新执行「设置游戏场景」（会自动写入 TagManager）。运行时 GameManager 也会在未定义时跳过设 tag 并打警告，不再崩溃。
C++ 死锁会导致 Unity 崩溃
