using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>所有坦克血条共用一个 Screen Space Canvas，避免每车一个全屏 Canvas 盖住画面。</summary>
public class HealthBar : MonoBehaviour
{
    private const float BarWidth = 112f;
    private const float BarHeight = 12f;
    private const float TextHeight = 24f;
    private const float TotalHeight = BarHeight + TextHeight + 4f;
    private const float BarPadding = 2f;
    private const int HealthFontSize = 20;
    private const int DamageFontSize = 28;

    [Header("UI组件")]
    [SerializeField] private Image healthFill;
    [SerializeField] private Image shieldFill;
    [SerializeField] private Text healthText;
    [SerializeField] private GameObject criticalIndicator;

    [Header("颜色设置")]
    [SerializeField] private Color highHealthColor = Color.green;
    [SerializeField] private Color mediumHealthColor = Color.yellow;
    [SerializeField] private Color lowHealthColor = Color.red;
    [SerializeField] private Color shieldColor = new Color(0, 0.5f, 1, 0.5f);

    [Header("动画设置")]
    [SerializeField] private float animationSpeed = 10f;
    [SerializeField] private float shakeIntensity = 3f;
    [SerializeField] private float criticalBlinkSpeed = 2f;

    [Header("偏移设置")]
    [SerializeField] private Vector3 worldOffset = new Vector3(0, 3.5f, 0);
    [SerializeField] private Vector2 screenOffset = new Vector2(0, 0);

    private static RectTransform s_sharedCanvasRect;
    private static Font s_defaultFont;

    private static Font DefaultFont
    {
        get
        {
            if (s_defaultFont == null)
                s_defaultFont = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            return s_defaultFont;
        }
    }

    private Transform target;
    private Camera mainCamera;
    private RectTransform rectTransform;
    private RectTransform fillRect;
    private CanvasGroup canvasGroup;
    private float fillMaxWidth;
    private float currentHealth = 1f;
    private float targetHealth = 1f;
    private float currentShield = 0f;
    private float targetShield = 0f;
    private float displayCurrentHp = 100f;
    private float displayMaxHp = 100f;
    private int lastDisplayedCurrent = -1;
    private int lastDisplayedMax = -1;
    private float shakeTimer = 0f;
    private float blinkTimer = 0f;
    private bool isCritical = false;
    private bool visible = true;
    private Vector2 baseAnchoredPosition;

    public static RectTransform SharedCanvasRect
    {
        get
        {
            if (!IsAlive(s_sharedCanvasRect))
                EnsureSharedCanvas();
            return s_sharedCanvasRect;
        }
    }

    public static void ClearSharedCanvas()
    {
        s_sharedCanvasRect = null;
    }

    private static void EnsureSharedCanvas()
    {
        var canvasGo = new GameObject("TankHealthBarsCanvas");
        var canvas = canvasGo.AddComponent<Canvas>();
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        canvas.sortingOrder = 120;

        var scaler = canvasGo.AddComponent<CanvasScaler>();
        scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
        scaler.referenceResolution = new Vector2(1920, 1080);
        scaler.matchWidthOrHeight = 0.5f;

        canvasGo.AddComponent<HealthBarCanvasLifecycle>();
        s_sharedCanvasRect = canvasGo.GetComponent<RectTransform>();
        DontDestroyOnLoad(canvasGo);
    }

    private sealed class HealthBarCanvasLifecycle : MonoBehaviour
    {
        private void OnDestroy()
        {
            ClearSharedCanvas();
        }

        public IEnumerator AnimateDamagePopup(int damage, Transform followTarget, Vector3 offset)
        {
            var go = new GameObject("DamagePopup");
            go.transform.SetParent(transform, false);
            var rt = go.AddComponent<RectTransform>();
            rt.sizeDelta = new Vector2(120f, 36f);

            var text = go.AddComponent<Text>();
            StyleDamageText(text);
            text.text = $"-{damage}";

            const float duration = 0.9f;
            const float floatOffset = TotalHeight * 0.5f + 10f;
            float driftX = Random.Range(-10f, 10f);
            float elapsed = 0f;
            Color startColor = text.color;
            Vector2 anchorPos = Vector2.zero;
            if (IsAlive(followTarget))
                anchorPos = WorldPointToCanvasLocalStatic(followTarget.position + offset);

            while (elapsed < duration)
            {
                if (IsAlive(followTarget))
                    anchorPos = WorldPointToCanvasLocalStatic(followTarget.position + offset);

                elapsed += Time.deltaTime;
                float t = elapsed / duration;
                rt.anchoredPosition = anchorPos
                    + new Vector2(0f, floatOffset)
                    + new Vector2(driftX * t, 48f * t);
                text.color = new Color(startColor.r, startColor.g, startColor.b, 1f - t);
                yield return null;
            }

            if (go != null)
                Destroy(go);
        }
    }

    private static HealthBarCanvasLifecycle GetCanvasLifecycleHost()
    {
        var canvas = SharedCanvasRect;
        if (!IsAlive(canvas))
            return null;

        var host = canvas.GetComponent<HealthBarCanvasLifecycle>();
        if (host == null)
            host = canvas.gameObject.AddComponent<HealthBarCanvasLifecycle>();
        return host;
    }

    private static bool IsAlive(Object obj) => obj != null;

    public static HealthBar CreateForTarget(Transform targetTransform, Vector3 offset)
    {
        if (!IsAlive(targetTransform))
            return null;

        var canvas = SharedCanvasRect;
        if (!IsAlive(canvas))
            return null;

        var barRoot = new GameObject("HealthBar");
        try
        {
            barRoot.transform.SetParent(canvas, false);

            var barRt = barRoot.AddComponent<RectTransform>();
            ApplyBarRootLayout(barRt);

            var group = barRoot.AddComponent<CanvasGroup>();
            CreateBarVisuals(barRoot.transform, out Image fillImage, out RectTransform fillRt, out Text hpText);
            if (!IsAlive(barRoot) || fillImage == null || hpText == null)
            {
                Destroy(barRoot);
                return null;
            }

            var bar = barRoot.AddComponent<HealthBar>();
            bar.Initialize(fillImage, fillRt, null, hpText, group, barRt);
            bar.SetTarget(targetTransform, offset);
            return bar;
        }
        catch (MissingReferenceException)
        {
            if (IsAlive(barRoot))
                Destroy(barRoot);
            return null;
        }
    }

    public void AttachToSharedCanvas(Transform targetTransform, Vector3 offset)
    {
        if (!IsAlive(targetTransform))
            return;

        var canvas = SharedCanvasRect;
        if (!IsAlive(canvas))
            return;

        var strayCanvas = GetComponent<Canvas>();
        if (strayCanvas != null)
        {
            Destroy(strayCanvas);
            var scaler = GetComponent<CanvasScaler>();
            if (scaler != null) Destroy(scaler);
            var raycaster = GetComponent<GraphicRaycaster>();
            if (raycaster != null) Destroy(raycaster);
        }

        transform.SetParent(SharedCanvasRect, false);
        rectTransform = GetComponent<RectTransform>();
        if (rectTransform == null)
            rectTransform = gameObject.AddComponent<RectTransform>();
        ApplyBarRootLayout(rectTransform);

        if (canvasGroup == null)
            canvasGroup = GetComponent<CanvasGroup>() ?? gameObject.AddComponent<CanvasGroup>();

        EnsureBarVisuals();
        SetTarget(targetTransform, offset);
    }

    public static void ApplyBarRootLayout(RectTransform rt)
    {
        rt.anchorMin = new Vector2(0.5f, 0.5f);
        rt.anchorMax = new Vector2(0.5f, 0.5f);
        rt.pivot = new Vector2(0.5f, 0.5f);
        rt.sizeDelta = new Vector2(BarWidth, TotalHeight);
        rt.localScale = Vector3.one;
        rt.anchoredPosition = Vector2.zero;
    }

    private static void CreateBarVisuals(Transform parent, out Image fillImage, out RectTransform fillRt, out Text hpText)
    {
        fillImage = null;
        fillRt = null;
        hpText = null;
        if (!IsAlive(parent))
            return;

        var bgGo = new GameObject("Background");
        bgGo.transform.SetParent(parent, false);
        var bgRt = bgGo.AddComponent<RectTransform>();
        bgRt.anchorMin = new Vector2(0f, 0f);
        bgRt.anchorMax = new Vector2(1f, 0f);
        bgRt.pivot = new Vector2(0.5f, 0f);
        bgRt.sizeDelta = new Vector2(0f, BarHeight);
        bgRt.anchoredPosition = Vector2.zero;
        var bgImage = bgGo.AddComponent<Image>();
        bgImage.color = new Color(0.12f, 0.12f, 0.12f, 0.9f);
        bgImage.raycastTarget = false;

        var fillGo = new GameObject("Fill");
        fillGo.transform.SetParent(bgGo.transform, false);
        fillRt = fillGo.AddComponent<RectTransform>();
        ConfigureFillRect(fillRt);
        fillImage = fillGo.AddComponent<Image>();
        fillImage.color = Color.green;
        fillImage.raycastTarget = false;

        hpText = CreateHealthTextLabel(parent);
    }

    private static Text CreateHealthTextLabel(Transform parent)
    {
        Transform panel = EnsureHealthTextPanel(parent);
        if (!IsAlive(panel))
            return null;

        Transform existing = panel.Find("HealthText");
        if (existing != null)
        {
            var existingText = existing.GetComponent<Text>();
            if (existingText != null)
            {
                StyleHealthText(existingText);
                return existingText;
            }
        }

        var textGo = new GameObject("HealthText");
        textGo.transform.SetParent(panel, false);
        var textRt = textGo.AddComponent<RectTransform>();
        textRt.anchorMin = Vector2.zero;
        textRt.anchorMax = Vector2.one;
        textRt.offsetMin = Vector2.zero;
        textRt.offsetMax = Vector2.zero;
        var text = textGo.AddComponent<Text>();
        StyleHealthText(text);
        text.text = "100/100";
        panel.SetAsLastSibling();
        return text;
    }

    private static Transform EnsureHealthTextPanel(Transform parent)
    {
        if (!IsAlive(parent))
            return null;

        Transform panel = parent.Find("HealthTextPanel");
        if (panel != null)
            return panel;

        var panelGo = new GameObject("HealthTextPanel");
        panelGo.transform.SetParent(parent, false);
        panel = panelGo.transform;
        var panelRt = panelGo.AddComponent<RectTransform>();
        panelRt.anchorMin = new Vector2(0f, 1f);
        panelRt.anchorMax = new Vector2(1f, 1f);
        panelRt.pivot = new Vector2(0.5f, 1f);
        panelRt.sizeDelta = new Vector2(0f, TextHeight);
        panelRt.anchoredPosition = Vector2.zero;
        var panelImage = panelGo.AddComponent<Image>();
        panelImage.color = new Color(0.04f, 0.04f, 0.06f, 0.88f);
        panelImage.raycastTarget = false;
        if (IsAlive(panel))
            panel.SetAsLastSibling();
        return panel;
    }

    private static void ConfigureFillRect(RectTransform fillRt)
    {
        fillRt.anchorMin = new Vector2(0f, 0f);
        fillRt.anchorMax = new Vector2(0f, 1f);
        fillRt.pivot = new Vector2(0f, 0.5f);
        fillRt.anchoredPosition = new Vector2(BarPadding, 0f);
        fillRt.sizeDelta = new Vector2(BarWidth - BarPadding * 2f, -BarPadding * 2f);
    }

    private void EnsureBarVisuals()
    {
        Transform fillTransform = null;
        foreach (Transform t in GetComponentsInChildren<Transform>(true))
        {
            if (t.name == "Fill")
            {
                fillTransform = t;
                break;
            }
        }

        if (fillTransform == null)
        {
            CreateBarVisuals(transform, out Image fillImage, out RectTransform fillRt, out Text hpText);
            if (fillImage == null || hpText == null)
                return;
            Initialize(fillImage, fillRt, shieldFill, hpText, canvasGroup, rectTransform);
            return;
        }

        fillRect = fillTransform.GetComponent<RectTransform>();
        healthFill = fillTransform.GetComponent<Image>();
        if (fillRect == null)
            fillRect = fillTransform.gameObject.AddComponent<RectTransform>();
        if (healthFill == null)
            healthFill = fillTransform.gameObject.AddComponent<Image>();

        ConfigureFillRect(fillRect);
        fillMaxWidth = BarWidth - BarPadding * 2f;
        healthFill.type = Image.Type.Simple;
        healthFill.raycastTarget = false;

        EnsureHealthText();

        Transform textPanel = transform.Find("HealthTextPanel");
        if (textPanel != null)
            textPanel.SetAsLastSibling();

        Transform bgTransform = transform.Find("Background");
        if (bgTransform == null)
        {
            var bgGo = new GameObject("Background");
            bgGo.transform.SetParent(transform, false);
            bgGo.transform.SetAsFirstSibling();
            var bgRt = bgGo.AddComponent<RectTransform>();
            bgRt.anchorMin = new Vector2(0f, 0f);
            bgRt.anchorMax = new Vector2(1f, 0f);
            bgRt.pivot = new Vector2(0.5f, 0f);
            bgRt.sizeDelta = new Vector2(0f, BarHeight);
            bgRt.anchoredPosition = Vector2.zero;
            var bgImage = bgGo.AddComponent<Image>();
            bgImage.color = new Color(0.12f, 0.12f, 0.12f, 0.9f);
            bgImage.raycastTarget = false;

            if (fillTransform.parent != bgGo.transform)
                fillTransform.SetParent(bgGo.transform, false);
        }
        else if (fillTransform.parent != bgTransform)
        {
            fillTransform.SetParent(bgTransform, false);
        }
    }

    private void EnsureHealthText()
    {
        if (healthText == null)
        {
            Transform textTransform = transform.Find("HealthText");
            if (textTransform == null)
            {
                Transform panel = transform.Find("HealthTextPanel");
                if (panel != null)
                    textTransform = panel.Find("HealthText");
            }
            if (textTransform != null)
                healthText = textTransform.GetComponent<Text>();
        }

        Transform textPanel = EnsureHealthTextPanel(transform);
        if (!IsAlive(textPanel))
            return;

        if (healthText == null)
            healthText = CreateHealthTextLabel(transform);
        if (healthText == null)
            return;

        if (healthText.transform.parent != textPanel)
            healthText.transform.SetParent(textPanel, false);

        var textRt = healthText.rectTransform;
        textRt.anchorMin = Vector2.zero;
        textRt.anchorMax = Vector2.one;
        textRt.offsetMin = Vector2.zero;
        textRt.offsetMax = Vector2.zero;
        if (IsAlive(textPanel))
            textPanel.SetAsLastSibling();
        StyleHealthText(healthText);
        RefreshHealthText();
    }

    private static void StyleHealthText(Text text)
    {
        if (text == null) return;
        text.font = DefaultFont;
        text.fontSize = HealthFontSize;
        text.fontStyle = FontStyle.Bold;
        text.alignment = TextAnchor.MiddleCenter;
        text.color = new Color(1f, 0.96f, 0.72f, 1f);
        text.raycastTarget = false;
        ApplyTextOutline(text, new Color(0f, 0f, 0f, 1f), new Vector2(2f, -2f));
    }

    private static void StyleDamageText(Text text)
    {
        if (text == null) return;
        text.font = DefaultFont;
        text.fontSize = DamageFontSize;
        text.fontStyle = FontStyle.Bold;
        text.alignment = TextAnchor.MiddleCenter;
        text.color = new Color(1f, 0.35f, 0.15f, 1f);
        text.raycastTarget = false;
        ApplyTextOutline(text, new Color(0f, 0f, 0f, 0.95f), new Vector2(2f, -2f));
    }

    private static void ApplyTextOutline(Text text, Color outlineColor, Vector2 distance)
    {
        var outline = text.GetComponent<Outline>();
        if (outline == null)
            outline = text.gameObject.AddComponent<Outline>();
        outline.effectColor = outlineColor;
        outline.effectDistance = distance;
    }

    private void RefreshHealthText()
    {
        if (healthText == null) return;
        int current = Mathf.Max(0, Mathf.CeilToInt(displayCurrentHp));
        int max = Mathf.Max(1, Mathf.CeilToInt(displayMaxHp));
        if (current == lastDisplayedCurrent && max == lastDisplayedMax)
            return;
        lastDisplayedCurrent = current;
        lastDisplayedMax = max;
        healthText.text = $"{current}/{max}";
    }

    public void Initialize(
        Image fill,
        RectTransform fillRt,
        Image shield,
        Text text,
        CanvasGroup group,
        RectTransform rt = null)
    {
        healthFill = fill;
        fillRect = fillRt;
        shieldFill = shield;
        healthText = text;
        canvasGroup = group;
        rectTransform = rt != null ? rt : GetComponent<RectTransform>();
        fillMaxWidth = BarWidth - BarPadding * 2f;
        EnsureHealthText();
        ApplyFillWidth(currentHealth);
        RefreshHealthText();
    }

    public void SetTarget(Transform targetTransform, Vector3 offset)
    {
        if (!IsAlive(targetTransform))
        {
            target = null;
            return;
        }
        target = targetTransform;
        worldOffset = offset;
    }

    public Vector3 GetWorldAnchorPosition()
    {
        return target != null ? target.position + worldOffset : Vector3.zero;
    }

    public void SetVisible(bool show)
    {
        visible = show;
        if (canvasGroup != null)
            canvasGroup.alpha = show ? 1f : 0f;
    }

    private void Awake()
    {
        if (rectTransform == null)
            rectTransform = GetComponent<RectTransform>();
        if (canvasGroup == null)
            canvasGroup = GetComponent<CanvasGroup>() ?? gameObject.AddComponent<CanvasGroup>();
        mainCamera = Camera.main;
        if (rectTransform != null)
            ApplyBarRootLayout(rectTransform);
        EnsureBarVisuals();
    }

    private void Start()
    {
        if (criticalIndicator != null)
            criticalIndicator.SetActive(false);
    }

    private void OnDestroy()
    {
        target = null;
    }

    private void LateUpdate()
    {
        if (!IsAlive(target))
        {
            Destroy(gameObject);
            return;
        }

        if (!IsAlive(rectTransform))
            return;

        if (mainCamera == null)
            mainCamera = Camera.main;

        UpdatePosition();
        UpdateHealthDisplay();
        UpdateAnimations();
    }

    private void UpdatePosition()
    {
        if (target == null || mainCamera == null || rectTransform == null) return;

        Vector3 worldPosition = GetWorldAnchorPosition();
        Vector3 viewportPos = mainCamera.WorldToViewportPoint(worldPosition);

        // 仅在目标处于相机后方时隐藏；不要用 screen z 误判侧翼/边缘坦克
        if (viewportPos.z <= 0f)
        {
            if (canvasGroup != null)
                canvasGroup.alpha = 0f;
            return;
        }

        baseAnchoredPosition = WorldPointToCanvasLocal(worldPosition);
        if (shakeTimer <= 0f)
            rectTransform.anchoredPosition = baseAnchoredPosition;

        if (visible && canvasGroup != null)
            canvasGroup.alpha = 1f;
    }

    public void UpdateHealth(float currentHp, float maxHp)
    {
        displayCurrentHp = currentHp;
        displayMaxHp = maxHp > 0f ? maxHp : 1f;
        float healthPercent = displayCurrentHp / displayMaxHp;
        targetHealth = Mathf.Clamp01(healthPercent);
        isCritical = healthPercent < 0.3f;
        RefreshHealthText();
    }

    public void UpdateHealthPercent(float healthPercent)
    {
        UpdateHealth(healthPercent * displayMaxHp, displayMaxHp);
    }

    public void UpdateShield(float shieldPercent)
    {
        targetShield = Mathf.Clamp01(shieldPercent);
    }

    public void ShowDamage(float damageAmount)
    {
        if (damageAmount <= 0.5f || !IsAlive(target))
            return;

        shakeTimer = 0.2f;
        ShowDamagePopup(target, worldOffset, damageAmount);
    }

    public static void ShowDamagePopup(Transform followTarget, Vector3 offset, float damageAmount)
    {
        if (damageAmount <= 0.5f || !IsAlive(followTarget))
            return;

        var host = GetCanvasLifecycleHost();
        if (host == null)
            return;

        host.StartCoroutine(host.AnimateDamagePopup(Mathf.RoundToInt(damageAmount), followTarget, offset));
    }

    private Vector2 WorldPointToCanvasLocal(Vector3 worldPoint)
    {
        return WorldPointToCanvasLocalStatic(worldPoint) + screenOffset;
    }

    private static Vector2 WorldPointToCanvasLocalStatic(Vector3 worldPoint)
    {
        Camera cam = Camera.main;
        if (cam == null)
            return Vector2.zero;

        Vector3 screenPosition = cam.WorldToScreenPoint(worldPoint);
        RectTransformUtility.ScreenPointToLocalPointInRectangle(
            SharedCanvasRect,
            screenPosition,
            null,
            out Vector2 localPoint);
        return localPoint;
    }

    private void ApplyFillWidth(float amount)
    {
        if (fillRect == null) return;
        float w = fillMaxWidth * Mathf.Clamp01(amount);
        fillRect.sizeDelta = new Vector2(w, fillRect.sizeDelta.y);
    }

    private void UpdateHealthDisplay()
    {
        currentHealth = Mathf.Lerp(currentHealth, targetHealth, animationSpeed * Time.deltaTime);
        ApplyFillWidth(currentHealth);

        if (healthFill != null)
        {
            if (currentHealth > 0.6f)
                healthFill.color = highHealthColor;
            else if (currentHealth > 0.3f)
                healthFill.color = mediumHealthColor;
            else
                healthFill.color = lowHealthColor;
        }

        currentShield = Mathf.Lerp(currentShield, targetShield, animationSpeed * Time.deltaTime);
        if (shieldFill != null)
        {
            shieldFill.fillAmount = currentShield;
            shieldFill.color = shieldColor;
        }
    }

    private void UpdateAnimations()
    {
        if (shakeTimer > 0f && rectTransform != null)
        {
            shakeTimer -= Time.deltaTime;
            float shakeX = Random.Range(-shakeIntensity, shakeIntensity);
            float shakeY = Random.Range(-shakeIntensity, shakeIntensity);
            rectTransform.anchoredPosition = baseAnchoredPosition + new Vector2(shakeX, shakeY);
            if (shakeTimer <= 0f)
                rectTransform.anchoredPosition = baseAnchoredPosition;
        }

        if (criticalIndicator != null && isCritical)
        {
            criticalIndicator.SetActive(true);
            blinkTimer += Time.deltaTime * criticalBlinkSpeed;
            var cg = criticalIndicator.GetComponent<CanvasGroup>();
            if (cg != null)
                cg.alpha = (Mathf.Sin(blinkTimer) + 1f) * 0.5f;
        }
        else if (criticalIndicator != null)
        {
            criticalIndicator.SetActive(false);
        }
    }
}
