using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// 战场 BGM 播放列表（随机切换）+ 距离衰减开炮音效。
/// </summary>
[DisallowMultipleComponent]
public class BattleAudioManager : MonoBehaviour
{
    private AudioClip[] backgroundMusicTracks;
    private AudioClip gunfireClip;
    private float bgmVolume = 0.32f;
    private float bgmFadeInSeconds = 1.5f;
    private float bgmCrossfadeSeconds = 1.5f;
    private float gunfireVolume = 0.55f;
    private float playerGunfireVolume = 0.75f;
    private float gunfireMinDistance = 2f;
    private float gunfireMaxDistance = 90f;
    private float gunfireMinInterval = 0.07f;
    private float gunfireFarVolumeFloor = 0.04f;

    private AudioSource bgmSource;
    private readonly Dictionary<uint, float> lastGunfireTimeByTank = new Dictionary<uint, float>();
    private readonly List<AudioClip> resolvedBgmTracks = new List<AudioClip>();
    private Coroutine bgmPlaylistCoroutine;
    private int lastBgmTrackIndex = -1;
    private bool bgmPlaying;

    public void Configure(
        AudioClip[] bgmTracks,
        AudioClip gunfire,
        float bgmVol,
        float gunfireVol,
        float playerGunfireVol,
        float gunfireMinDist,
        float gunfireMaxDist,
        float gunfireMinGap,
        float gunfireFarFloor,
        float bgmFadeIn,
        float bgmCrossfade)
    {
        backgroundMusicTracks = bgmTracks;
        gunfireClip = gunfire;
        bgmVolume = bgmVol;
        gunfireVolume = gunfireVol;
        playerGunfireVolume = playerGunfireVol;
        gunfireMinDistance = gunfireMinDist;
        gunfireMaxDistance = gunfireMaxDist;
        gunfireMinInterval = gunfireMinGap;
        gunfireFarVolumeFloor = gunfireFarFloor;
        bgmFadeInSeconds = bgmFadeIn;
        bgmCrossfadeSeconds = bgmCrossfade;
        RebuildBgmTrackList();
        EnsureClips();
        EnsureBgmSource();
    }

    private void Awake()
    {
        RebuildBgmTrackList();
        EnsureClips();
        EnsureBgmSource();
    }

    public void PlayBattleMusic()
    {
        EnsureClips();
        EnsureBgmSource();
        if (resolvedBgmTracks.Count == 0 || bgmSource == null)
            return;

        bgmPlaying = true;
        StopBgmPlaylist();
        bgmPlaylistCoroutine = StartCoroutine(RunBgmPlaylist());
    }

    public void StopBattleMusic(float fadeOutSeconds = 1f)
    {
        bgmPlaying = false;
        StopBgmPlaylist();

        if (bgmSource == null || !bgmSource.isPlaying)
            return;

        if (fadeOutSeconds <= 0.01f)
        {
            bgmSource.Stop();
            return;
        }

        StartCoroutine(FadeOutAndStopBgm(fadeOutSeconds));
    }

    public void PlayGunfireAt(Vector3 worldPosition, uint ownerTankId, bool isLocalPlayerTank)
    {
        EnsureClips();
        if (gunfireClip == null)
            return;

        if (ownerTankId != 0)
        {
            if (lastGunfireTimeByTank.TryGetValue(ownerTankId, out float lastTime) &&
                Time.time - lastTime < gunfireMinInterval)
            {
                return;
            }
            lastGunfireTimeByTank[ownerTankId] = Time.time;
        }

        float volumeScale = isLocalPlayerTank ? playerGunfireVolume : gunfireVolume;
        volumeScale *= ComputeDistanceVolumeScale(worldPosition);

        GameObject oneShot = new GameObject("GunfireAudio");
        oneShot.transform.position = worldPosition;
        AudioSource source = oneShot.AddComponent<AudioSource>();
        source.clip = gunfireClip;
        source.spatialBlend = 0f;
        source.volume = volumeScale;
        source.dopplerLevel = 0f;
        source.Play();
        Destroy(oneShot, gunfireClip.length + 0.08f);
    }

    public void ClearGunfireHistory()
    {
        lastGunfireTimeByTank.Clear();
    }

    private IEnumerator RunBgmPlaylist()
    {
        bgmSource.loop = resolvedBgmTracks.Count == 1;

        while (bgmPlaying)
        {
            AudioClip track = PickRandomBgmTrack();
            if (track == null)
                yield break;

            bgmSource.clip = track;
            bgmSource.volume = 0f;
            if (!bgmSource.isPlaying)
                bgmSource.Play();

            yield return FadeVolume(0f, bgmVolume, bgmFadeInSeconds);

            if (resolvedBgmTracks.Count == 1)
            {
                while (bgmPlaying)
                    yield return null;
                yield break;
            }

            float holdSeconds = track.length - bgmCrossfadeSeconds - bgmFadeInSeconds;
            if (holdSeconds > 0f)
                yield return new WaitForSeconds(holdSeconds);

            if (!bgmPlaying)
                yield break;

            yield return FadeVolume(bgmSource.volume, 0f, bgmCrossfadeSeconds);
            bgmSource.Stop();
        }
    }

    private AudioClip PickRandomBgmTrack()
    {
        if (resolvedBgmTracks.Count == 0)
            return null;
        if (resolvedBgmTracks.Count == 1)
            return resolvedBgmTracks[0];

        int index = Random.Range(0, resolvedBgmTracks.Count);
        if (resolvedBgmTracks.Count > 1)
        {
            int guard = 0;
            while (index == lastBgmTrackIndex && guard < 8)
            {
                index = Random.Range(0, resolvedBgmTracks.Count);
                guard++;
            }
        }

        lastBgmTrackIndex = index;
        return resolvedBgmTracks[index];
    }

    private void RebuildBgmTrackList()
    {
        resolvedBgmTracks.Clear();
        if (backgroundMusicTracks != null)
        {
            foreach (AudioClip clip in backgroundMusicTracks)
            {
                if (clip != null && !resolvedBgmTracks.Contains(clip))
                    resolvedBgmTracks.Add(clip);
            }
        }
    }

    private void StopBgmPlaylist()
    {
        if (bgmPlaylistCoroutine != null)
        {
            StopCoroutine(bgmPlaylistCoroutine);
            bgmPlaylistCoroutine = null;
        }
    }

    private IEnumerator FadeVolume(float from, float to, float duration)
    {
        if (bgmSource == null)
            yield break;

        if (duration <= 0.01f)
        {
            bgmSource.volume = to;
            yield break;
        }

        float elapsed = 0f;
        while (elapsed < duration && bgmPlaying && bgmSource != null)
        {
            elapsed += Time.deltaTime;
            bgmSource.volume = Mathf.Lerp(from, to, elapsed / duration);
            yield return null;
        }

        if (bgmSource != null)
            bgmSource.volume = to;
    }

    private float ComputeDistanceVolumeScale(Vector3 worldPosition)
    {
        Camera listenerCamera = Camera.main;
        if (listenerCamera == null)
            return 1f;

        float dx = listenerCamera.transform.position.x - worldPosition.x;
        float dz = listenerCamera.transform.position.z - worldPosition.z;
        float distance = Mathf.Sqrt(dx * dx + dz * dz);
        if (distance <= gunfireMinDistance)
            return 1f;
        if (distance >= gunfireMaxDistance)
            return gunfireFarVolumeFloor;

        float t = (distance - gunfireMinDistance) / (gunfireMaxDistance - gunfireMinDistance);
        return Mathf.Lerp(1f, gunfireFarVolumeFloor, t);
    }

    private void EnsureClips()
    {
        if (gunfireClip == null)
            gunfireClip = ProceduralAudioClipFactory.CreateGunfireClip();

        if (resolvedBgmTracks.Count == 0)
        {
            resolvedBgmTracks.Add(ProceduralAudioClipFactory.CreateBattleMusicClip(0));
            resolvedBgmTracks.Add(ProceduralAudioClipFactory.CreateBattleMusicClip(1));
        }
    }

    private void EnsureBgmSource()
    {
        if (bgmSource != null)
            return;

        bgmSource = gameObject.AddComponent<AudioSource>();
        bgmSource.loop = false;
        bgmSource.playOnAwake = false;
        bgmSource.spatialBlend = 0f;
        bgmSource.volume = 0f;
    }

    private IEnumerator FadeOutAndStopBgm(float duration)
    {
        yield return FadeVolume(bgmSource != null ? bgmSource.volume : 0f, 0f, duration);

        if (bgmSource != null)
            bgmSource.Stop();
    }
}
