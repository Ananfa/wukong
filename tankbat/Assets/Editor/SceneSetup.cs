using UnityEngine;

using UnityEngine.EventSystems;

using UnityEngine.UI;

using UnityEditor;

using System.IO;

using TankBattle;
using Vector2 = UnityEngine.Vector2;

public class SceneSetup

{
    // 搭场景时暂存引用，供 SerializedObject 写入 GameManager（原脚本只生成物体未赋值）
    private static GameObject sMainMenuUI;
    private static GameObject sGameUI;
    private static GameObject sGameOverUI;
    private static GameObject sLoadingUI;
    private static Button sSovietButton;
    private static Button sUsaButton;
    private static Button sGermanyButton;
    private static Button sItalyButton;
    private static Button sStartButton;
    private static Button sQuitButton;
    private static Button sRestartButton;
    private static Button sMenuButton;
    private static Text sScoreText;
    private static Text sKillsText;
    private static Text sTimeText;
    private static Text sWinnerText;
    private static Text sGameOverText;
    private static RawImage sMinimapRaw;
    private static RectTransform sMinimapRect;

    [MenuItem("Tools/坦克大战/仅补充 EventSystem（UI 可点击）")]
    public static void AddEventSystemOnly()
    {
        EnsureEventSystemExists();
        if (Object.FindObjectOfType<EventSystem>() != null)
            Debug.Log("已确保场景中存在 EventSystem + StandaloneInputModule（uGUI 点击所需）。");
        UnityEditor.SceneManagement.EditorSceneManager.MarkSceneDirty(
            UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene());
    }

    [MenuItem("Tools/坦克大战/确保 GameManager 音频组件")]
    public static void EnsureGameManagerAudioComponent()
    {
        GameManager gm = Object.FindObjectOfType<GameManager>();
        if (gm == null)
        {
            Debug.LogWarning("SceneSetup: 场景中未找到 GameManager。");
            return;
        }

        if (gm.GetComponent<BattleAudioManager>() == null)
            gm.gameObject.AddComponent<BattleAudioManager>();

        EditorUtility.SetDirty(gm.gameObject);
        Debug.Log("已为 GameManager 确保 BattleAudioManager 组件。请在 GameManager →「音频」里指定 Background Music。");
    }

    [MenuItem("Tools/坦克大战/设置游戏场景")]

    public static void SetupGameScene()

    {

    // 创建新场景

    EditorApplication.NewScene();

    // 运行时 GameManager 会为障碍物设置 tag Obstacle；提前写入 TagManager，避免开始游戏时报错
    ProjectTagUtility.EnsureTagExists("Obstacle");
    ProjectTagUtility.EnsureTagExists("TankBody");

    // 设置游戏管理器
    SetupGameManager();

    // 设置环境
    SetupEnvironment();

    // 设置灯光
    SetupLighting();

    // 设置UI
    SetupUI();

    // 设置相机
    SetupCamera();

    // 小地图相机 RenderTexture → UI RawImage，并把 UI 引用写入 GameManager
    LinkMinimapRenderTextureAndWireGameManager();

    // 保存场景
    SaveScene("GameScene");

    Debug.Log("游戏场景设置完成！（地面与围墙由开局 GenerateMap 生成；GameManager UI 已绑定；坦克预制体需自行指定）");
    }

    private static void SetupGameManager()
    {
    GameObject gameManager = new GameObject("GameManager");

    // 添加组件
    gameManager.AddComponent<GameManager>();
    gameManager.AddComponent<TankBattleClient>();
    gameManager.AddComponent<EffectGenerator>();
    gameManager.AddComponent<BattleAudioManager>();

    // 注意：DontDestroyOnLoad 仅能在运行时（Play）调用，不能写在 Editor 搭场景流程里。
    // 若需要跨场景保留 GameManager，在 GameManager.Awake 里对 this.gameObject 调用即可。
    }

    private static void SetupEnvironment()
    {
    // 对战地面与围墙均在开局 GenerateMap() 中生成（与 C++ 坐标 / worldDisplayScale 一致）。

    Material skyboxMat = AssetDatabase.LoadAssetAtPath<Material>("Assets/Materials/Skybox.mat");
    if (skyboxMat != null)
        RenderSettings.skybox = skyboxMat;
    // 阻挡物：等 GameCore 提供数据后再生成；不在编辑器里随机摆放假障碍。
    }

    private static void SetupLighting()
    {
    // 创建主光源
    GameObject mainLight = new GameObject("Directional Light");
    Light light = mainLight.AddComponent<Light>();
    light.type = LightType.Directional;
    light.transform.rotation = Quaternion.Euler(50, -30, 0);
    light.intensity = 1f;
    light.shadows = LightShadows.Soft;

    // 设置环境光
    RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat;
    RenderSettings.ambientLight = new Color(0.2f, 0.2f, 0.2f);

    // 设置雾
    RenderSettings.fog = true;
    RenderSettings.fogColor = new Color(0.5f, 0.6f, 0.7f, 0.5f);
    RenderSettings.fogDensity = 0.01f;
    }

    private static Font BuiltinUIFont()
    {
        return Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
    }

    private static GameObject CreateFullStretchPanel(string name, Transform parent)
    {
        GameObject go = new GameObject(name);
        go.transform.SetParent(parent, false);
        RectTransform rt = go.AddComponent<RectTransform>();
        rt.anchorMin = Vector2.zero;
        rt.anchorMax = Vector2.one;
        rt.offsetMin = Vector2.zero;
        rt.offsetMax = Vector2.zero;
        return go;
    }

    /// <summary>uGUI 点击依赖 EventSystem + InputModule，否则 Button 不响应。场景搭好后玩家首次运行前必须存在。</summary>
    private static void EnsureEventSystemExists()
    {
        if (Object.FindObjectOfType<EventSystem>() != null)
            return;
        GameObject esGo = new GameObject("EventSystem");
        esGo.AddComponent<EventSystem>();
        esGo.AddComponent<StandaloneInputModule>();
    }

    private static void SetupUI()
    {
    EnsureEventSystemExists();

    GameObject canvasObj = new GameObject("MainCanvas");
    Canvas canvas = canvasObj.AddComponent<Canvas>();
    canvas.renderMode = RenderMode.ScreenSpaceOverlay;

    CanvasScaler scaler = canvasObj.AddComponent<CanvasScaler>();
    scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
    scaler.referenceResolution = new Vector2(1920, 1080);
    scaler.matchWidthOrHeight = 0.5f;

    canvasObj.AddComponent<GraphicRaycaster>();

    // —— 主菜单（与 GameManager.mainMenuUI 对应）——
    sMainMenuUI = CreateFullStretchPanel("MainMenuUI", canvasObj.transform);
    {
        GameObject center = new GameObject("MenuCenter");
        center.transform.SetParent(sMainMenuUI.transform, false);
        RectTransform crt = center.AddComponent<RectTransform>();
        crt.anchorMin = new Vector2(0.5f, 0.5f);
        crt.anchorMax = new Vector2(0.5f, 0.5f);
        crt.pivot = new Vector2(0.5f, 0.5f);
        crt.anchoredPosition = Vector2.zero;
        crt.sizeDelta = new Vector2(400, 480);
        VerticalLayoutGroup v = center.AddComponent<VerticalLayoutGroup>();
        v.childAlignment = TextAnchor.UpperCenter;
        v.spacing = 12;
        v.padding = new RectOffset(20, 20, 20, 20);
        v.childControlHeight = true;
        v.childControlWidth = true;
        v.childForceExpandHeight = false;
        v.childForceExpandWidth = true;

        Text title = CreateUiText(center.transform, "Title", "坦克大战", 36, TextAnchor.MiddleCenter);
        LayoutElement leTitle = title.gameObject.AddComponent<LayoutElement>();
        leTitle.minHeight = 48;

        sSovietButton = CreateMenuButton(center.transform, "SovietButton", "苏联", new Color(0.86f, 0.08f, 0.24f));
        sUsaButton = CreateMenuButton(center.transform, "UsaButton", "美国", new Color(0.12f, 0.56f, 1f));
        sGermanyButton = CreateMenuButton(center.transform, "GermanyButton", "德国", new Color(0.41f, 0.41f, 0.41f));
        sItalyButton = CreateMenuButton(center.transform, "ItalyButton", "意大利", new Color(0f, 0.57f, 0.27f));
        sStartButton = CreateMenuButton(center.transform, "StartButton", "开始游戏", new Color(0.2f, 0.6f, 0.2f));
        sQuitButton = CreateMenuButton(center.transform, "QuitButton", "退出", new Color(0.4f, 0.4f, 0.4f));
    }

    // —— 游戏中 HUD（gameUI）——
    sGameUI = CreateFullStretchPanel("GameUI", canvasObj.transform);
    sGameUI.SetActive(false);
    CreateScorePanel(sGameUI);
    CreateMinimap(sGameUI);
    CreateAbilityButtons(sGameUI);

    // —— 结束界面 ——
    sGameOverUI = CreateFullStretchPanel("GameOverUI", canvasObj.transform);
    sGameOverUI.SetActive(false);
    {
        Image dim = sGameOverUI.AddComponent<Image>();
        dim.color = new Color(0, 0, 0, 0.65f);
        GameObject box = new GameObject("GameOverBox");
        box.transform.SetParent(sGameOverUI.transform, false);
        RectTransform brt = box.AddComponent<RectTransform>();
        brt.anchorMin = new Vector2(0.5f, 0.5f);
        brt.anchorMax = new Vector2(0.5f, 0.5f);
        brt.pivot = new Vector2(0.5f, 0.5f);
        brt.sizeDelta = new Vector2(480, 280);
        VerticalLayoutGroup vl = box.AddComponent<VerticalLayoutGroup>();
        vl.childAlignment = TextAnchor.MiddleCenter;
        vl.spacing = 16;
        vl.padding = new RectOffset(24, 24, 24, 24);
        vl.childControlWidth = true;
        vl.childControlHeight = true;

        sWinnerText = CreateUiText(box.transform, "WinnerText", "胜利者: —", 28, TextAnchor.MiddleCenter);
        sGameOverText = CreateUiText(box.transform, "GameOverText", "游戏结束", 22, TextAnchor.MiddleCenter);
        GameObject btnRow = new GameObject("GameOverButtons");
        btnRow.transform.SetParent(box.transform, false);
        RectTransform rowRt = btnRow.AddComponent<RectTransform>();
        rowRt.sizeDelta = new Vector2(400, 56);
        HorizontalLayoutGroup hl = btnRow.AddComponent<HorizontalLayoutGroup>();
        hl.spacing = 24;
        hl.childAlignment = TextAnchor.MiddleCenter;
        hl.childForceExpandWidth = true;
        sRestartButton = CreateMenuButton(btnRow.transform, "RestartButton", "再来一局", new Color(0.25f, 0.55f, 0.9f));
        sMenuButton = CreateMenuButton(btnRow.transform, "MenuButton", "返回菜单", new Color(0.5f, 0.5f, 0.5f));
    }

    // —— 加载中 ——
    sLoadingUI = CreateFullStretchPanel("LoadingUI", canvasObj.transform);
    sLoadingUI.SetActive(false);
    {
        Image dim = sLoadingUI.AddComponent<Image>();
        dim.color = new Color(0, 0, 0, 0.5f);
        CreateUiText(sLoadingUI.transform, "LoadingText", "加载中…", 32, TextAnchor.MiddleCenter);
    }
    }

    private static Text CreateUiText(Transform parent, string name, string content, int fontSize, TextAnchor align)
    {
        GameObject go = new GameObject(name);
        go.transform.SetParent(parent, false);
        RectTransform rt = go.AddComponent<RectTransform>();
        rt.sizeDelta = new Vector2(400, 40);
        Text t = go.AddComponent<Text>();
        t.text = content;
        t.font = BuiltinUIFont();
        t.fontSize = fontSize;
        t.alignment = align;
        t.color = Color.white;
        return t;
    }

    private static Button CreateMenuButton(Transform parent, string name, string label, Color bg)
    {
        GameObject buttonObj = new GameObject(name);
        buttonObj.transform.SetParent(parent, false);
        RectTransform rt = buttonObj.AddComponent<RectTransform>();
        rt.sizeDelta = new Vector2(320, 48);
        LayoutElement le = buttonObj.AddComponent<LayoutElement>();
        le.minHeight = 48;
        le.preferredHeight = 48;
        Image image = buttonObj.AddComponent<Image>();
        image.color = bg;
        Button button = buttonObj.AddComponent<Button>();
        GameObject textObj = new GameObject("Text");
        textObj.transform.SetParent(buttonObj.transform, false);
        RectTransform textRT = textObj.AddComponent<RectTransform>();
        textRT.anchorMin = Vector2.zero;
        textRT.anchorMax = Vector2.one;
        textRT.sizeDelta = Vector2.zero;
        Text textComp = textObj.AddComponent<Text>();
        textComp.text = label;
        textComp.font = BuiltinUIFont();
        textComp.fontSize = 20;
        textComp.alignment = TextAnchor.MiddleCenter;
        textComp.color = Color.white;
        return button;
    }

    private static void CreateScorePanel(GameObject parent)
    {
    GameObject panel = new GameObject("ScorePanel");
    panel.transform.SetParent(parent.transform, false);

    RectTransform rt = panel.AddComponent<RectTransform>();
    rt.anchorMin = new Vector2(0.5f, 1);
    rt.anchorMax = new Vector2(0.5f, 1);
    rt.pivot = new Vector2(0.5f, 1);
    rt.anchoredPosition = new Vector2(0, -20);
    rt.sizeDelta = new Vector2(420, 120);

    VerticalLayoutGroup v = panel.AddComponent<VerticalLayoutGroup>();
    v.childAlignment = TextAnchor.UpperCenter;
    v.spacing = 4;
    v.padding = new RectOffset(8, 8, 8, 8);
    v.childControlHeight = true;
    v.childControlWidth = true;

    sScoreText = CreateUiText(panel.transform, "ScoreText", "得分: 0", 22, TextAnchor.MiddleCenter);
    sKillsText = CreateUiText(panel.transform, "KillsText", "击杀: 0", 22, TextAnchor.MiddleCenter);
    sTimeText = CreateUiText(panel.transform, "TimeText", "00:00", 22, TextAnchor.MiddleCenter);
    }

    private static void CreateMinimap(GameObject parent)
    {
    GameObject minimap = new GameObject("Minimap");
    minimap.transform.SetParent(parent.transform, false);

    RectTransform rt = minimap.AddComponent<RectTransform>();
    rt.anchorMin = new Vector2(0, 0);
    rt.anchorMax = new Vector2(0, 0);
    rt.pivot = new Vector2(0, 0);
    rt.anchoredPosition = new Vector2(20, 20);
    rt.sizeDelta = new Vector2(150, 150);
    sMinimapRect = rt;

    GameObject background = new GameObject("Background");
    background.transform.SetParent(minimap.transform, false);
    RectTransform bgRT = background.AddComponent<RectTransform>();
    bgRT.anchorMin = Vector2.zero;
    bgRT.anchorMax = Vector2.one;
    bgRT.sizeDelta = Vector2.zero;
    Image bgImage = background.AddComponent<Image>();
    bgImage.color = new Color(0, 0, 0, 0.5f);

    GameObject border = new GameObject("Border");
    border.transform.SetParent(minimap.transform, false);
    RectTransform borderRT = border.AddComponent<RectTransform>();
    borderRT.anchorMin = Vector2.zero;
    borderRT.anchorMax = Vector2.one;
    borderRT.sizeDelta = Vector2.zero;
    Image borderImage = border.AddComponent<Image>();
    borderImage.color = new Color(1, 1, 1, 0.25f);
    borderImage.type = Image.Type.Simple;

    GameObject rawGo = new GameObject("MinimapRaw");
    rawGo.transform.SetParent(minimap.transform, false);
    RectTransform rawRt = rawGo.AddComponent<RectTransform>();
    rawRt.anchorMin = new Vector2(0.05f, 0.05f);
    rawRt.anchorMax = new Vector2(0.95f, 0.95f);
    rawRt.sizeDelta = Vector2.zero;
    sMinimapRaw = rawGo.AddComponent<RawImage>();
    sMinimapRaw.color = Color.white;
    }

    private static void CreateAbilityButtons(GameObject parent)
    {
    GameObject buttonPanel = new GameObject("ButtonPanel");
    buttonPanel.transform.SetParent(parent.transform, false);

    RectTransform rt = buttonPanel.AddComponent<RectTransform>();
    rt.anchorMin = new Vector2(1, 0);
    rt.anchorMax = new Vector2(1, 0);
    rt.pivot = new Vector2(1, 0);
    rt.anchoredPosition = new Vector2(-20, 20);
    rt.sizeDelta = new Vector2(200, 100);

    HorizontalLayoutGroup layout = buttonPanel.AddComponent<HorizontalLayoutGroup>();
    layout.childAlignment = TextAnchor.MiddleRight;
    layout.childControlWidth = true;
    layout.childControlHeight = true;
    layout.spacing = 10;

    CreateHudIconButton(buttonPanel.transform, "FireButton", "开火", Color.red);
    CreateHudIconButton(buttonPanel.transform, "AbilityButton", "技能", Color.blue);
    }

    private static void CreateHudIconButton(Transform parent, string name, string text, Color color)
    {
    GameObject buttonObj = new GameObject(name);
    buttonObj.transform.SetParent(parent, false);
    RectTransform rt = buttonObj.AddComponent<RectTransform>();
    rt.sizeDelta = new Vector2(80, 80);
    Image image = buttonObj.AddComponent<Image>();
    image.color = color;
    buttonObj.AddComponent<Button>();
    GameObject textObj = new GameObject("Text");
    textObj.transform.SetParent(buttonObj.transform, false);
    RectTransform textRT = textObj.AddComponent<RectTransform>();
    textRT.anchorMin = Vector2.zero;
    textRT.anchorMax = Vector2.one;
    textRT.sizeDelta = Vector2.zero;
    Text textComp = textObj.AddComponent<Text>();
    textComp.text = text;
    textComp.font = BuiltinUIFont();
    textComp.fontSize = 16;
    textComp.alignment = TextAnchor.MiddleCenter;
    textComp.color = Color.white;
    }

    private static void LinkMinimapRenderTextureAndWireGameManager()
    {
        Camera mmc = GameObject.Find("Minimap Camera")?.GetComponent<Camera>();
        if (mmc != null && mmc.targetTexture != null && sMinimapRaw != null)
            sMinimapRaw.texture = mmc.targetTexture;

        GameManager gm = GameObject.Find("GameManager")?.GetComponent<GameManager>();
        if (gm == null)
        {
            Debug.LogWarning("SceneSetup: 未找到 GameManager，跳过 UI 引用绑定。");
            return;
        }

        SerializedObject so = new SerializedObject(gm);
        void SetRef(string propName, Object obj)
        {
            SerializedProperty p = so.FindProperty(propName);
            if (p == null && !string.IsNullOrEmpty(propName))
            {
                string mStyle = "m_" + char.ToUpperInvariant(propName[0]) + propName.Substring(1);
                p = so.FindProperty(mStyle);
            }
            if (p == null)
            {
                Debug.LogWarning("SceneSetup: GameManager 上无序列化字段: " + propName);
                return;
            }
            p.objectReferenceValue = obj;
        }

        SetRef("mainMenuUI", sMainMenuUI);
        SetRef("gameUI", sGameUI);
        SetRef("gameOverUI", sGameOverUI);
        SetRef("loadingUI", sLoadingUI);
        SetRef("sovietButton", sSovietButton);
        SetRef("usaButton", sUsaButton);
        SetRef("germanyButton", sGermanyButton);
        SetRef("italyButton", sItalyButton);
        SetRef("startButton", sStartButton);
        SetRef("quitButton", sQuitButton);
        SetRef("restartButton", sRestartButton);
        SetRef("menuButton", sMenuButton);
        SetRef("scoreText", sScoreText);
        SetRef("killsText", sKillsText);
        SetRef("timeText", sTimeText);
        SetRef("winnerText", sWinnerText);
        SetRef("gameOverText", sGameOverText);
        SetRef("minimapImage", sMinimapRaw);
        SetRef("minimapRect", sMinimapRect);
        SetRef("minimapCamera", mmc);

        TryAssignTankPrefabsIfEmpty(so);

        so.ApplyModifiedProperties();
        EditorUtility.SetDirty(gm);
    }

    private static GameObject FindFirstTankPrefabInFolder(string assetFolder)
    {
        if (!AssetDatabase.IsValidFolder(assetFolder))
            return null;
        foreach (string guid in AssetDatabase.FindAssets("t:Prefab", new[] { assetFolder }))
        {
            string path = AssetDatabase.GUIDToAssetPath(guid);
            GameObject go = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (go != null && go.GetComponent<TankController>() != null)
                return go;
        }
        return null;
    }

    /// <summary>若 GameManager 上坦克预制体为空，从 QuickPrefabCreator 默认路径自动绑定第一个含 TankController 的预制体。</summary>
    private static void TryAssignTankPrefabsIfEmpty(SerializedObject gmSo)
    {
        void tryOne(string propName, string folder)
        {
            SerializedProperty p = gmSo.FindProperty(propName);
            if (p == null && !string.IsNullOrEmpty(propName))
            {
                string mStyle = "m_" + char.ToUpperInvariant(propName[0]) + propName.Substring(1);
                p = gmSo.FindProperty(mStyle);
            }
            if (p == null || p.objectReferenceValue != null) return;
            GameObject prefab = FindFirstTankPrefabInFolder(folder);
            if (prefab != null)
            {
                p.objectReferenceValue = prefab;
                Debug.Log("SceneSetup: 自动绑定 " + propName + " -> " + AssetDatabase.GetAssetPath(prefab));
            }
        }

        tryOne("sovietTankPrefab", "Assets/Prefabs/Tanks/Soviet");
        tryOne("usaTankPrefab", "Assets/Prefabs/Tanks/USA");
        tryOne("germanyTankPrefab", "Assets/Prefabs/Tanks/Germany");
        tryOne("italyTankPrefab", "Assets/Prefabs/Tanks/Italy");

        SerializedProperty hb = gmSo.FindProperty("healthBarPrefab");
        if (hb != null && hb.objectReferenceValue == null)
        {
            GameObject hbPrefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Prefabs/UI/HealthBar.prefab");
            if (hbPrefab != null)
            {
                hb.objectReferenceValue = hbPrefab;
                Debug.Log("SceneSetup: 自动绑定 healthBarPrefab");
            }
        }
    }

    private static void SetupCamera()
    {
    // 获取主相机
    Camera mainCamera = Camera.main;
    if (mainCamera == null)
    {
        GameObject cameraObj = new GameObject("Main Camera");
        mainCamera = cameraObj.AddComponent<Camera>();
        cameraObj.AddComponent<AudioListener>();
    }

    mainCamera.transform.position = new Vector3(0, 34, -12);
    mainCamera.transform.rotation = Quaternion.Euler(65, 0, 0);
    mainCamera.fieldOfView = 68;

    // 添加相机控制器
    CameraController cameraController = mainCamera.gameObject.AddComponent<CameraController>();

    // 创建小地图相机
    CreateMinimapCamera();
    }

    private static void CreateMinimapCamera()
    {
    GameObject minimapCameraObj = new GameObject("Minimap Camera");
    Camera minimapCamera = minimapCameraObj.AddComponent<Camera>();

    // 配置小地图相机
    minimapCamera.transform.position = new Vector3(0, 50, 0);
    minimapCamera.transform.rotation = Quaternion.Euler(90, 0, 0);
    minimapCamera.orthographic = true;
    minimapCamera.orthographicSize = 50;
    minimapCamera.clearFlags = CameraClearFlags.SolidColor;
    minimapCamera.backgroundColor = new Color(0, 0, 0, 0);
    minimapCamera.cullingMask = LayerMask.GetMask("Default");
    minimapCamera.depth = 1;

    // 创建渲染纹理
    RenderTexture renderTexture = new RenderTexture(256, 256, 16);
    minimapCamera.targetTexture = renderTexture;
    }

    private static void SaveScene(string sceneName)
    {
    string scenesPath = "Assets/Scenes";
    if (!Directory.Exists(scenesPath))
    {
        Directory.CreateDirectory(scenesPath);
    }

    string scenePath = scenesPath + "/" + sceneName + ".unity";
    UnityEditor.SceneManagement.EditorSceneManager.SaveScene(UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene(), scenePath);

    Debug.Log("场景保存到: " + scenePath);
    }
}