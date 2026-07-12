using System.Collections.Generic;
using TankBattle;
using UnityEngine;

/// <summary>
/// 根据 GameCore 快照 diff 在客户端播放命中/击毁等纯表现特效（不参与战斗逻辑）。
/// </summary>
public class BattleVfxController : MonoBehaviour
{
    [SerializeField] private EffectGenerator effectGenerator;
    [SerializeField] private float worldDisplayScale = 0.22f;

    private readonly Dictionary<uint, TankVfxTrack> tracks = new Dictionary<uint, TankVfxTrack>();

    private struct TankVfxTrack
    {
        public float hp;
        public bool isAlive;
        public TankBattle.Vector2 position;
    }

    public void Configure(EffectGenerator generator, float displayScale)
    {
        if (generator != null)
            effectGenerator = generator;
        worldDisplayScale = displayScale;
    }

    public void ResetTracks()
    {
        tracks.Clear();
    }

    /// <summary>在 UpdateTanks 之前调用，以便捕获从快照中消失的 AI 坦克。</summary>
    public void ProcessSnapshot(GameSnapshot snapshot)
    {
        if (snapshot == null)
            return;

        var currentIds = new HashSet<uint>();
        if (snapshot.tanks != null)
        {
            foreach (TankState tank in snapshot.tanks)
            {
                currentIds.Add(tank.id);
                Vector3 worldPos = NativeToUnity(tank.position);

                if (tracks.TryGetValue(tank.id, out TankVfxTrack prev))
                {
                    if (prev.isAlive && tank.isAlive && prev.hp > tank.hp + 0.5f)
                        SpawnHit(worldPos);
                }

                tracks[tank.id] = new TankVfxTrack
                {
                    hp = tank.hp,
                    isAlive = tank.isAlive,
                    position = tank.position
                };
            }
        }

        List<uint> removed = null;
        foreach (KeyValuePair<uint, TankVfxTrack> entry in tracks)
        {
            if (currentIds.Contains(entry.Key))
                continue;

            if (entry.Value.isAlive)
                SpawnExplosion(NativeToUnity(entry.Value.position));

            if (removed == null)
                removed = new List<uint>();
            removed.Add(entry.Key);
        }

        if (removed != null)
        {
            foreach (uint id in removed)
                tracks.Remove(id);
        }
    }

    public void SpawnExplosionAt(Vector3 worldPosition, float size = 1f)
    {
        SpawnExplosion(worldPosition, size);
    }

    public void SpawnExplosionAtNative(TankBattle.Vector2 nativePosition, float size = 1f)
    {
        SpawnExplosion(NativeToUnity(nativePosition), size);
    }

    private Vector3 NativeToUnity(TankBattle.Vector2 native)
    {
        return new Vector3(native.x * worldDisplayScale, 0f, native.y * worldDisplayScale);
    }

    private void SpawnExplosion(Vector3 position, float size = 1f)
    {
        if (effectGenerator != null)
        {
            effectGenerator.SpawnExplosion(position, size);
            return;
        }

        CreateFallbackBurst(position, new Color(1f, 0.35f, 0.1f), 1.2f * size, 0.85f);
    }

    private void SpawnHit(Vector3 position)
    {
        Vector3 hitPos = position + Vector3.up * 0.4f;
        if (effectGenerator != null)
        {
            effectGenerator.SpawnHitEffect(hitPos, Vector3.up);
            return;
        }

        CreateFallbackBurst(hitPos, new Color(1f, 0.55f, 0.15f), 0.45f, 0.35f);
    }

    private static void CreateFallbackBurst(Vector3 position, Color color, float size, float duration)
    {
        GameObject go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        go.name = "CombatVfx";
        go.transform.position = position;
        go.transform.localScale = Vector3.one * size;

        Collider col = go.GetComponent<Collider>();
        if (col != null)
            Destroy(col);

        Renderer renderer = go.GetComponent<Renderer>();
        if (renderer != null)
            renderer.material.color = color;

        Destroy(go, duration);
    }
}
