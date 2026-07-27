using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;
using System.Collections.Generic;
using System.Linq;
using TankBattle;

[RequireComponent(typeof(BattleAudioManager))]
public class GameManager : MonoBehaviour
{
    [Header("UI引用")]
    [SerializeField] private GameObject modeSelectUI;
    [SerializeField] private GameObject loginUI;
    [SerializeField] private GameObject mainMenuUI;
    [SerializeField] private GameObject gameUI;
    [SerializeField] private GameObject gameOverUI;
    [SerializeField] private GameObject loadingUI;
    
    [SerializeField] private Button sovietButton;
    [SerializeField] private Button usaButton;
    [SerializeField] private Button germanyButton;
    [SerializeField] private Button italyButton;
    [SerializeField] private Button startButton;
    [SerializeField] private Button quitButton;
    [SerializeField] private Button restartButton;
    [SerializeField] private Button menuButton;

    [Header("模式选择")]
    [SerializeField] private Button offlineModeButton;
    [SerializeField] private Button onlineModeButton;

    [Header("登录UI")]
    [SerializeField] private InputField accountInput;
    [SerializeField] private Button loginButton;
    [SerializeField] private Button loginBackButton;
    [SerializeField] private Text loginStatusText;
    
    [Header("UI组件")]
    [SerializeField] private Text scoreText;
    [SerializeField] private Text killsText;
    [SerializeField] private Text timeText;
    [SerializeField] private Text winnerText;
    [SerializeField] private Text gameOverText;
    
    [Header("阵营状态UI")]
    [SerializeField] private Transform factionStatusPanel;
    [SerializeField] private GameObject factionStatusPrefab;
    
    [Header("小地图")]
    [SerializeField] private RawImage minimapImage;
    [SerializeField] private RectTransform minimapRect;
    [SerializeField] private Camera minimapCamera;
    [SerializeField] private float minimapSize = 150f;
    
    [Header("坦克预制体")]
    [SerializeField] private GameObject sovietTankPrefab;
    [SerializeField] private GameObject usaTankPrefab;
    [SerializeField] private GameObject germanyTankPrefab;
    [SerializeField] private GameObject italyTankPrefab;
    [SerializeField] private GameObject healthBarPrefab;
    
    [Header("游戏设置")]
    [SerializeField] private float gameDuration = 300f; // 5分钟
    /// <summary>须与 Core/GameCore 中 m_worldWidth/m_worldHeight（默认 1000）及 Tank 坐标系（0..1000 正象限）一致。</summary>
    [SerializeField] private UnityEngine.Vector2 mapSize = new UnityEngine.Vector2(1000f, 1000f);
    [Tooltip("Native 逻辑坐标 → Unity 世界单位。例如 0.22 时战场约 220×220，地图不再占满千米级空间。")]
    [SerializeField] [Range(0.06f, 1f)] private float worldDisplayScale = 0.22f;
    public float WorldDisplayScale => worldDisplayScale;
    [Tooltip("坦克根节点缩放，在预制体/占位体原有尺寸上再放大。")]
    [SerializeField] [Range(0.5f, 6f)] private float tankVisualMultiplier = 2.5f;
    [Tooltip("在 Console 输出 AI 坦克状态（行为、锁定目标、Native/Unity 位置、configSpeed、velocity）；每台坦克最多每 aiLogIntervalSeconds 秒一条。")]
    [SerializeField] private bool logAiTankBehavior = true;
    [SerializeField] private float aiLogIntervalSeconds = 1f;
    [Tooltip("Native 坐标下主网格间距，用于地面花纹（默认 50，约每格 11 Unity 单位 @0.22 缩放）。")]
    [SerializeField] private float groundGridSpacingNative = 50f;
    [Tooltip("地图阻挡墙配置（Native 坐标），须与 C++ GameCore 使用同一份 JSON。")]
    [SerializeField] private TextAsset mapObstaclesConfig;

    [Header("音频")]
    [Tooltip("背景音乐列表：播放完后随机切下一首（尽量不连续重复同一首）。留空则用 2 段占位 BGM。")]
    [SerializeField] private AudioClip[] backgroundMusicTracks;
    [Tooltip("兼容旧场景：若上面列表为空，会使用这一首。")]
    [SerializeField] private AudioClip backgroundMusic;
    [Tooltip("开炮音效。留空则使用程序化占位音。")]
    [SerializeField] private AudioClip gunfireClip;
    [SerializeField] [Range(0f, 1f)] private float bgmVolume = 0.32f;
    [SerializeField] private float bgmCrossfadeSeconds = 1.5f;
    [SerializeField] [Range(0f, 1f)] private float gunfireVolume = 0.55f;
    [SerializeField] [Range(0f, 1f)] private float playerGunfireVolume = 0.75f;
    [SerializeField] private float gunfireMaxDistance = 90f;

    [Header("玩家标识")]
    [Tooltip("本地玩家坦克头顶倒三角距坦克中心的高度（Unity 世界单位，会乘以坦克显示倍率）。")]
    [SerializeField] private float playerTopMarkerHeight = 4.2f;

    private BattleAudioManager battleAudio;
    private EffectGenerator effectGenerator;
    private BattleVfxController battleVfx;
    
    // 引用
    private TankBattleClient tankBattleClient;
    private CameraController cameraController;
    private Dictionary<uint, TankController> tankInstances = new Dictionary<uint, TankController>();
    private Dictionary<uint, BulletVisualEntry> bulletInstances = new Dictionary<uint, BulletVisualEntry>();
    private Dictionary<Faction, FactionStatusUI> factionStatusUIs = new Dictionary<Faction, FactionStatusUI>();
    private readonly Dictionary<uint, float> aiTankLastLogTime = new Dictionary<uint, float>();
    
    // 状态
    private Faction selectedFaction = Faction.Soviet;
    private float gameTime = 0f;
    private uint playerScore = 0;
    private uint playerKills = 0;
    private bool isGameActive = false;
    private bool isGameOver = false;
    private bool battleCameraFramed = false;
    
    // 网络
    private uint playerId = 0;
    private bool loginInProgress;

    /// <summary>战场在 Unity 中的 XZ 尺寸（由 mapSize × worldDisplayScale 得到）。</summary>
    private UnityEngine.Vector2 UnityMapSize => new UnityEngine.Vector2(mapSize.x * worldDisplayScale, mapSize.y * worldDisplayScale);

    private Vector3 NativePlanarToUnity(TankBattle.Vector2 native)
    {
        return new Vector3(native.x * worldDisplayScale, 0f, native.y * worldDisplayScale);
    }

    private AudioClip[] ResolveBackgroundMusicTracks()
    {
        if (backgroundMusicTracks != null && backgroundMusicTracks.Length > 0)
            return backgroundMusicTracks;
        if (backgroundMusic != null)
            return new[] { backgroundMusic };
        return null;
    }

    /// <summary>C++ 平面角（0 沿 +X）→ Unity 绕 Y 的朝向（Native X→Unity X，Y→Z）。</summary>
    private static Quaternion NativePlanarRotationToUnity(float radians)
    {
        float c = Mathf.Cos(radians);
        float s = Mathf.Sin(radians);
        return Quaternion.LookRotation(new Vector3(c, 0f, s), Vector3.up);
    }
    
    private void Awake()
    {
        tankBattleClient = GetComponent<TankBattleClient>();
        if (tankBattleClient == null)
        {
            tankBattleClient = gameObject.AddComponent<TankBattleClient>();
        }
        
        cameraController = Camera.main.GetComponent<CameraController>();
        if (cameraController == null && Camera.main != null)
        {
            cameraController = Camera.main.gameObject.AddComponent<CameraController>();
        }

        if (battleAudio == null)
            battleAudio = GetComponent<BattleAudioManager>();
        if (effectGenerator == null)
            effectGenerator = GetComponent<EffectGenerator>();
        if (battleVfx == null)
            battleVfx = GetComponent<BattleVfxController>();
        if (battleVfx == null)
            battleVfx = gameObject.AddComponent<BattleVfxController>();
        battleVfx.Configure(effectGenerator, worldDisplayScale);
        battleAudio.Configure(
            ResolveBackgroundMusicTracks(),
            gunfireClip,
            bgmVolume,
            gunfireVolume,
            playerGunfireVolume,
            2f,
            gunfireMaxDistance,
            0.07f,
            0.04f,
            1.5f,
            bgmCrossfadeSeconds);
        
        // 初始化；单机/联网由模式选择界面决定
        tankBattleClient.Initialize();
        tankBattleClient.SetUseNetworkTransport(false);
    }
    
    private void Start()
    {
        EnsureItalyMenuButton();
        EnsureModeSelectUI();

        // 绑定UI事件
        if (sovietButton != null) sovietButton.onClick.AddListener(() => SelectFaction(Faction.Soviet));
        if (usaButton != null) usaButton.onClick.AddListener(() => SelectFaction(Faction.USA));
        if (germanyButton != null) germanyButton.onClick.AddListener(() => SelectFaction(Faction.Germany));
        if (italyButton != null) italyButton.onClick.AddListener(() => SelectFaction(Faction.Italy));
        if (startButton != null) startButton.onClick.AddListener(StartGame);
        if (quitButton != null) quitButton.onClick.AddListener(QuitGame);
        if (restartButton != null) restartButton.onClick.AddListener(RestartGame);
        if (menuButton != null) menuButton.onClick.AddListener(ReturnToMenu);
        if (offlineModeButton != null) offlineModeButton.onClick.AddListener(OnSelectOfflineMode);
        if (onlineModeButton != null) onlineModeButton.onClick.AddListener(OnSelectOnlineMode);
        
        Debug.Log("GameManager: ShowModeSelectUI");
        ShowModeSelectUI();
        
        // 初始化小地图
        if (minimapCamera != null)
        {
            minimapCamera.orthographicSize = minimapSize;
        }
        
        // 注册事件
        tankBattleClient.OnPlayerJoined += HandlePlayerJoined;
        tankBattleClient.OnPlayerLeft += HandlePlayerLeft;
        tankBattleClient.OnGameStarted += HandleGameStarted;
        tankBattleClient.OnGameEnded += HandleGameEnded;
        tankBattleClient.OnGameStateChanged += HandleGameStateChanged;
    }
    
    private void OnDestroy()
    {
        // 取消注册事件
        if (tankBattleClient != null)
        {
            tankBattleClient.OnPlayerJoined -= HandlePlayerJoined;
            tankBattleClient.OnPlayerLeft -= HandlePlayerLeft;
            tankBattleClient.OnGameStarted -= HandleGameStarted;
            tankBattleClient.OnGameEnded -= HandleGameEnded;
            tankBattleClient.OnGameStateChanged -= HandleGameStateChanged;
        }
    }
    
    private void Update()
    {
        if (isGameActive && !isGameOver)
        {
            // 更新游戏时间
            gameTime += Time.deltaTime;
            UpdateUITime();
            
            // 检查游戏结束条件
            if (gameTime >= gameDuration)
            {
                EndGame(GameEndReason.TimeOut, ResolveMatchWinner());
            }
            
            // 更新小地图
            UpdateMinimap();
        }
    }

    /// <summary>在 TankBattleClient 推送快照之后，按 C++ 速度外推子弹位置，避免只跟快照时显得卡顿/偏慢。</summary>
    private void LateUpdate()
    {
        if (!isGameActive || isGameOver) return;
        AdvanceBulletVisuals();
    }
    
    private void SelectFaction(Faction faction)
    {
        selectedFaction = faction;
        
        // 更新UI选择状态
        UpdateFactionSelectionUI();
    }

    /// <summary>旧场景未含意大利按钮时，在「德国」与「开始游戏」之间补一个。</summary>
    private void EnsureItalyMenuButton()
    {
        if (italyButton != null)
            return;

        if (mainMenuUI == null)
            return;

        Transform menuCenter = mainMenuUI.transform.Find("MenuCenter");
        if (menuCenter == null)
            menuCenter = mainMenuUI.transform;

        Transform existing = menuCenter.Find("ItalyButton");
        if (existing != null)
        {
            italyButton = existing.GetComponent<Button>();
            return;
        }

        int insertIndex = menuCenter.childCount;
        Transform startButtonTransform = menuCenter.Find("StartButton");
        if (startButtonTransform != null)
            insertIndex = startButtonTransform.GetSiblingIndex();
        else if (germanyButton != null)
            insertIndex = germanyButton.transform.GetSiblingIndex() + 1;

        italyButton = CreateRuntimeMenuButton(
            menuCenter,
            "ItalyButton",
            "意大利",
            new Color(0f, 0.57f, 0.27f),
            insertIndex);
    }

    /// <summary>无场景引用时，在 Canvas 下运行时生成「单机 / 联网」选择面板。</summary>
    private void EnsureModeSelectUI()
    {
        if (modeSelectUI != null && offlineModeButton != null && onlineModeButton != null)
            return;

        Canvas canvas = FindObjectOfType<Canvas>();
        if (canvas == null && mainMenuUI != null)
            canvas = mainMenuUI.GetComponentInParent<Canvas>();
        if (canvas == null)
        {
            Debug.LogError("GameManager: 找不到 Canvas，无法创建模式选择界面");
            return;
        }

        if (modeSelectUI == null)
        {
            modeSelectUI = new GameObject("ModeSelectUI");
            modeSelectUI.transform.SetParent(canvas.transform, false);
            modeSelectUI.transform.SetAsLastSibling();
            RectTransform rootRt = modeSelectUI.AddComponent<RectTransform>();
            StretchFull(rootRt);

            Image bg = modeSelectUI.AddComponent<Image>();
            bg.color = new Color(0.06f, 0.08f, 0.12f, 0.94f);

            GameObject panel = new GameObject("ModePanel");
            panel.transform.SetParent(modeSelectUI.transform, false);
            RectTransform panelRt = panel.AddComponent<RectTransform>();
            panelRt.sizeDelta = new UnityEngine.Vector2(420f, 280f);
            panelRt.anchoredPosition = UnityEngine.Vector2.zero;
            Image panelBg = panel.AddComponent<Image>();
            panelBg.color = new Color(0.16f, 0.18f, 0.22f, 1f);

            VerticalLayoutGroup layout = panel.AddComponent<VerticalLayoutGroup>();
            layout.padding = new RectOffset(28, 28, 28, 28);
            layout.spacing = 16f;
            layout.childAlignment = TextAnchor.MiddleCenter;
            layout.childControlHeight = true;
            layout.childControlWidth = true;
            layout.childForceExpandHeight = false;
            layout.childForceExpandWidth = true;

            CreateLoginLabel(panel.transform, "坦克大战", 32, FontStyle.Bold);
            CreateLoginLabel(panel.transform, "请选择游戏模式", 16, FontStyle.Normal);

            offlineModeButton = CreateRuntimeMenuButton(
                panel.transform, "OfflineModeButton", "单机游戏",
                new Color(0.25f, 0.55f, 0.35f), panel.transform.childCount);
            onlineModeButton = CreateRuntimeMenuButton(
                panel.transform, "OnlineModeButton", "联网对战",
                new Color(0.2f, 0.45f, 0.75f), panel.transform.childCount);
        }
        else
        {
            if (offlineModeButton == null)
            {
                Transform t = modeSelectUI.transform.Find("ModePanel/OfflineModeButton");
                if (t != null) offlineModeButton = t.GetComponent<Button>();
            }
            if (onlineModeButton == null)
            {
                Transform t = modeSelectUI.transform.Find("ModePanel/OnlineModeButton");
                if (t != null) onlineModeButton = t.GetComponent<Button>();
            }
        }
    }

    private void OnSelectOfflineMode()
    {
        if (tankBattleClient != null)
        {
            if (tankBattleClient.IsOnlineLoggedIn || tankBattleClient.IsInBattle)
                tankBattleClient.Logout();
            tankBattleClient.SetUseNetworkTransport(false);
        }

        Debug.Log("GameManager: 选择单机模式");
        ShowMainMenu();
    }

    private void OnSelectOnlineMode()
    {
        if (tankBattleClient == null)
            return;

        tankBattleClient.SetUseNetworkTransport(true);
        EnsureLoginUI();
        BindLoginButtonsIfNeeded();

        if (tankBattleClient.UseDirectKcpBypass || tankBattleClient.IsOnlineLoggedIn)
        {
            Debug.Log("GameManager: 联网（已登录或直连调试）→ 选阵营");
            ShowMainMenu();
            return;
        }

        Debug.Log("GameManager: 选择联网模式 → 登录");
        ShowLoginUI();
    }

    private void BindLoginButtonsIfNeeded()
    {
        if (loginButton != null)
        {
            loginButton.onClick.RemoveListener(OnLoginClicked);
            loginButton.onClick.AddListener(OnLoginClicked);
        }
        if (loginBackButton != null)
        {
            loginBackButton.onClick.RemoveListener(OnLoginBackClicked);
            loginBackButton.onClick.AddListener(OnLoginBackClicked);
        }
    }

    private void OnLoginBackClicked()
    {
        if (loginInProgress)
            return;

        if (tankBattleClient != null)
        {
            tankBattleClient.Logout();
            tankBattleClient.SetUseNetworkTransport(false);
        }
        SetLoginStatus("");
        ShowModeSelectUI();
    }

    /// <summary>无场景引用时，在 Canvas 下运行时生成简单登录面板。</summary>
    private void EnsureLoginUI()
    {
        if (loginUI != null && loginButton != null && accountInput != null)
            return;

        Canvas canvas = FindObjectOfType<Canvas>();
        if (canvas == null && mainMenuUI != null)
            canvas = mainMenuUI.GetComponentInParent<Canvas>();
        if (canvas == null)
        {
            Debug.LogError("GameManager: 找不到 Canvas，无法创建登录界面");
            return;
        }

        if (loginUI == null)
        {
            loginUI = new GameObject("LoginUI");
            loginUI.transform.SetParent(canvas.transform, false);
            loginUI.transform.SetAsLastSibling();
            RectTransform rootRt = loginUI.AddComponent<RectTransform>();
            StretchFull(rootRt);

            Image bg = loginUI.AddComponent<Image>();
            bg.color = new Color(0.08f, 0.1f, 0.14f, 0.92f);

            GameObject panel = new GameObject("LoginPanel");
            panel.transform.SetParent(loginUI.transform, false);
            RectTransform panelRt = panel.AddComponent<RectTransform>();
            panelRt.sizeDelta = new UnityEngine.Vector2(420f, 300f);
            panelRt.anchoredPosition = UnityEngine.Vector2.zero;
            Image panelBg = panel.AddComponent<Image>();
            panelBg.color = new Color(0.16f, 0.18f, 0.22f, 1f);

            VerticalLayoutGroup layout = panel.AddComponent<VerticalLayoutGroup>();
            layout.padding = new RectOffset(24, 24, 24, 24);
            layout.spacing = 14f;
            layout.childAlignment = TextAnchor.MiddleCenter;
            layout.childControlHeight = true;
            layout.childControlWidth = true;
            layout.childForceExpandHeight = false;
            layout.childForceExpandWidth = true;

            CreateLoginLabel(panel.transform, "账号登录", 28, FontStyle.Bold);
            CreateLoginLabel(panel.transform, "输入账号（密码稍后支持）", 16, FontStyle.Normal);

            string defaultAccount = tankBattleClient != null ? tankBattleClient.LoginOpenId : "";
            accountInput = CreateLoginInput(panel.transform, defaultAccount);
            loginButton = CreateRuntimeMenuButton(panel.transform, "LoginButton", "登录", new Color(0.2f, 0.45f, 0.75f), panel.transform.childCount);
            loginBackButton = CreateRuntimeMenuButton(panel.transform, "LoginBackButton", "返回", new Color(0.35f, 0.35f, 0.4f), panel.transform.childCount);
            loginStatusText = CreateLoginLabel(panel.transform, "", 14, FontStyle.Normal);
            loginStatusText.color = new Color(0.85f, 0.75f, 0.4f);
        }
        else
        {
            if (accountInput == null)
                accountInput = loginUI.GetComponentInChildren<InputField>(true);
            if (loginButton == null)
            {
                Transform t = loginUI.transform.Find("LoginPanel/LoginButton");
                if (t != null) loginButton = t.GetComponent<Button>();
            }
            if (loginBackButton == null)
            {
                Transform t = loginUI.transform.Find("LoginPanel/LoginBackButton");
                if (t != null) loginBackButton = t.GetComponent<Button>();
            }
            if (loginStatusText == null)
            {
                Text[] texts = loginUI.GetComponentsInChildren<Text>(true);
                if (texts != null && texts.Length > 0)
                    loginStatusText = texts[texts.Length - 1];
            }
        }
    }

    private static void StretchFull(RectTransform rt)
    {
        rt.anchorMin = UnityEngine.Vector2.zero;
        rt.anchorMax = UnityEngine.Vector2.one;
        rt.offsetMin = UnityEngine.Vector2.zero;
        rt.offsetMax = UnityEngine.Vector2.zero;
    }

    private static Text CreateLoginLabel(Transform parent, string text, int fontSize, FontStyle style)
    {
        GameObject go = new GameObject("Label");
        go.transform.SetParent(parent, false);
        LayoutElement le = go.AddComponent<LayoutElement>();
        le.minHeight = fontSize + 10;
        le.preferredHeight = fontSize + 12;
        Text t = go.AddComponent<Text>();
        t.text = text;
        t.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        t.fontSize = fontSize;
        t.fontStyle = style;
        t.alignment = TextAnchor.MiddleCenter;
        t.color = Color.white;
        return t;
    }

    private static InputField CreateLoginInput(Transform parent, string defaultText)
    {
        GameObject go = new GameObject("AccountInput");
        go.transform.SetParent(parent, false);
        LayoutElement le = go.AddComponent<LayoutElement>();
        le.minHeight = 40f;
        le.preferredHeight = 40f;
        Image img = go.AddComponent<Image>();
        img.color = new Color(0.95f, 0.95f, 0.95f, 1f);

        GameObject textGo = new GameObject("Text");
        textGo.transform.SetParent(go.transform, false);
        RectTransform textRt = textGo.AddComponent<RectTransform>();
        StretchFull(textRt);
        textRt.offsetMin = new UnityEngine.Vector2(10f, 4f);
        textRt.offsetMax = new UnityEngine.Vector2(-10f, -4f);
        Text text = textGo.AddComponent<Text>();
        text.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        text.fontSize = 18;
        text.color = Color.black;
        text.supportRichText = false;
        text.alignment = TextAnchor.MiddleLeft;

        GameObject placeholderGo = new GameObject("Placeholder");
        placeholderGo.transform.SetParent(go.transform, false);
        RectTransform phRt = placeholderGo.AddComponent<RectTransform>();
        StretchFull(phRt);
        phRt.offsetMin = new UnityEngine.Vector2(10f, 4f);
        phRt.offsetMax = new UnityEngine.Vector2(-10f, -4f);
        Text placeholder = placeholderGo.AddComponent<Text>();
        placeholder.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        placeholder.fontSize = 18;
        placeholder.fontStyle = FontStyle.Italic;
        placeholder.color = new Color(0.4f, 0.4f, 0.4f, 0.75f);
        placeholder.text = "账号 / openId";
        placeholder.alignment = TextAnchor.MiddleLeft;

        InputField input = go.AddComponent<InputField>();
        input.textComponent = text;
        input.placeholder = placeholder;
        input.text = defaultText ?? "";
        input.characterLimit = 64;
        return input;
    }

    private async void OnLoginClicked()
    {
        if (loginInProgress || tankBattleClient == null)
            return;

        string account = accountInput != null ? accountInput.text.Trim() : "";
        if (string.IsNullOrEmpty(account))
        {
            SetLoginStatus("请输入账号");
            return;
        }

        loginInProgress = true;
        if (loginButton != null) loginButton.interactable = false;
        SetLoginStatus("登录中...");

        bool ok = await tankBattleClient.LoginAndEnterLobbyAsync(account);
        loginInProgress = false;
        if (loginButton != null) loginButton.interactable = true;

        if (!ok)
        {
            SetLoginStatus("登录失败，请检查服务器与账号");
            return;
        }

        SetLoginStatus("登录成功");
        ShowMainMenu();
    }

    private void SetLoginStatus(string message)
    {
        if (loginStatusText != null)
            loginStatusText.text = message ?? "";
        else if (!string.IsNullOrEmpty(message))
            Debug.Log("Login: " + message);
    }

    private static Button CreateRuntimeMenuButton(
        Transform parent,
        string name,
        string label,
        Color bg,
        int siblingIndex)
    {
        GameObject buttonObj = new GameObject(name);
        buttonObj.transform.SetParent(parent, false);
        buttonObj.transform.SetSiblingIndex(Mathf.Clamp(siblingIndex, 0, parent.childCount - 1));

        RectTransform rt = buttonObj.AddComponent<RectTransform>();
        rt.sizeDelta = new UnityEngine.Vector2(320f, 48f);

        LayoutElement le = buttonObj.AddComponent<LayoutElement>();
        le.minHeight = 48f;
        le.preferredHeight = 48f;

        Image image = buttonObj.AddComponent<Image>();
        image.color = bg;

        Button button = buttonObj.AddComponent<Button>();

        GameObject textObj = new GameObject("Text");
        textObj.transform.SetParent(buttonObj.transform, false);
        RectTransform textRT = textObj.AddComponent<RectTransform>();
        textRT.anchorMin = UnityEngine.Vector2.zero;
        textRT.anchorMax = UnityEngine.Vector2.one;
        textRT.sizeDelta = UnityEngine.Vector2.zero;

        Text textComp = textObj.AddComponent<Text>();
        textComp.text = label;
        textComp.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        textComp.fontSize = 20;
        textComp.alignment = TextAnchor.MiddleCenter;
        textComp.color = Color.white;

        return button;
    }
    
    private void StartGame()
    {
        if (tankBattleClient != null && tankBattleClient.UseNetworkTransport
            && !tankBattleClient.UseDirectKcpBypass
            && !tankBattleClient.IsOnlineLoggedIn)
        {
            SetLoginStatus("请先登录");
            ShowLoginUI();
            return;
        }

        if (playerId == 0)
        {
            // 本地：立即拿到 playerId；在线：Snapshot 后再由 OnPlayerJoined 写入
            playerId = tankBattleClient.ConnectToServer(
                string.IsNullOrEmpty(tankBattleClient.LoginOpenId) ? "Player" : tankBattleClient.LoginOpenId,
                selectedFaction);
            Debug.Log($"创建玩家ID: {playerId}");
            if (playerId == 0 && !tankBattleClient.UseNetworkTransport)
            {
                Debug.LogError("无法连接到服务器");
                return;
            }
        }
        
        ShowLoadingScreen();

        if (mapObstaclesConfig != null)
        {
            if (!tankBattleClient.LoadMapObstacles(mapObstaclesConfig.text))
                Debug.LogWarning("GameManager: MapObstacles.json 加载失败，将使用 C++ 内置默认阻挡墙。");
        }
        else
        {
            Debug.LogWarning("GameManager: 未指定 MapObstacles 配置，将使用 C++ 内置默认阻挡墙。");
        }
        
        // 请求开始游戏（在线：仅开房间→KCP，不再登录）
        tankBattleClient.RequestStartGame();
    }
    
    private void HandlePlayerJoined(PlayerInfo player)
    {
        Debug.Log($"玩家加入: {player.name} ({player.faction})");

        if (playerId == 0 || player.id == playerId)
        {
            playerId = player.id;
            Debug.Log($"您已加入游戏！playerId={playerId}");
        }
    }
    
    private void HandlePlayerLeft(uint leftPlayerId)
    {
        Debug.Log($"玩家离开: {leftPlayerId}");
        
        if (leftPlayerId == playerId && isGameActive)
        {
            Debug.LogWarning("连接已断开");
            EndGame(GameEndReason.PlayerDisconnected, selectedFaction);
        }
    }
    
    private void HandleGameStarted()
    {
        Debug.Log("游戏开始！");
        aiTankLastLogTime.Clear();
        battleVfx?.ResetTracks();
        ClearBulletVisuals();
        
        isGameActive = true;
        isGameOver = false;
        battleCameraFramed = false;
        gameTime = 0f;
        playerScore = 0;
        playerKills = 0;
        
        ShowGameUI();
        
        // 生成地图
        GenerateMap();
        GenerateObstacleWalls(tankBattleClient.GetMapObstacles());
        
        // 设置相机边界与默认俯视视角
        if (cameraController != null)
        {
            cameraController.SetMapBounds(UnityMapSize);
            cameraController.ConfigureBattleView();
        }

        battleAudio?.ClearGunfireHistory();
        battleAudio?.PlayBattleMusic();
    }
    
    private void HandleGameEnded(Faction winner)
    {
        Debug.Log($"游戏结束！胜利者: {winner}");
        
        isGameActive = false;
        isGameOver = true;
        
        EndGame(GameEndReason.FactionWin, winner);
    }
    
    private void HandleGameStateChanged(GameSnapshot snapshot)
    {
        if (!isGameActive) return;
        
        battleVfx?.ProcessSnapshot(snapshot);

        // 更新坦克
        UpdateTanks(snapshot);
        UpdateBullets(snapshot);
        LogAiTankBehaviorPeriodic(snapshot);
        
        // 更新阵营状态
        UpdateFactionStatus(snapshot);
        
        // 更新玩家状态
        UpdatePlayerStatus();
    }
    
    private static bool warnedMissingTankPrefab;

    private void UpdateTanks(GameSnapshot snapshot)
    {
        // 移除不存在的坦克
        List<uint> tanksToRemove = new List<uint>();
        if (snapshot.tanks == null) return;

        foreach (var kvp in tankInstances)
        {
            if (!snapshot.tanks.Any(t => t.id == kvp.Key))
            {
                tanksToRemove.Add(kvp.Key);
            }
        }
        
        foreach (uint tankId in tanksToRemove)
        {
            if (tankInstances.TryGetValue(tankId, out TankController tank))
            {
                Destroy(tank.gameObject);
            }
            tankInstances.Remove(tankId);
        }
        
        // 创建/更新坦克
        foreach (var tankState in snapshot.tanks)
        {
            if (!tankInstances.ContainsKey(tankState.id))
            {
                Vector3 position = NativePlanarToUnity(tankState.position);
                Debug.Log($"new Tank:{tankState.id} x:{tankState.position.x} y:{tankState.position.y}");
                Quaternion rotation = NativePlanarRotationToUnity(tankState.rotation);

                GameObject tankObj = SpawnTankVisual(tankState, position, rotation);
                if (tankObj == null) continue;

                TankController tankController = tankObj.GetComponent<TankController>();
                if (tankController != null && tankObj.activeInHierarchy)
                {
                    tankController.TankId = tankState.id;
                    tankController.PlayerId = tankState.playerId;
                    tankController.Faction = tankState.faction;
                    tankController.TankType = tankState.type;

                    if (tankState.isAlive)
                    {
                        tankController.ConfigureHealthBar(
                            healthBarPrefab,
                            new Vector3(0f, 3.5f * tankVisualMultiplier, 0f));
                    }

                    tankController.UpdateFromNetwork(
                        position, rotation, NativePlanarRotationToUnity(tankState.turretRotation),
                        tankState.hp, tankState.maxHp, tankState.isAlive,
                        tankState.respawnTimeRemaining, tankState.shield,
                        tankState.reloadTimeRemaining, tankState.reloadDuration);

                    tankInstances[tankState.id] = tankController;

                    if (tankState.playerId == playerId && cameraController != null)
                    {
                        if (!battleCameraFramed)
                        {
                            cameraController.AssignBattleTarget(tankObj.transform);
                            cameraController.FramePlayerFactionAtCorner(selectedFaction, worldDisplayScale);
                            battleCameraFramed = true;
                        }
                        else
                        {
                            cameraController.Target = tankObj.transform;
                        }
                    }

                    SyncLocalPlayerTopMarker(tankController, tankState);
                }
            }
            else
            {
                // 更新现有坦克
                TankController tankController = tankInstances[tankState.id];
                if (tankController != null)
                {
                    Vector3 position = NativePlanarToUnity(tankState.position);
                    //Debug.Log($"old Tank:{tankState.id} x:{tankState.position.x} y:{tankState.position.y}");
                    Quaternion rotation = NativePlanarRotationToUnity(tankState.rotation);
                    Quaternion turretRotation = NativePlanarRotationToUnity(tankState.turretRotation);
                    
                    tankController.UpdateFromNetwork(
                        position, rotation, turretRotation,
                        tankState.hp, tankState.maxHp, tankState.isAlive,
                        tankState.respawnTimeRemaining, tankState.shield,
                        tankState.reloadTimeRemaining, tankState.reloadDuration);

                    SyncLocalPlayerTopMarker(tankController, tankState);
                }
            }
        }
    }

    private void SyncLocalPlayerTopMarker(TankController tank, TankState tankState)
    {
        if (tank == null)
            return;

        bool showMarker = tankState.playerId != 0
            && tankState.playerId == playerId
            && tankState.isAlive;

        PlayerTankTopMarker marker = tank.GetComponentInChildren<PlayerTankTopMarker>(true);
        if (!showMarker)
        {
            if (marker != null)
                marker.gameObject.SetActive(false);
            return;
        }

        if (marker == null)
        {
            GameObject markerGo = new GameObject("PlayerTopMarker");
            markerGo.transform.SetParent(tank.transform, false);
            marker = markerGo.AddComponent<PlayerTankTopMarker>();
        }

        marker.gameObject.SetActive(true);
        float worldHeight = playerTopMarkerHeight * tankVisualMultiplier;
        marker.ApplyStyle(TankController.GetFactionColor(tankState.faction), worldHeight);
    }

    private void ClearBulletVisuals()
    {
        foreach (BulletVisualEntry entry in bulletInstances.Values)
        {
            if (entry.GameObject != null)
                Destroy(entry.GameObject);
        }
        bulletInstances.Clear();
    }

    private void AdvanceBulletVisuals()
    {
        float now = Time.time;
        foreach (BulletVisualEntry entry in bulletInstances.Values)
        {
            if (entry.GameObject == null) continue;
            float dt = now - entry.SyncTime;
            if (dt <= 0f) continue;

            entry.GameObject.transform.position = entry.BasePosition + entry.Velocity * dt;
            if (entry.Velocity.sqrMagnitude > 0.0001f)
            {
                entry.GameObject.transform.rotation = Quaternion.LookRotation(
                    entry.Velocity.normalized, Vector3.up);
            }
        }
    }

    /// <summary>根据 C++ 快照同步子弹显示（逻辑与碰撞在 GameCore 中）。</summary>
    private void UpdateBullets(GameSnapshot snapshot)
    {
        if (snapshot.bullets == null)
        {
            ClearBulletVisuals();
            return;
        }

        var aliveIds = new HashSet<uint>();
        foreach (BulletState bullet in snapshot.bullets)
        {
            aliveIds.Add(bullet.id);
            Vector3 pos = NativePlanarToUnity(bullet.position);
            Vector3 velUnity = NativePlanarVelocityToUnity(bullet.velocity);
            Quaternion rot = velUnity.sqrMagnitude > 0.0001f
                ? Quaternion.LookRotation(velUnity.normalized, Vector3.up)
                : Quaternion.identity;

            if (!bulletInstances.TryGetValue(bullet.id, out BulletVisualEntry entry) || entry.GameObject == null)
            {
                Color bulletColor = GetBulletVisualColor(bullet, snapshot);
                GameObject go = CreateBulletVisual(pos, rot, bulletColor);
                entry = new BulletVisualEntry
                {
                    GameObject = go,
                    BasePosition = pos,
                    Velocity = velUnity,
                    SyncTime = Time.time
                };
                bulletInstances[bullet.id] = entry;

                if (tankInstances.TryGetValue(bullet.ownerId, out TankController ownerTank)
                    && ownerTank != null)
                {
                    ownerTank.PlayFireEffects();
                    PlayGunfireSound(ownerTank, snapshot, bullet.ownerId);
                }
                else if (battleAudio != null)
                {
                    PlayGunfireSoundAt(pos, bullet.ownerId, false);
                }
            }
            else
            {
                entry.BasePosition = pos;
                entry.Velocity = velUnity;
                entry.SyncTime = Time.time;
                entry.GameObject.transform.position = pos;
                entry.GameObject.transform.rotation = rot;
            }
        }

        List<uint> stale = null;
        foreach (uint id in bulletInstances.Keys)
        {
            if (!aliveIds.Contains(id))
            {
                if (stale == null) stale = new List<uint>();
                stale.Add(id);
            }
        }
        if (stale != null)
        {
            foreach (uint id in stale)
            {
                if (bulletInstances.TryGetValue(id, out BulletVisualEntry entry) && entry.GameObject != null)
                    Destroy(entry.GameObject);
                bulletInstances.Remove(id);
            }
        }
    }

    private Vector3 NativePlanarVelocityToUnity(TankBattle.Vector2 nativeVelocity)
    {
        return new Vector3(
            nativeVelocity.x * worldDisplayScale,
            0f,
            nativeVelocity.y * worldDisplayScale);
    }

    private void PlayGunfireSound(TankController ownerTank, GameSnapshot snapshot, uint ownerTankId)
    {
        bool isLocalPlayer = ownerTank.PlayerId != 0 && ownerTank.PlayerId == playerId;
        PlayGunfireSoundAt(ownerTank.GetFirePointWorldPosition(), ownerTankId, isLocalPlayer);
    }

    private void PlayGunfireSoundAt(Vector3 worldPosition, uint ownerTankId, bool isLocalPlayer)
    {
        if (battleAudio == null)
            return;
        battleAudio.PlayGunfireAt(worldPosition, ownerTankId, isLocalPlayer);
    }

    private sealed class BulletVisualEntry
    {
        public GameObject GameObject;
        public Vector3 BasePosition;
        public Vector3 Velocity;
        public float SyncTime;
    }

    private Faction ResolveBulletOwnerFaction(BulletState bullet, GameSnapshot snapshot)
    {
        if (tankInstances.TryGetValue(bullet.ownerId, out TankController ownerTank) && ownerTank != null)
            return ownerTank.Faction;

        if (snapshot.tanks != null)
        {
            foreach (TankState tank in snapshot.tanks)
            {
                if (tank.id == bullet.ownerId)
                    return tank.faction;
            }
        }

        return selectedFaction;
    }

    private Color GetBulletVisualColor(BulletState bullet, GameSnapshot snapshot)
    {
        Faction ownerFaction = ResolveBulletOwnerFaction(bullet, snapshot);
        bool isFriendly = ownerFaction == selectedFaction;

        if (isFriendly)
        {
            Color baseColor = TankController.GetFactionColor(ownerFaction);
            return bullet.penetrating
                ? Color.Lerp(baseColor, Color.white, 0.35f)
                : baseColor;
        }

        return bullet.penetrating
            ? new Color(1f, 0.2f, 0.25f)
            : new Color(1f, 0.45f, 0.12f);
    }

    private static GameObject CreateBulletVisual(Vector3 position, Quaternion rotation, Color color)
    {
        GameObject go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        go.name = "Bullet";
        go.transform.position = position;
        go.transform.rotation = rotation;
        go.transform.localScale = Vector3.one * 0.35f;
        var col = go.GetComponent<Collider>();
        if (col != null) Destroy(col);

        Renderer renderer = go.GetComponent<Renderer>();
        if (renderer != null)
            renderer.material.color = color;

        Light light = go.AddComponent<Light>();
        light.type = LightType.Point;
        light.color = color;
        light.range = 2.5f;
        light.intensity = 0.85f;

        TrailRenderer trail = go.AddComponent<TrailRenderer>();
        trail.time = 0.12f;
        trail.startWidth = 0.18f;
        trail.endWidth = 0.02f;
        trail.startColor = color;
        trail.endColor = new Color(color.r, color.g, color.b, 0f);
        trail.material = new Material(Shader.Find("Sprites/Default"));

        return go;
    }
    
    private void LogAiTankBehaviorPeriodic(GameSnapshot snapshot)
    {
        if (!logAiTankBehavior || snapshot.tanks == null) return;

        float interval = Mathf.Max(0.05f, aiLogIntervalSeconds);
        float now = Time.time;
        var aliveAi = new HashSet<uint>();

        foreach (TankState t in snapshot.tanks)
        {
            if (t.playerId != 0 || !t.isAlive) continue;
            aliveAi.Add(t.id);

            if (aiTankLastLogTime.TryGetValue(t.id, out float last) && now - last < interval)
                continue;
            aiTankLastLogTime[t.id] = now;

            string desc;
            switch ((AiMoveMode)t.aiMoveMode)
            {
                case AiMoveMode.WanderNoTarget: desc = "游荡/无锁定目标"; break;
                case AiMoveMode.ApproachTarget: desc = "接近目标"; break;
                case AiMoveMode.RetreatFromTarget: desc = "远离目标"; break;
                case AiMoveMode.StrafeTarget: desc = "侧向移动"; break;
                case AiMoveMode.AvoidObstacle: desc = "避障转向"; break;
                case AiMoveMode.FollowPath: desc = "沿路径移动"; break;
                default: desc = "待机"; break;
            }
            float vlen = Mathf.Sqrt(t.velocity.x * t.velocity.x + t.velocity.y * t.velocity.y);
            Vector3 posU = NativePlanarToUnity(t.position);
            Debug.Log($"[AI] 坦克 {t.id}：{desc}；锁定目标 id={t.lockedTargetId}；frame={snapshot.frame}；" +
                      $"posNative=({t.position.x:F2},{t.position.y:F2})；posUnity=({posU.x:F2},{posU.z:F2})；" +
                      $"configSpeed={t.moveSpeed:F3}；velocity(m_velocity)=({t.velocity.x:F3},{t.velocity.y:F3})；|v|={vlen:F3}");
        }

        List<uint> stale = null;
        foreach (uint id in aiTankLastLogTime.Keys)
        {
            if (!aliveAi.Contains(id))
            {
                if (stale == null) stale = new List<uint>();
                stale.Add(id);
            }
        }
        if (stale != null)
        {
            foreach (uint id in stale)
                aiTankLastLogTime.Remove(id);
        }
    }
    
    private void UpdateFactionStatus(GameSnapshot snapshot)
    {
        // 这里应该从服务器获取阵营状态
        // 简化实现：手动统计
    }
    
    private void UpdatePlayerStatus()
    {
        // 从服务器获取玩家状态
        PlayerInfo playerInfo = tankBattleClient.GetPlayerInfo(playerId);
        if (playerInfo != null)
        {
            playerScore = playerInfo.score;
            playerKills = playerInfo.kills;
            
            UpdateUIScore();
        }
    }
    
    private GameObject GetTankPrefab(Faction faction, TankType type)
    {
        switch (faction)
        {
            case Faction.Soviet:
                return sovietTankPrefab;
            case Faction.USA:
                return usaTankPrefab;
            case Faction.Germany:
                return germanyTankPrefab;
            case Faction.Italy:
                return italyTankPrefab;
            default:
                return sovietTankPrefab;
        }
    }

    /// <summary>有预制体则实例化；否则生成带 TankController 的占位体，保证能看见坦克。</summary>
    private GameObject SpawnTankVisual(TankState tankState, Vector3 position, Quaternion rotation)
    {
        GameObject prefab = GetTankPrefab(tankState.faction, tankState.type);
        if (prefab != null)
        {
            GameObject inst = Instantiate(prefab, position, rotation);
            inst.transform.localScale = inst.transform.localScale * tankVisualMultiplier;
            TankController prefabController = inst.GetComponent<TankController>();
            if (prefabController != null)
            {
                prefabController.Faction = tankState.faction;
                prefabController.PlayerId = tankState.playerId;
            }
            return inst;
        }

        if (!warnedMissingTankPrefab)
        {
            warnedMissingTankPrefab = true;
            Debug.LogWarning("GameManager: 四阵营坦克预制体未配置，使用占位模型。请在 Inspector 指定预制体，或使用 Tools/坦克大战/快速创建预制体 后执行「设置游戏场景」自动绑定。");
        }

        GameObject root = new GameObject("Tank_" + tankState.id);
        root.transform.position = position;
        root.transform.rotation = rotation;
        root.transform.localScale = Vector3.one * tankVisualMultiplier;

        Rigidbody rb = root.AddComponent<Rigidbody>();
        rb.isKinematic = true;
        rb.mass = 1000f;

        BoxCollider box = root.AddComponent<BoxCollider>();
        box.center = new Vector3(0, 0.5f, 0);
        box.size = new Vector3(2, 1, 3);

        GameObject body = GameObject.CreatePrimitive(PrimitiveType.Cube);
        body.name = "Body";
        body.transform.SetParent(root.transform, false);
        body.transform.localPosition = Vector3.zero;
        body.transform.localScale = new Vector3(2, 1, 3);
        TrySetTag(body, "TankBody");
        Destroy(body.GetComponent<Collider>());

        GameObject turret = GameObject.CreatePrimitive(PrimitiveType.Cube);
        turret.name = "Turret";
        turret.transform.SetParent(root.transform, false);
        turret.transform.localPosition = new Vector3(0, 0.6f, 0);
        turret.transform.localScale = new Vector3(1.2f, 0.5f, 1.4f);
        Destroy(turret.GetComponent<Collider>());

        GameObject barrel = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        barrel.name = "Barrel";
        barrel.transform.SetParent(turret.transform, false);
        barrel.transform.localRotation = Quaternion.Euler(90f, 0f, 0f);
        barrel.transform.localPosition = new Vector3(0f, 0f, 0.85f);
        barrel.transform.localScale = new Vector3(0.18f, 0.55f, 0.18f);
        Destroy(barrel.GetComponent<Collider>());

        GameObject firePoint = new GameObject("FirePoint");
        firePoint.transform.SetParent(barrel.transform, false);
        firePoint.transform.localPosition = new Vector3(0f, 0f, 0.5f);

        Color teamCol = TankController.GetFactionColor(tankState.faction);
        foreach (Renderer rend in root.GetComponentsInChildren<Renderer>())
            rend.material.color = teamCol;

        TankController placeholderController = root.AddComponent<TankController>();
        placeholderController.WireRuntimeTurret(turret.transform, barrel.transform, firePoint.transform);
        return root;
    }
    
    /// <summary>未在 Project Settings 中注册的 Tag 赋值会抛 UnityException。</summary>
    private static void TrySetTag(GameObject go, string tagName)
    {
        if (go == null || string.IsNullOrEmpty(tagName)) return;
        try
        {
            go.tag = tagName;
        }
        catch (UnityException)
        {
            Debug.LogWarning($"GameManager: 工程中未定义标签 \"{tagName}\"，物体保持 Untagged。请在 Edit → Project Settings → Tags and Layers 添加，或重新执行 Tools/坦克大战/设置游戏场景。");
        }
    }

    private void GenerateMap()
    {
        if (mapSize.x < 500f || mapSize.y < 500f)
            Debug.LogWarning("GameManager: Native 战场为约 1000×1000，当前 mapSize 过小会导致坦克不在地面上。请在 Inspector 将 Map Size 设为 1000×1000。");

        UnityEngine.Vector2 u = UnityMapSize;
        DestroyBattlefieldEnvironment();

        // 生成地形：Unity 默认 Plane 为 10×10（XZ），覆盖 0..UnityMapSize 的正象限，中心在半幅处
        GameObject terrain = GameObject.CreatePrimitive(PrimitiveType.Plane);
        terrain.name = "BattlefieldGround";
        terrain.transform.position = new Vector3(u.x * 0.5f, 0f, u.y * 0.5f);
        terrain.transform.localScale = new Vector3(u.x / 10f, 1f, u.y / 10f);
        terrain.GetComponent<Renderer>().material = BattlefieldGroundMaterial.Create(
            mapSize.x,
            mapSize.y,
            groundGridSpacingNative);

        GenerateBoundaryWalls(u.x, u.y);
    }

    private void GenerateObstacleWalls(ObstacleWallState[] obstacles)
    {
        if (obstacles == null || obstacles.Length == 0)
            return;

        const float wallHeight = 5f;
        Color wallColor = new Color(0.35f, 0.35f, 0.38f);
        GameObject root = new GameObject("BattlefieldObstacles");

        for (int i = 0; i < obstacles.Length; i++)
        {
            ObstacleWallState obstacle = obstacles[i];
            GameObject wall = GameObject.CreatePrimitive(PrimitiveType.Cube);
            wall.name = $"ObstacleWall_{i}";
            wall.transform.SetParent(root.transform, false);

            Vector3 center = NativePlanarToUnity(new TankBattle.Vector2(obstacle.centerX, obstacle.centerY));
            center.y = wallHeight * 0.5f;
            wall.transform.position = center;
            wall.transform.localScale = new Vector3(
                obstacle.width * worldDisplayScale,
                wallHeight,
                obstacle.height * worldDisplayScale);
            wall.transform.rotation = Quaternion.Euler(0f, -obstacle.rotation * Mathf.Rad2Deg, 0f);
            wall.GetComponent<Renderer>().material.color = wallColor;
            TrySetTag(wall, "Obstacle");
        }
    }

    private void DestroyBattlefieldEnvironment()
    {
        GameObject obstaclesRoot = GameObject.Find("BattlefieldObstacles");
        if (obstaclesRoot != null)
            Destroy(obstaclesRoot);

        GameObject wallsRoot = GameObject.Find("BattlefieldWalls");
        if (wallsRoot != null)
            Destroy(wallsRoot);

        GameObject ground = GameObject.Find("BattlefieldGround");
        if (ground != null)
            Destroy(ground);

        // 旧版 SceneSetup 在原点生成的围墙
        string[] legacyWallNames = { "EastWall", "WestWall", "NorthWall", "SouthWall" };
        foreach (string wallName in legacyWallNames)
        {
            GameObject legacy = GameObject.Find(wallName);
            if (legacy != null)
                Destroy(legacy);
        }
    }

    /// <summary>围住 Native 正象限映射后的 Unity 矩形 [0,width]×[0,depth]（与 BattlefieldGround 一致）。</summary>
    private void GenerateBoundaryWalls(float width, float depth)
    {
        const float wallHeight = 5f;
        const float wallThickness = 1f;
        Color wallColor = new Color(0.1f, 0.1f, 0.1f);

        GameObject root = new GameObject("BattlefieldWalls");

        CreateWallSegment(root.transform, "EastWall",
            new Vector3(width + wallThickness * 0.5f, wallHeight * 0.5f, depth * 0.5f),
            new Vector3(wallThickness, wallHeight, depth), wallColor);
        CreateWallSegment(root.transform, "WestWall",
            new Vector3(-wallThickness * 0.5f, wallHeight * 0.5f, depth * 0.5f),
            new Vector3(wallThickness, wallHeight, depth), wallColor);
        CreateWallSegment(root.transform, "NorthWall",
            new Vector3(width * 0.5f, wallHeight * 0.5f, depth + wallThickness * 0.5f),
            new Vector3(width, wallHeight, wallThickness), wallColor);
        CreateWallSegment(root.transform, "SouthWall",
            new Vector3(width * 0.5f, wallHeight * 0.5f, -wallThickness * 0.5f),
            new Vector3(width, wallHeight, wallThickness), wallColor);
    }

    private static void CreateWallSegment(Transform parent, string name, Vector3 position, Vector3 scale, Color color)
    {
        GameObject wall = GameObject.CreatePrimitive(PrimitiveType.Cube);
        wall.name = name;
        wall.transform.SetParent(parent, false);
        wall.transform.position = position;
        wall.transform.localScale = scale;
        wall.GetComponent<Renderer>().material.color = color;
    }
    
    private void UpdateMinimap()
    {
        if (minimapCamera == null) return;
        
        // 更新小地图位置
        if (cameraController != null && cameraController.Target != null)
        {
            Vector3 targetPos = cameraController.Target.position;
            minimapCamera.transform.position = new Vector3(targetPos.x, minimapCamera.transform.position.y, targetPos.z);
        }
    }
    
    private void UpdateUITime()
    {
        if (timeText != null)
        {
            int minutes = Mathf.FloorToInt(gameTime / 60);
            int seconds = Mathf.FloorToInt(gameTime % 60);
            timeText.text = $"{minutes:00}:{seconds:00}";
        }
    }
    
    private void UpdateUIScore()
    {
        if (scoreText != null) scoreText.text = $"得分: {playerScore}";
        if (killsText != null) killsText.text = $"击杀: {playerKills}";
    }
    
    private void UpdateFactionSelectionUI()
    {
        // 高亮选中的阵营按钮
        // 这里可以添加UI反馈
    }
    
    private Faction ResolveMatchWinner()
    {
        if (tankBattleClient == null)
            return selectedFaction;

        var factionStatus = tankBattleClient.GetFactionStatus();

        Faction winner = selectedFaction;
        uint bestKills = 0;
        uint bestDeaths = uint.MaxValue;
        bool hasCandidate = false;

        foreach (var status in factionStatus)
        {
            if (!hasCandidate
                || status.kills > bestKills
                || (status.kills == bestKills && status.deaths < bestDeaths))
            {
                hasCandidate = true;
                bestKills = status.kills;
                bestDeaths = status.deaths;
                winner = status.faction;
            }
        }

        return winner;
    }

    private void EndGame(GameEndReason reason, Faction winner)
    {
        isGameActive = false;
        isGameOver = true;

        battleAudio?.StopBattleMusic();
        
        ShowGameOverUI();
        
        // 设置胜利者文本
        if (winnerText != null)
        {
            string winnerName = GetFactionName(winner);
            FactionStatus winnerStats = null;
            if (tankBattleClient != null)
            {
                foreach (var status in tankBattleClient.GetFactionStatus())
                {
                    if (status.faction == winner)
                    {
                        winnerStats = status;
                        break;
                    }
                }
            }

            if (winnerStats != null)
                winnerText.text = $"胜利者: {winnerName}（击杀 {winnerStats.kills} / 阵亡 {winnerStats.deaths}）";
            else
                winnerText.text = $"胜利者: {winnerName}";
        }
        
        // 设置游戏结束文本
        if (gameOverText != null)
        {
            string reasonText = GetGameEndReasonText(reason);
            gameOverText.text = reasonText;
        }
    }
    
    private string GetFactionName(Faction faction)
    {
        switch (faction)
        {
            case Faction.Soviet: return "苏联";
            case Faction.USA: return "美国";
            case Faction.Germany: return "德国";
            case Faction.Italy: return "意大利";
            default: return "未知";
        }
    }
    
    private string GetGameEndReasonText(GameEndReason reason)
    {
        switch (reason)
        {
            case GameEndReason.FactionWin: return "一方阵营获得胜利";
            case GameEndReason.TimeOut: return "时间到，按击杀数与阵亡数结算";
            case GameEndReason.PlayerDisconnected: return "玩家断开连接";
            default: return "游戏结束";
        }
    }
    
    // UI控制
    private void HideAllScreens()
    {
        SetUIState(modeSelectUI, false);
        SetUIState(loginUI, false);
        SetUIState(mainMenuUI, false);
        SetUIState(gameUI, false);
        SetUIState(gameOverUI, false);
        SetUIState(loadingUI, false);
    }

    private void ShowModeSelectUI()
    {
        if (modeSelectUI == null)
            EnsureModeSelectUI();
        if (modeSelectUI == null)
            Debug.LogError("GameManager: modeSelectUI 为空，无法显示模式选择。请确认场景有 Canvas。");

        HideAllScreens();
        SetUIState(modeSelectUI, true);
    }

    private void ShowLoginUI()
    {
        if (loginUI == null)
            EnsureLoginUI();
        if (loginUI == null)
        {
            Debug.LogError(
                "GameManager: loginUI 为空，无法显示登录界面。请确认场景有 Canvas，" +
                "或在 GameManager 上手动指定 Login UI。");
        }
        HideAllScreens();
        SetUIState(loginUI, true);
    }

    private void ShowMainMenu()
    {
        HideAllScreens();
        SetUIState(mainMenuUI, true);
    }
    
    private void ShowGameUI()
    {
        HideAllScreens();
        SetUIState(gameUI, true);
    }
    
    private void ShowGameOverUI()
    {
        HideAllScreens();
        SetUIState(gameOverUI, true);
    }
    
    private void ShowLoadingScreen()
    {
        HideAllScreens();
        SetUIState(loadingUI, true);
    }
    
    private void SetUIState(GameObject uiObject, bool active)
    {
        if (uiObject != null)
        {
            uiObject.SetActive(active);
        }
    }

    private void ClearBattlePresentation()
    {
        ClearBulletVisuals();
        foreach (var kvp in tankInstances)
        {
            if (kvp.Value != null)
                Destroy(kvp.Value.gameObject);
        }
        tankInstances.Clear();
        DestroyBattlefieldEnvironment();
        battleVfx?.ResetTracks();
        battleAudio?.StopBattleMusic();
    }
    
    // 按钮事件
    private void RestartGame()
    {
        // 在线：退房间回到选阵营，不重载场景以免丢掉登录态
        if (tankBattleClient != null && tankBattleClient.UseNetworkTransport)
        {
            ReturnToMenu();
            return;
        }
        SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex);
    }
    
    private void ReturnToMenu()
    {
        if (tankBattleClient != null)
        {
            if (tankBattleClient.UseNetworkTransport)
                tankBattleClient.LeaveBattle(); // 只退战斗房间，保持 Lobby
            else
                tankBattleClient.Disconnect(notifyPlayerLeft: false);
        }

        ClearBattlePresentation();
        playerId = 0;
        isGameActive = false;
        isGameOver = false;
        battleCameraFramed = false;

        // 战斗结束回到选阵营；会话丢失则回登录；单机也回选阵营
        if (tankBattleClient != null && tankBattleClient.UseNetworkTransport
            && !tankBattleClient.UseDirectKcpBypass
            && !tankBattleClient.IsOnlineLoggedIn)
            ShowLoginUI();
        else
            ShowMainMenu();
    }
    
    private void QuitGame()
    {
        if (tankBattleClient != null && tankBattleClient.UseNetworkTransport)
            tankBattleClient.Logout();

        Application.Quit();
        
        #if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false;
        #endif
    }
    
    // 阵营状态UI类
    private class FactionStatusUI
    {
        public GameObject uiObject;
        public Text factionNameText;
        public Text aliveCountText;
        public Image progressBar;
    }
}

public enum GameEndReason
{
    FactionWin,
    TimeOut,
    PlayerDisconnected
}