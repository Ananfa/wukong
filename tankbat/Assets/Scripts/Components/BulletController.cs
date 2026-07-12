using UnityEngine;
using System.Collections;
using TankBattle;

public class BulletController : MonoBehaviour

{

[Header("子弹配置")]

[SerializeField] private float speed = 20f;

[SerializeField] private float damage = 20f;

[SerializeField] private float lifeTime = 3f;

[SerializeField] private bool penetrating = false;

[SerializeField] private GameObject hitEffectPrefab;
[Header("轨迹效果")]
[SerializeField] private GameObject trailEffect;
[SerializeField] private Light bulletLight;
[SerializeField] private Color normalColor = Color.yellow;
[SerializeField] private Color penetrateColor = Color.red;

// 状态
private uint ownerTankId;
private uint ownerPlayerId;
private Faction ownerFaction = Faction.Soviet;
private Vector3 direction;
private float spawnTime;
private bool isActive = true;

public void Initialize(uint tankId, uint playerId, Faction ownerFactionValue, float bulletDamage, float bulletSpeed)
{
    ownerTankId = tankId;
    ownerPlayerId = playerId;
    ownerFaction = ownerFactionValue;
    damage = bulletDamage;
    speed = bulletSpeed;
    spawnTime = Time.time;
    
    // 设置子弹颜色
    if (penetrating && bulletLight != null)
    {
        bulletLight.color = penetrateColor;
        
        // 放大穿透子弹
        transform.localScale *= 1.5f;
    }
}

private void Start()
{
    // 自动销毁
    Destroy(gameObject, lifeTime);
    
    // 播放声音
    AudioSource audioSource = GetComponent<AudioSource>();
    if (audioSource != null)
    {
        audioSource.Play();
    }
}

private void Update()
{
    if (!isActive) return;
    
    // 移动
    transform.position += transform.forward * speed * Time.deltaTime;
    
    // 检查生命周期
    if (Time.time - spawnTime > lifeTime)
    {
        Destroy(gameObject);
    }
}

private void OnTriggerEnter(Collider other)
{
    if (!isActive) return;
    
    // 检查是否击中坦克
    TankController tank = other.GetComponent<TankController>();
    if (tank != null
        && tank.TankId != ownerTankId
        && tank.IsAlive
        && TankController.AreHostileFactions(ownerFaction, tank.Faction))
    {
        // 伤害与 HP 由 C++ 权威端处理；本地触发器仅作命中特效。
        if (hitEffectPrefab != null)
        {
            GameObject hitEffect = Instantiate(hitEffectPrefab, transform.position, Quaternion.identity);
            Destroy(hitEffect, 2f);
        }
        
        if (!penetrating)
        {
            isActive = false;
            Destroy(gameObject);
        }
    }
    // 检查是否击中环境物体
    else if (other.CompareTag("Environment") || other.CompareTag("Obstacle"))
    {
        isActive = false;
        
        if (hitEffectPrefab != null)
        {
            GameObject hitEffect = Instantiate(hitEffectPrefab, transform.position, Quaternion.identity);
            Destroy(hitEffect, 2f);
        }
        
        Destroy(gameObject);
    }
}

// 调试绘制
private void OnDrawGizmos()
{
    Gizmos.color = penetrating ? Color.red : Color.yellow;
    Gizmos.DrawWireSphere(transform.position, 0.2f);
    Gizmos.DrawLine(transform.position, transform.position + transform.forward * 2f);
}
}