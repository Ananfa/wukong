using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using System.IO;
using TankBattle;
using Vector2 = UnityEngine.Vector2;

public class QuickPrefabCreator : EditorWindow
{
    private string prefabName = "Tank";
    private Faction faction = Faction.Soviet;
    private TankType tankType = TankType.T34;
    
    [MenuItem("Tools/坦克大战/快速创建预制体")]
    public static void ShowWindow()
    {
        GetWindow<QuickPrefabCreator>("预制体创建器");
    }
    
    private void OnGUI()
    {
        GUILayout.Label("坦克预制体创建", EditorStyles.boldLabel);
        
        prefabName = EditorGUILayout.TextField("预制体名称", prefabName);
        faction = (Faction)EditorGUILayout.EnumPopup("阵营", faction);
        tankType = (TankType)EditorGUILayout.EnumPopup("坦克类型", tankType);
        
        GUILayout.Space(20);
        
        if (GUILayout.Button("创建坦克预制体", GUILayout.Height(40)))
        {
            CreateTankPrefab();
        }
        
        GUILayout.Space(10);
        
        if (GUILayout.Button("创建子弹预制体"))
        {
            CreateBulletPrefab();
        }
        
        if (GUILayout.Button("创建爆炸特效"))
        {
            CreateExplosionEffect();
        }
        
        if (GUILayout.Button("创建血条UI"))
        {
            CreateHealthBar();
        }
    }
    
    private void CreateTankPrefab()
    {
        ProjectTagUtility.EnsureTagExists("TankBody");

        // 创建基本结构
        GameObject tank = new GameObject(prefabName);
        
        // 添加车身
        GameObject body = GameObject.CreatePrimitive(PrimitiveType.Cube);
        body.name = "Body";
        body.transform.SetParent(tank.transform);
        body.transform.localPosition = Vector3.zero;
        body.transform.localScale = new Vector3(2, 1, 3);
        body.tag = "TankBody";
        
        // 添加炮塔
        GameObject turret = GameObject.CreatePrimitive(PrimitiveType.Cube);
        turret.name = "Turret";
        turret.transform.SetParent(tank.transform);
        turret.transform.localPosition = new Vector3(0, 0.5f, 0);
        turret.transform.localScale = new Vector3(1.5f, 0.8f, 1.5f);
        
        // 炮管：Unity 默认 Cylinder 沿 Y 轴，需绕 X 转 90° 使炮口朝 +Z（车身前方）
        GameObject barrel = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        barrel.name = "Barrel";
        barrel.transform.SetParent(turret.transform);
        barrel.transform.localRotation = Quaternion.Euler(90f, 0f, 0f);
        barrel.transform.localPosition = new Vector3(0f, 0f, 1.2f);
        barrel.transform.localScale = new Vector3(0.2f, 0.8f, 0.2f);
        
        // 添加履带
        GameObject leftTrack = GameObject.CreatePrimitive(PrimitiveType.Cube);
        leftTrack.name = "LeftTrack";
        leftTrack.transform.SetParent(tank.transform);
        leftTrack.transform.localPosition = new Vector3(-1.2f, -0.5f, 0);
        leftTrack.transform.localScale = new Vector3(0.2f, 0.3f, 3.2f);
        
        GameObject rightTrack = GameObject.CreatePrimitive(PrimitiveType.Cube);
        rightTrack.name = "RightTrack";
        rightTrack.transform.SetParent(tank.transform);
        rightTrack.transform.localPosition = new Vector3(1.2f, -0.5f, 0);
        rightTrack.transform.localScale = new Vector3(0.2f, 0.3f, 3.2f);
        
        // 添加组件
        Rigidbody rb = tank.AddComponent<Rigidbody>();
        rb.mass = 1000;
        rb.drag = 5;
        rb.angularDrag = 5;
        rb.isKinematic = true; // 网络同步使用
        
        BoxCollider collider = tank.AddComponent<BoxCollider>();
        collider.center = new Vector3(0, 0.5f, 0);
        collider.size = new Vector3(2, 1, 3);
        
        GameObject firePoint = new GameObject("FirePoint");
        firePoint.transform.SetParent(barrel.transform);
        firePoint.transform.localPosition = new Vector3(0f, 0f, 0.8f);
        
        TankController tankController = tank.AddComponent<TankController>();
        tankController.Faction = faction;
        tankController.TankType = tankType;
        SerializedObject so = new SerializedObject(tankController);
        so.FindProperty("turretTransform").objectReferenceValue = turret.transform;
        so.FindProperty("barrelTransform").objectReferenceValue = barrel.transform;
        so.FindProperty("firePoint").objectReferenceValue = firePoint.transform;
        so.ApplyModifiedPropertiesWithoutUndo();
        
        // 创建材质
        Material tankMaterial = new Material(Shader.Find("Standard"));
        tankMaterial.color = GetFactionColor(faction);
        
        // 应用材质
        Renderer[] renderers = tank.GetComponentsInChildren<Renderer>();
        foreach (Renderer renderer in renderers)
        {
            renderer.material = tankMaterial;
        }
        
        // 保存为预制体
        string folderPath = "Assets/Prefabs/Tanks/" + faction;
        if (!Directory.Exists(folderPath))
        {
            Directory.CreateDirectory(folderPath);
        }
        
        string prefabPath = folderPath + "/" + prefabName + ".prefab";
        PrefabUtility.SaveAsPrefabAsset(tank, prefabPath);
        
        DestroyImmediate(tank);
        
        Debug.Log("坦克预制体创建成功: " + prefabPath);
    }
    
    private void CreateBulletPrefab()
    {
        GameObject bullet = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        bullet.name = "Bullet";
        bullet.transform.localScale = Vector3.one * 0.5f;
        
        // 添加轨迹效果
        TrailRenderer trail = bullet.AddComponent<TrailRenderer>();
        trail.time = 0.2f;
        trail.startWidth = 0.2f;
        trail.endWidth = 0.05f;
        trail.material = new Material(Shader.Find("Particles/Standard Unlit"));
        trail.startColor = Color.yellow;
        trail.endColor = Color.red;
        
        // 添加光源
        Light light = bullet.AddComponent<Light>();
        light.type = LightType.Point;
        light.range = 2;
        light.intensity = 2;
        light.color = Color.yellow;
        
        // 添加脚本
        bullet.AddComponent<BulletController>();
        
        // 添加音效
        AudioSource audio = bullet.AddComponent<AudioSource>();
        audio.playOnAwake = false;
        audio.spatialBlend = 1;
        
        // 保存预制体
        string path = "Assets/Prefabs/Projectiles/Bullet.prefab";
        PrefabUtility.SaveAsPrefabAsset(bullet, path);
        
        DestroyImmediate(bullet);
        
        Debug.Log("子弹预制体创建成功: " + path);
    }
    
    private void CreateExplosionEffect()
    {
        GameObject explosion = new GameObject("Explosion");
        
        // 添加粒子系统
        ParticleSystem ps = explosion.AddComponent<ParticleSystem>();
        
        // 配置粒子系统
        var main = ps.main;
        main.duration = 1f;
        main.loop = false;
        main.startLifetime = 0.5f;
        main.startSpeed = 10f;
        main.startSize = 1f;
        main.startColor = new Color(1f, 0.5f, 0f);
        
        var emission = ps.emission;
        emission.rateOverTime = 0;
        emission.SetBursts(new[] { new ParticleSystem.Burst(0f, 50) });
        
        var shape = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Sphere;
        
        var velocity = ps.velocityOverLifetime;
        velocity.enabled = true;
        
        // 添加音频
        AudioSource audio = explosion.AddComponent<AudioSource>();
        audio.playOnAwake = true;
        audio.spatialBlend = 1;
        
        // 添加自动销毁（定义见 EffectGenerator 内嵌套类）
        explosion.AddComponent<EffectGenerator.AutoDestroy>().lifetime = 2f;
        
        // 保存预制体
        string path = "Assets/Prefabs/Effects/Explosion.prefab";
        PrefabUtility.SaveAsPrefabAsset(explosion, path);
        
        Debug.Log("爆炸特效预制体创建成功: " + path);
    }
    
    private void CreateHealthBar()
    {
        GameObject healthBar = new GameObject("HealthBar");
        RectTransform barRt = healthBar.AddComponent<RectTransform>();
        HealthBar.ApplyBarRootLayout(barRt);

        CanvasGroup canvasGroup = healthBar.AddComponent<CanvasGroup>();

        GameObject background = new GameObject("Background");
        background.transform.SetParent(healthBar.transform, false);
        RectTransform bgRT = background.AddComponent<RectTransform>();
        bgRT.anchorMin = new Vector2(0f, 0f);
        bgRT.anchorMax = new Vector2(1f, 0f);
        bgRT.pivot = new Vector2(0.5f, 0f);
        bgRT.sizeDelta = new Vector2(0f, 12f);
        bgRT.anchoredPosition = Vector2.zero;
        Image bgImage = background.AddComponent<Image>();
        bgImage.color = new Color(0.12f, 0.12f, 0.12f, 0.9f);
        bgImage.raycastTarget = false;

        GameObject fill = new GameObject("Fill");
        fill.transform.SetParent(background.transform, false);
        RectTransform fillRT = fill.AddComponent<RectTransform>();
        fillRT.anchorMin = new Vector2(0f, 0f);
        fillRT.anchorMax = new Vector2(0f, 1f);
        fillRT.pivot = new Vector2(0f, 0.5f);
        fillRT.anchoredPosition = new Vector2(1f, 0f);
        fillRT.sizeDelta = new Vector2(108f, -4f);
        Image fillImage = fill.AddComponent<Image>();
        fillImage.color = Color.green;
        fillImage.raycastTarget = false;

        GameObject healthTextPanel = new GameObject("HealthTextPanel");
        healthTextPanel.transform.SetParent(healthBar.transform, false);
        RectTransform panelRT = healthTextPanel.AddComponent<RectTransform>();
        panelRT.anchorMin = new Vector2(0f, 1f);
        panelRT.anchorMax = new Vector2(1f, 1f);
        panelRT.pivot = new Vector2(0.5f, 1f);
        panelRT.sizeDelta = new Vector2(0f, 24f);
        panelRT.anchoredPosition = Vector2.zero;
        Image panelImage = healthTextPanel.AddComponent<Image>();
        panelImage.color = new Color(0.04f, 0.04f, 0.06f, 0.88f);
        panelImage.raycastTarget = false;

        GameObject healthTextGo = new GameObject("HealthText");
        healthTextGo.transform.SetParent(healthTextPanel.transform, false);
        RectTransform textRT = healthTextGo.AddComponent<RectTransform>();
        textRT.anchorMin = Vector2.zero;
        textRT.anchorMax = Vector2.one;
        textRT.offsetMin = Vector2.zero;
        textRT.offsetMax = Vector2.zero;
        Text hpText = healthTextGo.AddComponent<Text>();
        hpText.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        hpText.fontSize = 20;
        hpText.fontStyle = FontStyle.Bold;
        hpText.alignment = TextAnchor.MiddleCenter;
        hpText.color = new Color(1f, 0.96f, 0.72f, 1f);
        hpText.text = "100/100";
        hpText.raycastTarget = false;
        var outline = healthTextGo.AddComponent<Outline>();
        outline.effectColor = Color.black;
        outline.effectDistance = new Vector2(2f, -2f);

        HealthBar bar = healthBar.AddComponent<HealthBar>();
        bar.Initialize(fillImage, fillRT, null, hpText, canvasGroup, barRt);

        string path = "Assets/Prefabs/UI/HealthBar.prefab";
        System.IO.Directory.CreateDirectory("Assets/Prefabs/UI");
        PrefabUtility.SaveAsPrefabAsset(healthBar, path);
        DestroyImmediate(healthBar);
        Debug.Log("血条预制体创建成功: " + path);
    }
    
    private Color GetFactionColor(Faction faction)
    {
        switch (faction)
        {
            case Faction.Soviet:
                return new Color(0.86f, 0.08f, 0.24f);
            case Faction.USA:
                return new Color(0.12f, 0.56f, 1.0f);
            case Faction.Germany:
                return new Color(0.41f, 0.41f, 0.41f);
            case Faction.Italy:
                return new Color(0.0f, 0.57f, 0.27f);
            default:
                return Color.white;
        }
    }
}