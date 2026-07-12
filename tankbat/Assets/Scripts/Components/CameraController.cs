using UnityEngine;
using System.Collections;
using TankBattle;

public class CameraController : MonoBehaviour
{
    private const float FactionSpawnMarginNative = 80f;
    private const float FactionSpawnSpreadNative = 140f;
    private const float NativeMapSize = 1000f;
    private static readonly UnityEngine.Vector2 DefaultFactionCornerViewport = new UnityEngine.Vector2(0.78f, 0.22f);
    [Header("目标设置")]
    [SerializeField] private Transform target;
    [Tooltip("初始参考偏移；实际距离由 currentDistance / currentHeight 控制")]
    [SerializeField] private Vector3 offset = new Vector3(0, 34, -12);
    
    [Header("跟随设置")]
    [SerializeField] private float smoothTime = 0.3f;
    [SerializeField] private float rotationSpeed = 5f;
    
    [Header("镜头控制")]
    [SerializeField] private float fieldOfView = 68f;
    [SerializeField] private float minDistance = 14f;
    [SerializeField] private float maxDistance = 48f;
    [SerializeField] private float zoomSpeed = 3f;
    [SerializeField] private float rotationSensitivity = 1f;
    [SerializeField] private float lookAtHeightOffset = 0.5f;
    [SerializeField] private UnityEngine.Vector2 factionCornerViewport = DefaultFactionCornerViewport;
    
    [Header("边界限制")]
    [SerializeField] private UnityEngine.Vector2 mapBounds = new UnityEngine.Vector2(50, 50);
    [SerializeField] private float cameraHeightLimit = 58f;
    
    // 状态
    private Vector3 velocity = Vector3.zero;
    [SerializeField] private float currentDistance = 22f;
    private float currentRotation = 0f;
    [SerializeField] private float currentHeight = 34f;
    private UnityEngine.Vector2 lookAtPlanarOffset = UnityEngine.Vector2.zero;
    private float lookAtVerticalOffset;
    private bool usePlanarLookOffset;
    
    public Transform Target 
    { 
        get => target; 
        set
        {
            target = value;
            if (target != null)
            {
                SnapToTarget();
            }
        }
    }

    /// <summary>开战构图前绑定目标，避免先用默认朝向 Snap 到错误方位。</summary>
    public void AssignBattleTarget(Transform playerTarget)
    {
        target = playerTarget;
    }
    
    private void Start()
    {
        Camera cam = GetComponent<Camera>();
        if (cam != null)
            cam.fieldOfView = fieldOfView;

        if (currentDistance < minDistance)
            currentDistance = offset.magnitude > 0.01f ? new Vector3(offset.x, 0f, offset.z).magnitude : 22f;
        if (currentHeight < 5f)
            currentHeight = offset.y > 5f ? offset.y : 34f;

        if (target != null)
            SnapToTarget();
    }
    
    private void LateUpdate()
    {
        if (target == null) return;
        
        // 处理输入
        HandleInput();
        
        // 计算目标位置
        Vector3 targetPosition = target.position;
        
        // 计算相机偏移
        Vector3 offsetDirection = ComputeOffsetDirection();
        
        Vector3 desiredPosition = targetPosition + offsetDirection;
        
        // 地图在 Unity 中为 0..mapBounds 正象限
        desiredPosition.x = Mathf.Clamp(desiredPosition.x, 0f, mapBounds.x);
        desiredPosition.z = Mathf.Clamp(desiredPosition.z, 0f, mapBounds.y);
        desiredPosition.y = Mathf.Clamp(desiredPosition.y, 5f, cameraHeightLimit);
        
        // 平滑移动
        transform.position = Vector3.SmoothDamp(transform.position, desiredPosition, ref velocity, smoothTime);
        
        // 朝向目标（较低注视点 → 更俯视）
        transform.LookAt(GetLookAtPoint());
    }
    
    private Vector3 GetLookAtPoint()
    {
        if (target == null)
            return transform.position + transform.forward;

        Vector3 lookAt = target.position + Vector3.up * (lookAtHeightOffset + (usePlanarLookOffset ? lookAtVerticalOffset : 0f));
        if (usePlanarLookOffset)
            lookAt += new Vector3(lookAtPlanarOffset.x, 0f, lookAtPlanarOffset.y);
        return lookAt;
    }

    private Vector3 ComputeOffsetDirection()
    {
        return new Vector3(
            Mathf.Sin(currentRotation * Mathf.Deg2Rad) * currentDistance,
            currentHeight,
            Mathf.Cos(currentRotation * Mathf.Deg2Rad) * currentDistance
        );
    }

    private void ApplyCameraImmediate()
    {
        if (target == null) return;
        transform.position = target.position + ComputeOffsetDirection();
        transform.LookAt(GetLookAtPoint());
    }
    
    private void HandleInput()
    {
        // 缩放
        float scroll = Input.GetAxis("Mouse ScrollWheel");
        if (Mathf.Abs(scroll) > 0.01f)
        {
            currentDistance = Mathf.Clamp(currentDistance - scroll * zoomSpeed, minDistance, maxDistance);
            currentHeight = Mathf.Clamp(currentHeight - scroll * zoomSpeed * 0.65f, 12f, cameraHeightLimit);
        }
        
        // 旋转
        if (Input.GetMouseButton(1)) // 右键旋转
        {
            float mouseX = Input.GetAxis("Mouse X");
            currentRotation += mouseX * rotationSensitivity;
        }
        
        // 切换目标
        if (Input.GetKeyDown(KeyCode.Tab))
        {
            CycleTargets();
        }
    }
    
    private void SnapToTarget()
    {
        ApplyCameraImmediate();
    }
    
    private void CycleTargets()
    {
        TankController[] tanks = FindObjectsOfType<TankController>();
        if (tanks.Length == 0) return;
        
        int currentIndex = -1;
        for (int i = 0; i < tanks.Length; i++)
        {
            if (tanks[i].gameObject == target.gameObject)
            {
                currentIndex = i;
                break;
            }
        }
        
        int nextIndex = (currentIndex + 1) % tanks.Length;
        Target = tanks[nextIndex].transform;
    }
    
    // 设置地图边界
    public void SetMapBounds(UnityEngine.Vector2 bounds)
    {
        mapBounds = bounds;
    }

    /// <summary>开战时应用默认战场视角（覆盖场景中旧序列化值）。</summary>
    public void ConfigureBattleView(
        float? fov = null,
        float? distance = null,
        float? height = null)
    {
        fieldOfView = fov ?? 68f;
        currentDistance = distance ?? 22f;
        currentHeight = height ?? 34f;

        Camera cam = GetComponent<Camera>();
        if (cam != null)
            cam.fieldOfView = fieldOfView;
    }

    public static Vector3 GetFactionSpawnCenterUnity(Faction faction, float worldDisplayScale)
    {
        float cx;
        float cz;
        switch (faction)
        {
            case Faction.Soviet:
                cx = FactionSpawnMarginNative + FactionSpawnSpreadNative * 0.5f;
                cz = FactionSpawnMarginNative + FactionSpawnSpreadNative * 0.5f;
                break;
            case Faction.USA:
                cx = NativeMapSize - FactionSpawnMarginNative - FactionSpawnSpreadNative * 0.5f;
                cz = FactionSpawnMarginNative + FactionSpawnSpreadNative * 0.5f;
                break;
            case Faction.Germany:
                cx = FactionSpawnMarginNative + FactionSpawnSpreadNative * 0.5f;
                cz = NativeMapSize - FactionSpawnMarginNative - FactionSpawnSpreadNative * 0.5f;
                break;
            case Faction.Italy:
                cx = NativeMapSize - FactionSpawnMarginNative - FactionSpawnSpreadNative * 0.5f;
                cz = NativeMapSize - FactionSpawnMarginNative - FactionSpawnSpreadNative * 0.5f;
                break;
            default:
                cx = NativeMapSize * 0.5f;
                cz = NativeMapSize * 0.5f;
                break;
        }

        return new Vector3(cx * worldDisplayScale, 0f, cz * worldDisplayScale);
    }

    /// <summary>
    /// 相机绕玩家旋转角：offset = (sin(r)*d, h, cos(r)*d)。
    /// 315° = 相机在玩家西北侧 → 玩家在画面右下（苏/美出生点对称，须用同一角度）。
    /// </summary>
    private static float GetBattleOrbitRotation(Faction faction)
    {
        switch (faction)
        {
            case Faction.Germany:
                return 0f;
            case Faction.Italy:
                return 90f;
            case Faction.Soviet:
                return 270f;
            case Faction.USA:
                return 180f;
            default:
                return 315f;
        }
    }

    private bool TryGetPlayerViewport(Camera cam, out Vector3 viewport)
    {
        viewport = cam.WorldToViewportPoint(target.position);
        return viewport.z > 0f;
    }

    private void RefinePlayerToViewport(Camera cam, UnityEngine.Vector2 desiredViewport, int iterations)
    {
        float step = Mathf.Max(4f, currentDistance * 0.18f);
        float maxPlanarOffset = Mathf.Max(14f, currentDistance * 0.55f);

        for (int i = 0; i < iterations; i++)
        {
            ApplyCameraImmediate();
            if (!TryGetPlayerViewport(cam, out Vector3 viewport))
                return;

            float errX = desiredViewport.x - viewport.x;
            float errY = desiredViewport.y - viewport.y;
            if (Mathf.Abs(errX) < 0.015f && Mathf.Abs(errY) < 0.015f)
                return;

            Vector3 lookAt = GetLookAtPoint();
            lookAt += cam.transform.right * (-errX * step);
            lookAt += cam.transform.up * (-errY * step);

            lookAtVerticalOffset = lookAt.y - target.position.y - lookAtHeightOffset;
            lookAtPlanarOffset = new UnityEngine.Vector2(
                lookAt.x - target.position.x,
                lookAt.z - target.position.z);

            lookAtPlanarOffset = UnityEngine.Vector2.ClampMagnitude(lookAtPlanarOffset, maxPlanarOffset);
            lookAtVerticalOffset = Mathf.Clamp(lookAtVerticalOffset, -8f, 8f);
        }
    }

    /// <summary>开战时将玩家坦克调整到画面右下方。</summary>
    public void FramePlayerFactionAtCorner(Faction faction, float worldDisplayScale)
    {
        if (target == null)
            return;

        ConfigureBattleView();
        usePlanarLookOffset = true;

        Camera cam = GetComponent<Camera>();
        if (cam == null)
            return;

        currentRotation = GetBattleOrbitRotation(faction);
        lookAtPlanarOffset = UnityEngine.Vector2.zero;
        lookAtVerticalOffset = 0f;
        RefinePlayerToViewport(cam, factionCornerViewport, 16);

        ApplyCameraImmediate();
        if (TryGetPlayerViewport(cam, out Vector3 viewport) &&
            (viewport.y > 0.35f || viewport.x < 0.55f))
        {
            // 微调发散时回退：仅保留 orbit 角，苏/美与苏联共用 315°
            lookAtPlanarOffset = UnityEngine.Vector2.zero;
            lookAtVerticalOffset = 0f;
            ApplyCameraImmediate();
        }

        velocity = Vector3.zero;
        ApplyCameraImmediate();
    }
    
    // 调试绘制
    private void OnDrawGizmos()
    {
        if (target != null)
        {
            Gizmos.color = Color.yellow;
            Gizmos.DrawLine(transform.position, target.position);
            Gizmos.DrawWireSphere(target.position, 1f);
        }
        
        // 绘制地图边界
        Gizmos.color = Color.white;
        Gizmos.DrawWireCube(Vector3.zero, new Vector3(mapBounds.x * 2, 0, mapBounds.y * 2));
    }
}