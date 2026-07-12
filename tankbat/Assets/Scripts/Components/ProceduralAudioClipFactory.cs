using UnityEngine;

/// <summary>无外部音频资源时的占位 Clip，可在 Inspector 指定正式素材覆盖。</summary>
public static class ProceduralAudioClipFactory
{
    public static AudioClip CreateGunfireClip()
    {
        const int sampleRate = 44100;
        const float duration = 0.12f;
        int sampleCount = Mathf.CeilToInt(sampleRate * duration);
        var data = new float[sampleCount];
        uint seed = 0x9E3779B9u;

        for (int i = 0; i < sampleCount; i++)
        {
            float t = i / (float)sampleRate;
            float envelope = Mathf.Exp(-t * 42f);
            seed = seed * 1664525u + 1013904223u;
            float noise = (seed / (float)uint.MaxValue) * 2f - 1f;
            float crack = Mathf.Sin(t * 900f) * Mathf.Exp(-t * 120f) * 0.35f;
            data[i] = (noise * 0.55f + crack) * envelope * 0.85f;
        }

        AudioClip clip = AudioClip.Create("ProceduralGunfire", sampleCount, 1, sampleRate, false);
        clip.SetData(data, 0);
        return clip;
    }

    public static AudioClip CreateBattleMusicClip(int variant = 0)
    {
        const int sampleRate = 44100;
        const float duration = 8f;
        int sampleCount = Mathf.CeilToInt(sampleRate * duration);
        var data = new float[sampleCount];

        float baseFreq = variant == 0 ? 55f : 49f;
        float midFreq = variant == 0 ? 110f : 98f;
        float pulseRate = variant == 0 ? 0.6f : 0.45f;

        for (int i = 0; i < sampleCount; i++)
        {
            float t = i / (float)sampleRate;
            float pulse = 0.5f + 0.5f * Mathf.Sin(t * pulseRate * Mathf.PI * 2f);
            float bass = Mathf.Sin(t * baseFreq * Mathf.PI * 2f) * 0.07f;
            float mid = Mathf.Sin(t * midFreq * Mathf.PI * 2f) * 0.035f * pulse;
            float air = Mathf.Sin(t * (midFreq * 2f) * Mathf.PI * 2f + Mathf.Sin(t * 3f) * 0.5f) * 0.015f;
            data[i] = (bass + mid + air) * 0.9f;
        }

        AudioClip clip = AudioClip.Create($"ProceduralBattleBgm_{variant}", sampleCount, 1, sampleRate, false);
        clip.SetData(data, 0);
        return clip;
    }
}
