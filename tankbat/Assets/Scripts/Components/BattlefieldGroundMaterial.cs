using UnityEngine;

/// <summary>
/// 运行时生成带网格与噪点的战场地面材质，便于观察坦克位移。
/// </summary>
public static class BattlefieldGroundMaterial
{
    public static Material Create(
        float nativeMapWidth,
        float nativeMapHeight,
        float gridSpacingNative = 50f)
    {
        Texture2D texture = CreateGroundTexture(
            1024,
            1024,
            nativeMapWidth,
            nativeMapHeight,
            gridSpacingNative);

        Shader shader = Shader.Find("Standard");
        if (shader == null)
            shader = Shader.Find("Universal Render Pipeline/Lit");
        if (shader == null)
            shader = Shader.Find("Unlit/Texture");
        if (shader == null)
            shader = Shader.Find("Unlit/Color");

        Material material = new Material(shader);
        material.name = "BattlefieldGroundRuntime";
        material.mainTexture = texture;

        if (material.HasProperty("_Glossiness"))
            material.SetFloat("_Glossiness", 0.04f);
        if (material.HasProperty("_Metallic"))
            material.SetFloat("_Metallic", 0f);
        if (material.HasProperty("_Smoothness"))
            material.SetFloat("_Smoothness", 0.04f);

        return material;
    }

    private static Texture2D CreateGroundTexture(
        int width,
        int height,
        float nativeMapWidth,
        float nativeMapHeight,
        float gridSpacingNative)
    {
        var texture = new Texture2D(width, height, TextureFormat.RGB24, false)
        {
            name = "BattlefieldGroundTexture",
            wrapMode = TextureWrapMode.Clamp,
            filterMode = FilterMode.Bilinear
        };

        Color baseColor = new Color(0.24f, 0.40f, 0.20f);
        Color patchDark = new Color(0.18f, 0.32f, 0.15f);
        Color patchLight = new Color(0.28f, 0.46f, 0.23f);
        Color majorGrid = new Color(0.12f, 0.22f, 0.10f);
        Color minorGrid = new Color(0.17f, 0.30f, 0.14f);

        float minorSpacingNative = gridSpacingNative * 0.2f;
        float majorSpacingNative = gridSpacingNative * 5f;

        for (int y = 0; y < height; y++)
        {
            float v = y / (float)(height - 1);
            float nativeY = v * nativeMapHeight;

            for (int x = 0; x < width; x++)
            {
                float u = x / (float)(width - 1);
                float nativeX = u * nativeMapWidth;

                float noise = Mathf.PerlinNoise(u * 18f + 0.17f, v * 18f + 0.31f);
                float patch = Mathf.PerlinNoise(u * 4.5f + 2.1f, v * 4.5f + 1.3f);
                Color color = Color.Lerp(patchDark, patchLight, patch);
                color = Color.Lerp(color, baseColor, 0.35f + noise * 0.25f);

                if (IsGridLine(nativeX, minorSpacingNative, width, nativeMapWidth, 1))
                    color = Color.Lerp(color, minorGrid, 0.55f);
                if (IsGridLine(nativeY, minorSpacingNative, height, nativeMapHeight, 1))
                    color = Color.Lerp(color, minorGrid, 0.55f);
                if (IsGridLine(nativeX, gridSpacingNative, width, nativeMapWidth, 2))
                    color = Color.Lerp(color, majorGrid, 0.75f);
                if (IsGridLine(nativeY, gridSpacingNative, height, nativeMapHeight, 2))
                    color = Color.Lerp(color, majorGrid, 0.75f);
                if (IsGridLine(nativeX, majorSpacingNative, width, nativeMapWidth, 3))
                    color = Color.Lerp(color, majorGrid, 0.95f);
                if (IsGridLine(nativeY, majorSpacingNative, height, nativeMapHeight, 3))
                    color = Color.Lerp(color, majorGrid, 0.95f);

                texture.SetPixel(x, y, color);
            }
        }

        texture.Apply();
        return texture;
    }

    private static bool IsGridLine(
        float nativeCoord,
        float spacingNative,
        int texSize,
        float nativeMapSize,
        int lineWidthTexels)
    {
        if (spacingNative <= 0.01f || nativeMapSize <= 0.01f)
            return false;

        float cellNative = spacingNative;
        float phase = nativeCoord / cellNative;
        float distToLine = Mathf.Abs(phase - Mathf.Round(phase)) * cellNative;
        float texelsPerNative = texSize / nativeMapSize;
        return distToLine * texelsPerNative <= lineWidthTexels;
    }
}
