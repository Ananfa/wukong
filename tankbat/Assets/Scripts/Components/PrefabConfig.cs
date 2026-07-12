using System.Collections.Generic;
using UnityEngine;
using TankBattle;

[CreateAssetMenu(fileName = "PrefabConfig", menuName = "坦克大战/预制体配置")]
public class PrefabConfig : ScriptableObject
{
    [System.Serializable]
    public class TankPrefabData
    {
        public Faction faction;
        public TankType type;
        public GameObject prefab;
        public Color color = Color.white;
    }
    
    [System.Serializable]
    public class EffectPrefabData
    {
        public string effectName;
        public GameObject prefab;
        public float duration = 2f;
    }
    
    [Header("坦克预制体")]
    public List<TankPrefabData> tankPrefabs = new List<TankPrefabData>();
    
    [Header("子弹预制体")]
    public GameObject bulletPrefab;
    public GameObject penetratingBulletPrefab;
    public GameObject sniperBulletPrefab;
    
    [Header("特效预制体")]
    public List<EffectPrefabData> effectPrefabs = new List<EffectPrefabData>();
    
    [Header("UI预制体")]
    public GameObject healthBarPrefab;
    public GameObject minimapPrefab;
    
    [Header("音效")]
    public AudioClip explosionSound;
    public AudioClip gunfireSound;
    public AudioClip hitSound;
    public AudioClip abilitySound;
    public AudioClip engineSound;
    
    // 获取坦克预制体
    public GameObject GetTankPrefab(Faction faction, TankType type)
    {
        foreach (var data in tankPrefabs)
        {
            if (data.faction == faction && data.type == type)
            {
                return data.prefab;
            }
        }
        return null;
    }
    
    // 获取坦克颜色
    public Color GetTankColor(Faction faction, TankType type)
    {
        foreach (var data in tankPrefabs)
        {
            if (data.faction == faction && data.type == type)
            {
                return data.color;
            }
        }
        
        // 默认颜色
        switch (faction)
        {
            case Faction.Soviet: return new Color(0.86f, 0.08f, 0.24f);
            case Faction.USA: return new Color(0.12f, 0.56f, 1.0f);
            case Faction.Germany: return new Color(0.41f, 0.41f, 0.41f);
            case Faction.Italy: return new Color(0.0f, 0.57f, 0.27f);
            default: return Color.white;
        }
    }
    
    // 获取特效预制体
    public GameObject GetEffectPrefab(string effectName)
    {
        foreach (var data in effectPrefabs)
        {
            if (data.effectName == effectName)
            {
                return data.prefab;
            }
        }
        return null;
    }
    
    // 获取特效持续时间
    public float GetEffectDuration(string effectName)
    {
        foreach (var data in effectPrefabs)
        {
            if (data.effectName == effectName)
            {
                return data.duration;
            }
        }
        return 2f;
    }
    
    // 验证配置
    public void ValidateConfig()
    {
        // 检查坦克预制体
        foreach (var data in tankPrefabs)
        {
            if (data.prefab == null)
            {
                Debug.LogWarning($"坦克预制体配置错误: {data.faction} - {data.type}");
            }
        }
        
        // 检查子弹预制体
        if (bulletPrefab == null) Debug.LogWarning("子弹预制体未配置");
        if (penetratingBulletPrefab == null) Debug.LogWarning("穿透子弹预制体未配置");
        if (sniperBulletPrefab == null) Debug.LogWarning("狙击子弹预制体未配置");
        
        // 检查UI预制体
        if (healthBarPrefab == null) Debug.LogWarning("血条预制体未配置");
        if (minimapPrefab == null) Debug.LogWarning("小地图预制体未配置");
    }
}