using UnityEngine;

/// <summary>
/// 本地玩家坦克头顶倒三角标识（朝向相机、上下跳动，颜色随阵营）。
/// </summary>
public class PlayerTankTopMarker : MonoBehaviour
{
    [SerializeField] private float heightAboveTank = 4.2f;
    [SerializeField] private float triangleWidth = 0.72f;
    [SerializeField] private float triangleHeight = 0.58f;
    [SerializeField] private float bounceAmplitude = 0.28f;
    [SerializeField] private float bounceSpeed = 3.4f;

    private Transform visualRoot;
    private Renderer fillRenderer;
    private Renderer outlineRenderer;
    private Color baseColor = Color.cyan;
    private float phaseOffset;
    private Camera mainCamera;

    public void ApplyStyle(Color factionColor, float worldHeightAboveTank)
    {
        baseColor = BrightenMarkerColor(factionColor);
        heightAboveTank = Mathf.Max(2f, worldHeightAboveTank);
        EnsureVisuals();
        RefreshColor();
    }

    private void Awake()
    {
        phaseOffset = Random.Range(0f, Mathf.PI * 2f);
        EnsureVisuals();
    }

    private void LateUpdate()
    {
        Transform parent = transform.parent;
        if (parent == null || visualRoot == null)
            return;

        if (mainCamera == null)
            mainCamera = Camera.main;

        float bounce = Mathf.Abs(Mathf.Sin(Time.time * bounceSpeed + phaseOffset)) * bounceAmplitude;
        Vector3 p = parent.position;
        transform.position = new Vector3(p.x, p.y + heightAboveTank + bounce, p.z);

        if (mainCamera != null)
            visualRoot.rotation = mainCamera.transform.rotation;
    }

    private void EnsureVisuals()
    {
        if (visualRoot != null)
            return;

        GameObject root = new GameObject("TriangleVisual");
        root.transform.SetParent(transform, false);
        visualRoot = root.transform;

        MeshFilter fillFilter = root.AddComponent<MeshFilter>();
        fillFilter.sharedMesh = BuildTriangleMesh(triangleWidth, triangleHeight);

        fillRenderer = root.AddComponent<MeshRenderer>();
        fillRenderer.material = CreateTriangleMaterial();
        fillRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
        fillRenderer.receiveShadows = false;

        GameObject outlineGo = new GameObject("TriangleOutline");
        outlineGo.transform.SetParent(visualRoot, false);
        outlineGo.transform.localPosition = new Vector3(0f, 0f, -0.03f);

        MeshFilter outlineFilter = outlineGo.AddComponent<MeshFilter>();
        outlineFilter.sharedMesh = BuildTriangleMesh(triangleWidth * 1.22f, triangleHeight * 1.22f);

        outlineRenderer = outlineGo.AddComponent<MeshRenderer>();
        outlineRenderer.material = CreateOutlineMaterial();
        outlineRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
        outlineRenderer.receiveShadows = false;

        RefreshColor();
    }

    private void RefreshColor()
    {
        if (fillRenderer != null)
            fillRenderer.material.color = baseColor;

        if (outlineRenderer != null)
            outlineRenderer.material.color = new Color(1f, 1f, 1f, 0.95f);
    }

    private static Color BrightenMarkerColor(Color color)
    {
        Color.RGBToHSV(color, out float h, out float s, out float v);
        s = Mathf.Clamp01(Mathf.Max(s * 1.2f, 0.55f));
        v = Mathf.Max(v, 0.95f);
        Color bright = Color.HSVToRGB(h, s, v);
        bright.a = 1f;
        return Color.Lerp(bright, Color.white, 0.18f);
    }

    private static Mesh BuildTriangleMesh(float width, float height)
    {
        float halfWidth = width * 0.5f;
        Mesh mesh = new Mesh { name = "PlayerTankMarkerTriangle" };
        mesh.vertices = new[]
        {
            new Vector3(-halfWidth, height * 0.5f, 0f),
            new Vector3(halfWidth, height * 0.5f, 0f),
            new Vector3(0f, -height * 0.5f, 0f),
        };
        mesh.triangles = new[] { 0, 1, 2 };
        mesh.RecalculateNormals();
        mesh.RecalculateBounds();
        return mesh;
    }

    private static Material CreateTriangleMaterial()
    {
        Material mat = new Material(FindUnlitShader());
        mat.renderQueue = 3200;
        return mat;
    }

    private static Material CreateOutlineMaterial()
    {
        Material mat = new Material(FindUnlitShader());
        mat.renderQueue = 3199;
        return mat;
    }

    private static Shader FindUnlitShader()
    {
        Shader shader = Shader.Find("Unlit/Color");
        if (shader == null)
            shader = Shader.Find("Sprites/Default");
        return shader;
    }
}
