using UnityEngine;

using System.Collections;

using System.Collections.Generic;

public class EffectGenerator : MonoBehaviour

{

    [System.Serializable]

    public class EffectData

    {

    public string effectName;

    public GameObject prefab;

    public float duration = 2f;

    public int poolSize = 10;

    }

    [Header("效果预制体")]
    [SerializeField] private EffectData[] effectData;

    [Header("音效")]
    [SerializeField] private AudioClip explosionSound;
    [SerializeField] private AudioClip gunfireSound;
    [SerializeField] private AudioClip hitSound;
    [SerializeField] private AudioClip abilitySound;

    // 对象池
    private Dictionary<string, Queue<GameObject>> effectPools = new Dictionary<string, Queue<GameObject>>();
    private Dictionary<string, EffectData> effectLookup = new Dictionary<string, EffectData>();

    // 引用
    private AudioSource audioSource;

    private void Awake()
    {
        audioSource = GetComponent<AudioSource>();
        if (audioSource == null)
        {
            audioSource = gameObject.AddComponent<AudioSource>();
        }
        
        InitializePools();
    }

    private void InitializePools()
    {
        foreach (EffectData data in effectData)
        {
            if (data.prefab == null) continue;
            
            effectLookup[data.effectName] = data;
            
            Queue<GameObject> pool = new Queue<GameObject>();
            
            for (int i = 0; i < data.poolSize; i++)
            {
                GameObject effect = Instantiate(data.prefab);
                effect.SetActive(false);
                effect.transform.SetParent(transform);
                
                // 添加自动回收组件
                AutoDestroy autoDestroy = effect.GetComponent<AutoDestroy>();
                if (autoDestroy == null)
                {
                    autoDestroy = effect.AddComponent<AutoDestroy>();
                }
                autoDestroy.lifetime = data.duration;
                
                pool.Enqueue(effect);
            }
            
            effectPools[data.effectName] = pool;
        }
    }

    public GameObject SpawnEffect(string effectName, Vector3 position, Quaternion rotation)
    {
        if (!effectPools.ContainsKey(effectName) || !effectLookup.ContainsKey(effectName))
        {
            Debug.LogWarning($"未找到效果: {effectName}");
            return null;
        }
        
        Queue<GameObject> pool = effectPools[effectName];
        
        if (pool.Count == 0)
        {
            // 如果池空了，创建新对象
            EffectData data = effectLookup[effectName];
            GameObject newEffect = Instantiate(data.prefab, position, rotation);
            
            AutoDestroy autoDestroy = newEffect.GetComponent<AutoDestroy>();
            if (autoDestroy == null)
            {
                autoDestroy = newEffect.AddComponent<AutoDestroy>();
            }
            autoDestroy.lifetime = data.duration;
            
            return newEffect;
        }
        else
        {
            // 从池中获取对象
            GameObject effect = pool.Dequeue();
            effect.SetActive(true);
            effect.transform.position = position;
            effect.transform.rotation = rotation;
            
            // 重新激活组件
            ParticleSystem[] particles = effect.GetComponentsInChildren<ParticleSystem>();
            foreach (ParticleSystem ps in particles)
            {
                ps.Clear();
                ps.Play();
            }
            
            // 延迟回收
            StartCoroutine(ReturnToPoolAfterDelay(effectName, effect, effectLookup[effectName].duration));
            
            return effect;
        }
    }

    public void SpawnExplosion(Vector3 position, float size = 1f)
    {
        GameObject explosion = SpawnEffect("Explosion", position, Quaternion.identity);
        if (explosion != null)
        {
            explosion.transform.localScale = Vector3.one * size;
        }
        
        PlaySound(explosionSound, 0.5f);
    }

    public void SpawnMuzzleFlash(Vector3 position, Quaternion rotation)
    {
        SpawnEffect("MuzzleFlash", position, rotation);
        PlaySound(gunfireSound, 0.3f);
    }

    public void SpawnHitEffect(Vector3 position, Vector3 normal)
    {
        Quaternion rotation = Quaternion.LookRotation(normal);
        SpawnEffect("HitEffect", position, rotation);
        PlaySound(hitSound, 0.2f);
    }

    public void SpawnSmoke(Vector3 position, float duration = 5f)
    {
        GameObject smoke = SpawnEffect("Smoke", position, Quaternion.identity);
        if (smoke != null)
        {
            AutoDestroy autoDestroy = smoke.GetComponent<AutoDestroy>();
            if (autoDestroy != null)
            {
                autoDestroy.lifetime = duration;
            }
        }
    }

    public void SpawnShieldEffect(Vector3 position, float duration = 3f)
    {
        GameObject shield = SpawnEffect("Shield", position, Quaternion.identity);
        if (shield != null)
        {
            AutoDestroy autoDestroy = shield.GetComponent<AutoDestroy>();
            if (autoDestroy != null)
            {
                autoDestroy.lifetime = duration;
            }
        }
        
        PlaySound(abilitySound, 0.4f);
    }

    public void SpawnHealEffect(Vector3 position)
    {
        SpawnEffect("Heal", position, Quaternion.identity);
        PlaySound(abilitySound, 0.4f);
    }

    public void SpawnSpeedEffect(Vector3 position)
    {
        SpawnEffect("SpeedBoost", position, Quaternion.identity);
        PlaySound(abilitySound, 0.4f);
    }

    public void SpawnTrailEffect(Vector3 startPos, Vector3 endPos, float width = 0.2f, Color color = default)
    {
        if (color == default) color = Color.yellow;
        
        GameObject trail = SpawnEffect("Trail", startPos, Quaternion.identity);
        if (trail != null)
        {
            LineRenderer lineRenderer = trail.GetComponent<LineRenderer>();
            if (lineRenderer != null)
            {
                lineRenderer.startWidth = width;
                lineRenderer.endWidth = width;
                lineRenderer.startColor = color;
                lineRenderer.endColor = color;
                
                lineRenderer.SetPosition(0, startPos);
                lineRenderer.SetPosition(1, endPos);
            }
        }
    }

    private IEnumerator ReturnToPoolAfterDelay(string effectName, GameObject effect, float delay)
    {
        yield return new WaitForSeconds(delay);
        
        ReturnToPool(effectName, effect);
    }

    public void ReturnToPool(string effectName, GameObject effect)
    {
        if (effect == null || !effectPools.ContainsKey(effectName)) return;
        
        effect.SetActive(false);
        effect.transform.SetParent(transform);
        
        effectPools[effectName].Enqueue(effect);
    }

    private void PlaySound(AudioClip clip, float volume = 1f)
    {
        if (clip != null && audioSource != null)
        {
            audioSource.PlayOneShot(clip, volume);
        }
    }

    // 自动销毁组件
    [RequireComponent(typeof(ParticleSystem))]
    public class AutoDestroy : MonoBehaviour
    {
        public float lifetime = 2f;
        
        private ParticleSystem particles;
        private float timer = 0f;
        
        private void Awake()
        {
            particles = GetComponent<ParticleSystem>();
        }
        
        private void OnEnable()
        {
            timer = 0f;
            
            if (particles != null)
            {
                particles.Clear();
                particles.Play();
            }
        }
        
        private void Update()
        {
            timer += Time.deltaTime;
            
            if (timer >= lifetime)
            {
                gameObject.SetActive(false);
            }
        }
    }
}