using UnityEngine;
using System.Collections;
using TankBattle;

[RequireComponent(typeof(Rigidbody))]
public class TankController : MonoBehaviour
{
    [Header("坦克配置")]
    [SerializeField] private Faction faction = Faction.Soviet;
    [SerializeField] private TankType tankType = TankType.T34;
    [SerializeField] private float maxHealth = 100f;
    [SerializeField] private float moveSpeed = 3f;
    [SerializeField] private float rotationSpeed = 90f;
    
    [Header("炮塔配置")]
    [SerializeField] private Transform turretTransform;
    [SerializeField] private Transform barrelTransform;
    [SerializeField] private float turretRotationSpeed = 120f;
    
    [Header("开火配置")]
    [SerializeField] private Transform firePoint;
    [SerializeField] private GameObject bulletPrefab;
    [SerializeField] private float fireRate = 1f;
    [SerializeField] private float bulletSpeed = 20f;
    [SerializeField] private float bulletDamage = 20f;
    
    [Header("效果")]
    [SerializeField] private GameObject explosionPrefab;
    [SerializeField] private GameObject smokePrefab;
    [SerializeField] private GameObject muzzleFlashPrefab;
    [SerializeField] private GameObject hitEffectPrefab;
    
    [Header("UI")]
    [SerializeField] private GameObject healthBarPrefab;
    [SerializeField] private Vector3 healthBarOffset = new Vector3(0, 3.5f, 0);
    
    // 组件
    private Rigidbody rb;
    private HealthBar healthBar;
    private float networkShield;
    
    // 状态
    private float currentHealth;
    private float lastFireTime;
    private float networkReloadRemaining;
    private float networkReloadDuration;
    private bool isAlive = true;
    private uint tankId;
    private uint playerId;
    private Coroutine recoilCoroutine;
    private int damageFlashGeneration;
    
    // 网络同步
    private Vector3 targetPosition;
    private Quaternion targetRotation;
    private Quaternion targetTurretRotation;
    private float interpolationSpeed = 10f;
    private float lastNetworkHealth = -1f;
    private float lastBarHealth = -1f;
    private float lastBarMaxHealth = -1f;
    private float lastBarShield = -1f;
    private bool lastBarAlive = true;
    private float pendingDamageAmount;
    private float pendingDamageFlushTime;
    private Coroutine pendingDamageCoroutine;
    private const float DamagePopupMergeSeconds = 0.14f;
    
    public uint TankId { get => tankId; set => tankId = value; }
    public uint PlayerId { get => playerId; set => playerId = value; }
    public Faction Faction { get => faction; set { faction = value; SetTankColor(); } }
    
    public static bool AreHostileFactions(Faction a, Faction b) => a != b;
    public TankType TankType { get => tankType; set => tankType = value; }

    public Vector3 GetFirePointWorldPosition()
    {
        if (firePoint != null)
            return firePoint.position;
        if (barrelTransform != null)
            return barrelTransform.position + barrelTransform.forward * 0.5f;
        return transform.position + transform.forward * 0.8f;
    }

    /// <summary>运行时占位坦克等未在预制体里配好炮塔引用时调用。</summary>
    public void WireRuntimeTurret(Transform turret, Transform barrel = null, Transform firePointTransform = null)
    {
        turretTransform = turret;
        if (barrel != null) barrelTransform = barrel;
        if (firePointTransform != null) firePoint = firePointTransform;
    }

    public float CurrentHealth => currentHealth;
    public float MaxHealth => maxHealth;
    public bool IsAlive => isAlive;
    public float ReloadTimeRemaining => networkReloadRemaining;
    public float ReloadProgress => networkReloadDuration > 0.01f
        ? 1f - Mathf.Clamp01(networkReloadRemaining / networkReloadDuration)
        : 1f;
    public bool IsReloading => networkReloadRemaining > 0.01f;
    
    private void Awake()
    {
        rb = GetComponent<Rigidbody>();
        rb.isKinematic = true; // 网络同步，使用运动学刚体
        
        currentHealth = maxHealth;
    }

    /// <summary>由 GameManager 注入预制体；无预制体则运行时创建默认血条。</summary>
    public void ConfigureHealthBar(GameObject prefab, Vector3? offset = null)
    {
        if (prefab != null)
            healthBarPrefab = prefab;
        if (offset.HasValue)
            healthBarOffset = offset.Value;
        EnsureHealthBar();
    }

    private void EnsureHealthBar()
    {
        if (!ObjectExists(this) || !ObjectExists(gameObject) || !ObjectExists(transform))
            return;

        if (healthBar != null && !ObjectExists(healthBar))
            healthBar = null;

        if (healthBar == null)
        {
            if (!isAlive)
                return;

            if (healthBarPrefab != null)
            {
                GameObject barObj = Instantiate(healthBarPrefab);
                healthBar = barObj.GetComponent<HealthBar>();
                if (healthBar != null)
                    healthBar.AttachToSharedCanvas(transform, healthBarOffset);
                else
                    Destroy(barObj);
            }

            if (healthBar == null)
                healthBar = HealthBar.CreateForTarget(transform, healthBarOffset);
            if (healthBar == null)
                return;

            lastBarHealth = -1f;
            lastBarMaxHealth = -1f;
            if (tankId != 0)
                healthBar.gameObject.name = $"HealthBar_Tank_{tankId}";
        }
        else
        {
            healthBar.SetTarget(transform, healthBarOffset);
        }

        SyncHealthBar();
    }

    private static bool ObjectExists(Object obj) => obj != null;

    private void ResetDamageAccumulator()
    {
        pendingDamageAmount = 0f;
        pendingDamageFlushTime = 0f;
        if (pendingDamageCoroutine != null)
        {
            StopCoroutine(pendingDamageCoroutine);
            pendingDamageCoroutine = null;
        }
    }

    private void QueueDamagePopup(float amount)
    {
        if (amount <= 0.5f)
            return;

        pendingDamageAmount += amount;
        pendingDamageFlushTime = Time.unscaledTime + DamagePopupMergeSeconds;
        if (pendingDamageCoroutine == null)
            pendingDamageCoroutine = StartCoroutine(FlushPendingDamagePopup());
    }

    private IEnumerator FlushPendingDamagePopup()
    {
        while (Time.unscaledTime < pendingDamageFlushTime)
            yield return null;

        float total = pendingDamageAmount;
        pendingDamageAmount = 0f;
        pendingDamageCoroutine = null;
        if (total <= 0.5f || !isAlive)
            yield break;

        EnsureHealthBar();
        if (healthBar != null)
            healthBar.ShowDamage(total);
        else
            HealthBar.ShowDamagePopup(transform, healthBarOffset, total);
        ShowDamageEffect();
    }

    private void SyncHealthBar()
    {
        if (healthBar == null)
            return;

        float displayMax = maxHealth > 0.01f ? maxHealth : Mathf.Max(currentHealth, 1f);
        bool shieldActive = networkShield > 0.01f;
        if (Mathf.Approximately(currentHealth, lastBarHealth)
            && Mathf.Approximately(displayMax, lastBarMaxHealth)
            && Mathf.Approximately(networkShield, lastBarShield)
            && isAlive == lastBarAlive)
        {
            return;
        }

        lastBarHealth = currentHealth;
        lastBarMaxHealth = displayMax;
        lastBarShield = networkShield;
        lastBarAlive = isAlive;

        healthBar.SetVisible(isAlive);
        healthBar.UpdateHealth(currentHealth, displayMax);
        healthBar.UpdateShield(shieldActive ? 1f : 0f);
    }
    
    private void Start()
    {
        // 根据阵营设置颜色
        SetTankColor();
    }
    
    private void Update()
    {
        if (isAlive)
        {
            transform.position = Vector3.Lerp(transform.position, targetPosition, interpolationSpeed * Time.deltaTime);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, interpolationSpeed * Time.deltaTime);
            if (turretTransform != null)
            {
                Quaternion targetLocal = Quaternion.Inverse(targetRotation) * targetTurretRotation;
                turretTransform.localRotation = Quaternion.Slerp(turretTransform.localRotation, targetLocal, interpolationSpeed * Time.deltaTime);
            }
        }

        // 血条仅在 HP/护盾变化时由 SyncHealthBar 更新，避免每帧改 UI 文本触发临时分配
    }
    
    public static Color GetFactionColor(Faction f)
    {
        switch (f)
        {
            case Faction.Soviet: return new Color(0.86f, 0.08f, 0.24f);
            case Faction.USA: return new Color(0.12f, 0.56f, 1.0f);
            case Faction.Germany: return new Color(0.41f, 0.41f, 0.41f);
            case Faction.Italy: return new Color(0.0f, 0.57f, 0.27f);
            default: return Color.white;
        }
    }
    
    private void SetTankColor()
    {
        ApplyRendererColors(GetComponentsInChildren<Renderer>(true), GetFactionColor(faction));
    }

    private static void ApplyRendererColors(Renderer[] renderers, Color color)
    {
        if (renderers == null)
            return;
        foreach (Renderer renderer in renderers)
        {
            if (renderer == null)
                continue;
            renderer.material.color = color;
        }
    }
    
    public void TakeDamage(float damage, uint attackerId)
    {
        if (!isAlive) return;
        // 伤害由 C++ GameCore 权威结算，飘字在 UpdateFromNetwork 中按 HP 变化触发。
        ShowDamageEffect();
    }
    
    /// <summary>由 GameManager 在 C++ 生成子弹时调用，播放枪口特效与后坐力。</summary>
    public void PlayFireEffects()
    {
        if (!isAlive) return;

        if (muzzleFlashPrefab != null && firePoint != null)
        {
            GameObject muzzleFlash = Instantiate(muzzleFlashPrefab, firePoint.position, firePoint.rotation);
            muzzleFlash.transform.SetParent(firePoint, worldPositionStays: true);
            Destroy(muzzleFlash, 0.4f);
        }

        if (recoilCoroutine != null)
            StopCoroutine(recoilCoroutine);
        recoilCoroutine = StartCoroutine(FireRecoilRoutine());
    }

    public void Fire()
    {
        if (!isAlive || IsReloading || Time.time - lastFireTime < 1f / fireRate) return;
        
        lastFireTime = Time.time;
        PlayFireEffects();
        
        if (bulletPrefab != null && firePoint != null)
        {
            GameObject bulletObj = Instantiate(bulletPrefab, firePoint.position, firePoint.rotation);
            BulletController bullet = bulletObj.GetComponent<BulletController>();
            
            if (bullet != null)
                bullet.Initialize(tankId, playerId, faction, bulletDamage, bulletSpeed);
        }
    }
    
    private IEnumerator FireRecoilRoutine()
    {
        float barrelKick = barrelTransform != null ? 0.42f : 0f;
        float turretPitch = turretTransform != null ? 6f : 0f;
        float kickDuration = 0.07f;
        float recoverDuration = 0.22f;

        Vector3 barrelOrig = barrelTransform != null ? barrelTransform.localPosition : Vector3.zero;
        Vector3 barrelBack = barrelOrig - Vector3.forward * barrelKick;
        Quaternion turretOrig = turretTransform != null ? turretTransform.localRotation : Quaternion.identity;
        Quaternion turretKick = turretOrig * Quaternion.Euler(-turretPitch, 0f, 0f);

        float elapsed = 0f;
        while (elapsed < kickDuration)
        {
            elapsed += Time.deltaTime;
            float t = elapsed / kickDuration;
            float ease = 1f - (1f - t) * (1f - t);
            if (barrelTransform != null)
                barrelTransform.localPosition = Vector3.Lerp(barrelOrig, barrelBack, ease);
            if (turretTransform != null)
                turretTransform.localRotation = Quaternion.Slerp(turretOrig, turretKick, ease);
            yield return null;
        }

        elapsed = 0f;
        while (elapsed < recoverDuration)
        {
            elapsed += Time.deltaTime;
            float t = elapsed / recoverDuration;
            float ease = t * t;
            if (barrelTransform != null)
                barrelTransform.localPosition = Vector3.Lerp(barrelBack, barrelOrig, ease);
            if (turretTransform != null)
                turretTransform.localRotation = Quaternion.Slerp(turretKick, turretOrig, ease);
            yield return null;
        }

        if (barrelTransform != null) barrelTransform.localPosition = barrelOrig;
        if (turretTransform != null) turretTransform.localRotation = turretOrig;
        recoilCoroutine = null;
    }
    
    private void ShowDamageEffect()
    {
        int generation = ++damageFlashGeneration;
        StartCoroutine(DamageFlash(generation));

        if (hitEffectPrefab != null)
        {
            GameObject hitEffect = Instantiate(hitEffectPrefab, transform.position + Vector3.up, Quaternion.identity);
            Destroy(hitEffect, 2f);
        }
    }
    
    private IEnumerator DamageFlash(int generation)
    {
        Renderer[] renderers = GetComponentsInChildren<Renderer>(true);
        if (renderers.Length == 0)
            yield break;

        Color factionColor = GetFactionColor(faction);
        ApplyRendererColors(renderers, factionColor);

        for (int i = 0; i < 3; i++)
        {
            if (generation != damageFlashGeneration)
                yield break;

            ApplyRendererColors(renderers, Color.white);
            yield return new WaitForSeconds(0.1f);

            if (generation != damageFlashGeneration)
                yield break;

            ApplyRendererColors(renderers, Color.red);
            yield return new WaitForSeconds(0.1f);
        }

        if (generation == damageFlashGeneration && isAlive)
            ApplyRendererColors(renderers, factionColor);
    }
    
    private void Die(bool destroyAfterDelay)
    {
        isAlive = false;
        currentHealth = 0;
        lastNetworkHealth = -1f;
        lastBarHealth = -1f;
        ResetDamageAccumulator();
        ++damageFlashGeneration;
        
        if (explosionPrefab != null)
        {
            GameObject explosion = Instantiate(explosionPrefab, transform.position, Quaternion.identity);
            Destroy(explosion, 3f);
        }
        else
        {
            BattleVfxController vfx = FindObjectOfType<BattleVfxController>();
            if (vfx != null)
                vfx.SpawnExplosionAt(transform.position);
        }
        
        if (smokePrefab != null)
        {
            GameObject smoke = Instantiate(smokePrefab, transform.position, Quaternion.identity);
            Destroy(smoke, 5f);
        }
        
        foreach (Renderer renderer in GetComponentsInChildren<Renderer>())
            renderer.enabled = false;
        
        foreach (Collider collider in GetComponentsInChildren<Collider>())
            collider.enabled = false;
        
        if (healthBar != null)
        {
            if (destroyAfterDelay)
            {
                var bar = healthBar;
                healthBar = null;
                if (ObjectExists(bar))
                    Destroy(bar.gameObject);
            }
            else if (ObjectExists(healthBar))
                healthBar.SetVisible(false);
        }
        
        if (destroyAfterDelay)
            Destroy(gameObject, 5f);
    }

    private void ReviveFromNetwork(float health)
    {
        isAlive = true;
        currentHealth = health;
        lastNetworkHealth = health;
        lastBarHealth = -1f;
        ResetDamageAccumulator();
        ++damageFlashGeneration;
        
        foreach (Renderer renderer in GetComponentsInChildren<Renderer>())
            renderer.enabled = true;
        
        foreach (Collider collider in GetComponentsInChildren<Collider>())
            collider.enabled = false;
        
        EnsureHealthBar();
        SyncHealthBar();
        SetTankColor();
    }
    
    // 网络同步
    public void UpdateFromNetwork(
        Vector3 position,
        Quaternion rotation,
        Quaternion turretRotation,
        float health,
        float maxHp,
        bool alive,
        float respawnTimeRemaining,
        float shield,
        float reloadTimeRemaining,
        float reloadDuration)
    {
        targetPosition = position;
        targetRotation = rotation;
        targetTurretRotation = turretRotation;
        if (maxHp > 0f)
            maxHealth = maxHp;
        networkShield = shield;
        networkReloadRemaining = reloadTimeRemaining;
        networkReloadDuration = reloadDuration;

        if (!alive && isAlive)
            Die(destroyAfterDelay: playerId == 0);
        else if (alive && !isAlive)
        {
            ReviveFromNetwork(health);
            transform.position = position;
            transform.rotation = rotation;
        }
        else if (alive && isAlive)
        {
            EnsureHealthBar();
        }

        if (alive && isAlive && lastNetworkHealth >= 0f)
        {
            if (health > lastNetworkHealth + 0.5f)
            {
                // 复活/治疗：只刷新基线，不飘字
                ResetDamageAccumulator();
            }
            else if (lastNetworkHealth > health + 0.5f)
            {
                float damageTaken = lastNetworkHealth - health;
                QueueDamagePopup(damageTaken);
                ShowDamageEffect();
            }
        }

        currentHealth = health;
        lastNetworkHealth = health;
        SyncHealthBar();
        
        if (!alive && !isAlive && playerId != 0 && respawnTimeRemaining > 0f)
            transform.position = targetPosition;
    }
    
    public void SetMoveInput(UnityEngine.Vector2 input)
    {
        if (!isAlive) return;
        
        // 移动
        Vector3 moveDirection = new Vector3(input.x, 0, input.y);
        if (moveDirection.magnitude > 0.1f)
        {
            // 旋转车身
            Quaternion targetRotation = Quaternion.LookRotation(moveDirection);
            transform.rotation = Quaternion.RotateTowards(transform.rotation, targetRotation, rotationSpeed * Time.deltaTime);
            
            // 移动
            transform.Translate(Vector3.forward * moveSpeed * Time.deltaTime);
        }
    }
    
    private void OnDestroy()
    {
        if (healthBar != null)
        {
            var bar = healthBar;
            healthBar = null;
            if (ObjectExists(bar))
                Destroy(bar.gameObject);
        }
    }

    public void SetAimDirection(Vector3 aimDirection)
    {
        if (turretTransform == null || !isAlive) return;
        
        aimDirection.y = 0;
        if (aimDirection.magnitude > 0.1f)
        {
            Quaternion targetRotation = Quaternion.LookRotation(aimDirection);
            turretTransform.rotation = Quaternion.RotateTowards(turretTransform.rotation, targetRotation, turretRotationSpeed * Time.deltaTime);
        }
    }
}