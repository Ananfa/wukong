using UnityEngine;

using UnityEditor;

using System.IO;

public class MaterialCreator
{
    /// <summary>
    /// 按顺序查找 Shader，避免 Particles/Additive、Particles/Standard Unlit 等在内置管线改名或
    /// 仅 URP 工程里不存在时 Shader.Find 为 null 导致 new Material(null) 抛错。
    /// </summary>
    private static Shader FindShaderOrDefault(params string[] names)
    {
        if (names != null)
        {
            foreach (string n in names)
            {
                if (string.IsNullOrEmpty(n)) continue;
                Shader s = Shader.Find(n);
                if (s != null) return s;
            }
        }

        Shader fallback = Shader.Find("Unlit/Color");
        if (fallback != null) return fallback;
        fallback = Shader.Find("Sprites/Default");
        if (fallback != null) return fallback;
        fallback = Shader.Find("UI/Default");
        if (fallback != null) return fallback;
        fallback = Shader.Find("Standard");
        if (fallback != null) return fallback;
        fallback = Shader.Find("Universal Render Pipeline/Lit");
        if (fallback != null) return fallback;
        fallback = Shader.Find("Hidden/InternalErrorShader");
        if (fallback != null) return fallback;

        Debug.LogError("MaterialCreator: 未找到任何可用 Shader。请使用 Built-in 3D 或安装 URP/Lit 默认资源。");
        return null;
    }

    private static void SetParticleColor(Material m, Color c)
    {
        if (m == null) return;
        if (m.HasProperty("_Color")) m.SetColor("_Color", c);
        if (m.HasProperty("_TintColor")) m.SetColor("_TintColor", c);
        if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", c);
    }

    [MenuItem("Tools/坦克大战/创建游戏材质")]

    public static void CreateGameMaterials()

    {

    CreateTankMaterials();

    CreateEffectMaterials();

    CreateTerrainMaterials();

    CreateUIMaterials();

    Debug.Log("游戏材质创建完成！");
    }

    private static void CreateTankMaterials()
    {
    // 苏联坦克材质
    Material sovietMaterial = new Material(FindShaderOrDefault("Standard", "Standard (Specular setup)", "Universal Render Pipeline/Lit"));
    sovietMaterial.color = new Color(0.86f, 0.08f, 0.24f);
    sovietMaterial.SetFloat("_Metallic", 0.3f);
    sovietMaterial.SetFloat("_Glossiness", 0.2f);
    SaveMaterial(sovietMaterial, "Soviet_Tank");

    // 美国坦克材质
    Material usaMaterial = new Material(FindShaderOrDefault("Standard", "Standard (Specular setup)", "Universal Render Pipeline/Lit"));
    usaMaterial.color = new Color(0.12f, 0.56f, 1.0f);
    usaMaterial.SetFloat("_Metallic", 0.2f);
    usaMaterial.SetFloat("_Glossiness", 0.3f);
    SaveMaterial(usaMaterial, "USA_Tank");

    // 德国坦克材质
    Material germanyMaterial = new Material(FindShaderOrDefault("Standard", "Standard (Specular setup)", "Universal Render Pipeline/Lit"));
    germanyMaterial.color = new Color(0.41f, 0.41f, 0.41f);
    germanyMaterial.SetFloat("_Metallic", 0.4f);
    germanyMaterial.SetFloat("_Glossiness", 0.4f);
    SaveMaterial(germanyMaterial, "Germany_Tank");

    // 履带材质
    Material trackMaterial = new Material(FindShaderOrDefault("Standard", "Standard (Specular setup)", "Universal Render Pipeline/Lit"));
    trackMaterial.color = new Color(0.1f, 0.1f, 0.1f);
    trackMaterial.SetFloat("_Metallic", 0.5f);
    trackMaterial.SetFloat("_Glossiness", 0.1f);
    SaveMaterial(trackMaterial, "Tank_Track");
    }

    private static void CreateEffectMaterials()
    {
    // 爆炸材质
    Material explosionMaterial = new Material(FindShaderOrDefault(
        "Particles/Standard Unlit",
        "Mobile/Particles/Alpha Blended",
        "Legacy Shaders/Particles/Alpha Blended",
        "Unlit/Color"));
    SetParticleColor(explosionMaterial, new Color(1f, 0.5f, 0f, 1f));
    SaveMaterial(explosionMaterial, "Explosion");

    // 烟雾材质
    Material smokeMaterial = new Material(FindShaderOrDefault(
        "Particles/Standard Unlit",
        "Mobile/Particles/Alpha Blended",
        "Unlit/Color"));
    SetParticleColor(smokeMaterial, new Color(0.3f, 0.3f, 0.3f, 0.5f));
    SaveMaterial(smokeMaterial, "Smoke");

    // 枪口火焰材质（老名字 Particles/Additive 在部分版本已移除）
    Material muzzleFlashMaterial = new Material(FindShaderOrDefault(
        "Particles/Additive",
        "Mobile/Particles/Additive",
        "Legacy Shaders/Particles/Additive",
        "Particles/Standard Unlit",
        "Unlit/Color",
        "Sprites/Default"));
    SetParticleColor(muzzleFlashMaterial, new Color(1f, 0.8f, 0.2f, 0.8f));
    SaveMaterial(muzzleFlashMaterial, "MuzzleFlash");

    // 护盾材质
    Material shieldMaterial = new Material(FindShaderOrDefault("Standard", "Universal Render Pipeline/Lit", "Unlit/Color"));
    shieldMaterial.SetColor("_Color", new Color(0f, 0.5f, 1f, 0.3f));
    shieldMaterial.SetFloat("_Metallic", 0.8f);
    shieldMaterial.SetFloat("_Glossiness", 0.9f);
    shieldMaterial.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
    shieldMaterial.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
    shieldMaterial.SetOverrideTag("RenderType", "Transparent");
    shieldMaterial.SetInt("_ZWrite", 0);
    shieldMaterial.DisableKeyword("_ALPHATEST_ON");
    shieldMaterial.EnableKeyword("_ALPHABLEND_ON");
    shieldMaterial.DisableKeyword("_ALPHAPREMULTIPLY_ON");
    SaveMaterial(shieldMaterial, "Shield");

    // 治疗效果材质
    Material healMaterial = new Material(FindShaderOrDefault(
        "Particles/Additive",
        "Mobile/Particles/Additive",
        "Legacy Shaders/Particles/Additive",
        "Particles/Standard Unlit",
        "Unlit/Color"));
    SetParticleColor(healMaterial, new Color(0f, 1f, 0.2f, 0.6f));
    SaveMaterial(healMaterial, "Heal");
    }

    private static void CreateTerrainMaterials()
    {
    // 地面材质
    Material groundMaterial = new Material(FindShaderOrDefault("Standard", "Universal Render Pipeline/Lit", "Unlit/Color"));
    groundMaterial.color = new Color(0.2f, 0.4f, 0.2f);
    groundMaterial.SetFloat("_Metallic", 0f);
    groundMaterial.SetFloat("_Glossiness", 0f);
    SaveMaterial(groundMaterial, "Ground");

    // 岩石材质
    Material rockMaterial = new Material(FindShaderOrDefault("Standard", "Universal Render Pipeline/Lit", "Unlit/Color"));
    rockMaterial.color = new Color(0.4f, 0.3f, 0.2f);
    rockMaterial.SetFloat("_Metallic", 0.1f);
    rockMaterial.SetFloat("_Glossiness", 0.1f);
    SaveMaterial(rockMaterial, "Rock");

    // 草地材质
    Material grassMaterial = new Material(FindShaderOrDefault("Standard", "Universal Render Pipeline/Lit", "Unlit/Color"));
    grassMaterial.color = new Color(0.1f, 0.6f, 0.1f);
    grassMaterial.SetFloat("_Metallic", 0f);
    grassMaterial.SetFloat("_Glossiness", 0f);
    SaveMaterial(grassMaterial, "Grass");
    }

    private static void CreateUIMaterials()
    {
    // 血条背景
    Material healthBarBgMaterial = new Material(FindShaderOrDefault("UI/Default", "UI/Default ETC1", "Unlit/Transparent"));
    healthBarBgMaterial.color = new Color(0.2f, 0.2f, 0.2f, 0.8f);
    SaveMaterial(healthBarBgMaterial, "HealthBar_Background");

    // 血条填充 - 高血量
    Material healthBarHighMaterial = new Material(FindShaderOrDefault("UI/Default", "UI/Default ETC1", "Unlit/Color"));
    healthBarHighMaterial.color = new Color(0f, 1f, 0f, 1f);
    SaveMaterial(healthBarHighMaterial, "HealthBar_High");

    // 血条填充 - 中等血量
    Material healthBarMediumMaterial = new Material(FindShaderOrDefault("UI/Default", "UI/Default ETC1", "Unlit/Color"));
    healthBarMediumMaterial.color = new Color(1f, 1f, 0f, 1f);
    SaveMaterial(healthBarMediumMaterial, "HealthBar_Medium");

    // 血条填充 - 低血量
    Material healthBarLowMaterial = new Material(FindShaderOrDefault("UI/Default", "UI/Default ETC1", "Unlit/Color"));
    healthBarLowMaterial.color = new Color(1f, 0f, 0f, 1f);
    SaveMaterial(healthBarLowMaterial, "HealthBar_Low");

    // 护盾条
    Material shieldBarMaterial = new Material(FindShaderOrDefault("UI/Default", "UI/Default ETC1", "Unlit/Color"));
    shieldBarMaterial.color = new Color(0f, 0.5f, 1f, 0.5f);
    SaveMaterial(shieldBarMaterial, "ShieldBar");
    }

    private static void SaveMaterial(Material material, string name)
    {
    if (material == null || material.shader == null)
    {
        Debug.LogError("MaterialCreator: 材质未创建，Shader 为空: " + name);
        if (material != null) Object.DestroyImmediate(material);
        return;
    }

    string folderPath = "Assets/Materials";
    if (!Directory.Exists(folderPath))
    {
        Directory.CreateDirectory(folderPath);
    }

    string materialPath = folderPath + "/" + name + ".mat";
    AssetDatabase.CreateAsset(material, materialPath);

    Debug.Log("材质创建成功: " + materialPath);
    }
}