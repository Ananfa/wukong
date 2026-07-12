using System;
using TankBattle;
using UnityEditor;
using UnityEngine;

public class MapEditorWindow : EditorWindow
{
    private enum EditMode
    {
        Walls,
        SpawnZones
    }

    private MapConfigData config;
    private string assetPath = MapConfigJsonIO.DefaultAssetPath;
    private float displayScale = 0.22f;
    private EditMode editMode = EditMode.Walls;
    private int selectedWallIndex = -1;
    private Faction selectedSpawnFaction = Faction.Soviet;
    private bool showGrid = true;
    private bool showLabels = true;
    private UnityEngine.Vector2 scroll;

    [MenuItem("Tools/坦克大战/地图编辑器")]
    public static void Open()
    {
        var window = GetWindow<MapEditorWindow>("地图编辑器");
        window.minSize = new UnityEngine.Vector2(360f, 420f);
        window.Show();
    }

    private void OnEnable()
    {
        SceneView.duringSceneGui += OnSceneGUI;
        if (config == null)
            ReloadFromDisk();
    }

    private void OnDisable()
    {
        SceneView.duringSceneGui -= OnSceneGUI;
    }

    private void OnGUI()
    {
        EditorGUILayout.LabelField("地图编辑器", EditorStyles.boldLabel);
        EditorGUILayout.HelpBox(
            "Native 坐标系：X/Y 为 0..worldWidth/Height。Scene 视图可拖拽墙中心、旋转，或拖出生区对角。保存后写入 MapObstacles.json，C++ GameCore 开局加载。",
            UnityEditor.MessageType.Info);

        assetPath = EditorGUILayout.TextField("配置文件", assetPath);
        displayScale = EditorGUILayout.Slider("Scene 显示缩放", displayScale, 0.06f, 1f);

        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button("重新加载"))
            ReloadFromDisk();
        if (GUILayout.Button("保存"))
            SaveToDisk();
        if (GUILayout.Button("恢复默认"))
            ResetToDefault();
        if (GUILayout.Button("聚焦 Scene"))
            FrameSceneView();
        EditorGUILayout.EndHorizontal();

        editMode = (EditMode)GUILayout.Toolbar((int)editMode, new[] { "阻挡墙", "出生区域" });

        if (config == null)
        {
            EditorGUILayout.HelpBox("未加载配置。", UnityEditor.MessageType.Warning);
            return;
        }

        config.worldWidth = EditorGUILayout.FloatField("World Width", config.worldWidth);
        config.worldHeight = EditorGUILayout.FloatField("World Height", config.worldHeight);
        showGrid = EditorGUILayout.Toggle("Scene 显示网格", showGrid);
        showLabels = EditorGUILayout.Toggle("Scene 显示标签", showLabels);

        scroll = EditorGUILayout.BeginScrollView(scroll);

        if (editMode == EditMode.Walls)
            DrawWallPanel();
        else
            DrawSpawnPanel();

        EditorGUILayout.EndScrollView();

        if (GUI.changed)
            SceneView.RepaintAll();
    }

    private void DrawWallPanel()
    {
        EditorGUILayout.LabelField("阻挡墙", EditorStyles.boldLabel);
        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button("添加墙"))
        {
            config.walls.Add(new MapWallData
            {
                x = config.worldWidth * 0.5f,
                y = config.worldHeight * 0.5f,
                width = 120f,
                height = 18f
            });
            selectedWallIndex = config.walls.Count - 1;
        }
        using (new EditorGUI.DisabledScope(selectedWallIndex < 0 || selectedWallIndex >= config.walls.Count))
        {
            if (GUILayout.Button("删除选中", GUILayout.Width(90f)))
            {
                config.walls.RemoveAt(selectedWallIndex);
                selectedWallIndex = Mathf.Clamp(selectedWallIndex, -1, config.walls.Count - 1);
            }
        }
        EditorGUILayout.EndHorizontal();

        for (int i = 0; i < config.walls.Count; i++)
        {
            MapWallData wall = config.walls[i];
            bool selected = i == selectedWallIndex;
            EditorGUILayout.BeginVertical(selected ? "SelectionRect" : "box");
            EditorGUILayout.BeginHorizontal();
            if (GUILayout.Toggle(selected, $"墙 #{i + 1}", "Button"))
                selectedWallIndex = i;
            EditorGUILayout.EndHorizontal();

            if (selected)
            {
                wall.x = EditorGUILayout.FloatField("Center X", wall.x);
                wall.y = EditorGUILayout.FloatField("Center Y", wall.y);
                wall.width = EditorGUILayout.FloatField("Width", wall.width);
                wall.height = EditorGUILayout.FloatField("Height", wall.height);
                wall.rotation = EditorGUILayout.FloatField("Rotation (rad)", wall.rotation);
                wall.rotation = EditorGUILayout.Slider("Rotation (deg)", wall.rotation * Mathf.Rad2Deg, -180f, 180f) * Mathf.Deg2Rad;
            }
            EditorGUILayout.EndVertical();
        }
    }

    private void DrawSpawnPanel()
    {
        EditorGUILayout.LabelField("阵营出生/复活矩形", EditorStyles.boldLabel);
        selectedSpawnFaction = (Faction)EditorGUILayout.EnumPopup("当前编辑阵营", selectedSpawnFaction);

        MapSpawnZoneData zone = config.GetSpawnZone(selectedSpawnFaction);
        zone.faction = FactionToKey(selectedSpawnFaction);
        zone.minX = EditorGUILayout.FloatField("Min X", zone.minX);
        zone.maxX = EditorGUILayout.FloatField("Max X", zone.maxX);
        zone.minY = EditorGUILayout.FloatField("Min Y", zone.minY);
        zone.maxY = EditorGUILayout.FloatField("Max Y", zone.maxY);

        if (zone.maxX < zone.minX)
        {
            float tmp = zone.minX;
            zone.minX = zone.maxX;
            zone.maxX = tmp;
        }
        if (zone.maxY < zone.minY)
        {
            float tmp = zone.minY;
            zone.minY = zone.maxY;
            zone.maxY = tmp;
        }

        EditorGUILayout.LabelField("尺寸",
            $"{zone.maxX - zone.minX:0.#} × {zone.maxY - zone.minY:0.#} Native");
    }

    private void OnSceneGUI(SceneView sceneView)
    {
        if (config == null)
            return;

        DrawMapBounds();
        if (showGrid)
            DrawNativeGrid();

        if (editMode == EditMode.SpawnZones)
        {
            DrawAllSpawnZones();
            DrawSpawnZoneHandles(config.GetSpawnZone(selectedSpawnFaction));
        }
        else
        {
            DrawAllWalls();
            DrawSelectedWallHandles();
        }

        HandleUtility.AddDefaultControl(GUIUtility.GetControlID(FocusType.Passive));
    }

    private void DrawMapBounds()
    {
        Handles.color = new Color(0.2f, 0.8f, 0.2f, 0.9f);
        Vector3 a = NativeToScene(0f, 0f);
        Vector3 b = NativeToScene(config.worldWidth, 0f);
        Vector3 c = NativeToScene(config.worldWidth, config.worldHeight);
        Vector3 d = NativeToScene(0f, config.worldHeight);
        Handles.DrawLine(a, b);
        Handles.DrawLine(b, c);
        Handles.DrawLine(c, d);
        Handles.DrawLine(d, a);
    }

    private void DrawNativeGrid()
    {
        Handles.color = new Color(1f, 1f, 1f, 0.08f);
        const float step = 50f;
        for (float x = 0f; x <= config.worldWidth; x += step)
        {
            Handles.DrawLine(
                NativeToScene(x, 0f),
                NativeToScene(x, config.worldHeight));
        }
        for (float y = 0f; y <= config.worldHeight; y += step)
        {
            Handles.DrawLine(
                NativeToScene(0f, y),
                NativeToScene(config.worldWidth, y));
        }
    }

    private void DrawAllWalls()
    {
        for (int i = 0; i < config.walls.Count; i++)
        {
            MapWallData wall = config.walls[i];
            Color color = i == selectedWallIndex
                ? new Color(1f, 0.85f, 0.2f, 0.95f)
                : new Color(0.55f, 0.55f, 0.58f, 0.9f);
            DrawWall(wall, color, showLabels ? $"墙{i + 1}" : null);
        }
    }

    private void DrawSelectedWallHandles()
    {
        if (selectedWallIndex < 0 || selectedWallIndex >= config.walls.Count)
            return;

        MapWallData wall = config.walls[selectedWallIndex];
        Vector3 center = NativeToScene(wall.x, wall.y);
        float handleSize = HandleUtility.GetHandleSize(center) * 0.08f;

        EditorGUI.BeginChangeCheck();
        Vector3 moved = Handles.PositionHandle(center, Quaternion.Euler(0f, -wall.rotation * Mathf.Rad2Deg, 0f));
        if (EditorGUI.EndChangeCheck())
        {
            UnityEngine.Vector2 native = SceneToNative(moved);
            wall.x = native.x;
            wall.y = native.y;
        }

        Quaternion rot = Handles.RotationHandle(
            Quaternion.Euler(0f, -wall.rotation * Mathf.Rad2Deg, 0f),
            center);
        wall.rotation = -rot.eulerAngles.y * Mathf.Deg2Rad;

        DrawWallResizeHandles(wall, center, handleSize);
    }

    private void DrawWallResizeHandles(MapWallData wall, Vector3 center, float handleSize)
    {
        Quaternion rotation = Quaternion.Euler(0f, -wall.rotation * Mathf.Rad2Deg, 0f);
        Vector3 right = rotation * Vector3.right;
        Vector3 forward = rotation * Vector3.forward;

        float halfW = wall.width * displayScale * 0.5f;
        float halfH = wall.height * displayScale * 0.5f;

        Vector3 widthHandlePos = center + right * halfW;
        Vector3 heightHandlePos = center + forward * halfH;

        EditorGUI.BeginChangeCheck();
        Vector3 newWidthPos = Handles.FreeMoveHandle(
            widthHandlePos,
            handleSize,
            Vector3.zero,
            Handles.DotHandleCap);
        if (EditorGUI.EndChangeCheck())
        {
            float projected = Vector3.Dot(newWidthPos - center, right);
            wall.width = Mathf.Max(4f, projected * 2f / displayScale);
        }

        EditorGUI.BeginChangeCheck();
        Vector3 newHeightPos = Handles.FreeMoveHandle(
            heightHandlePos,
            handleSize,
            Vector3.zero,
            Handles.DotHandleCap);
        if (EditorGUI.EndChangeCheck())
        {
            float projected = Vector3.Dot(newHeightPos - center, forward);
            wall.height = Mathf.Max(4f, projected * 2f / displayScale);
        }
    }

    private void DrawWall(MapWallData wall, Color color, string label)
    {
        Vector3 center = NativeToScene(wall.x, wall.y);
        Quaternion rotation = Quaternion.Euler(0f, -wall.rotation * Mathf.Rad2Deg, 0f);
        Vector3 size = new Vector3(
            wall.width * displayScale,
            0.05f,
            wall.height * displayScale);

        Handles.color = new Color(color.r, color.g, color.b, 0.18f);
        Handles.matrix = Matrix4x4.TRS(center, rotation, Vector3.one);
        Handles.DrawSolidRectangleWithOutline(
            new[]
            {
                new Vector3(-size.x * 0.5f, 0f, -size.z * 0.5f),
                new Vector3(size.x * 0.5f, 0f, -size.z * 0.5f),
                new Vector3(size.x * 0.5f, 0f, size.z * 0.5f),
                new Vector3(-size.x * 0.5f, 0f, size.z * 0.5f)
            },
            new Color(color.r, color.g, color.b, 0.15f),
            color);
        Handles.matrix = Matrix4x4.identity;

        if (!string.IsNullOrEmpty(label))
            Handles.Label(center + Vector3.up * 0.2f, label);
    }

    private void DrawAllSpawnZones()
    {
        foreach (MapSpawnZoneData zone in config.spawnZones)
        {
            Faction faction = zone.ToFaction();
            Color color = TankController.GetFactionColor(faction);
            bool selected = faction == selectedSpawnFaction;
            color.a = selected ? 0.35f : 0.18f;
            DrawSpawnRect(zone, color, selected ? FactionLabel(faction) + " (编辑中)" : FactionLabel(faction));
        }
    }

    private void DrawSpawnZoneHandles(MapSpawnZoneData zone)
    {
        Vector3 min = NativeToScene(zone.minX, zone.minY);
        Vector3 max = NativeToScene(zone.maxX, zone.maxY);
        float handleSize = HandleUtility.GetHandleSize(min) * 0.06f;

        EditorGUI.BeginChangeCheck();
        Vector3 movedMin = Handles.FreeMoveHandle(min, handleSize, Vector3.zero, Handles.RectangleHandleCap);
        Vector3 movedMax = Handles.FreeMoveHandle(max, handleSize, Vector3.zero, Handles.RectangleHandleCap);
        if (EditorGUI.EndChangeCheck())
        {
            UnityEngine.Vector2 nativeMin = SceneToNative(movedMin);
            UnityEngine.Vector2 nativeMax = SceneToNative(movedMax);
            zone.minX = Mathf.Min(nativeMin.x, nativeMax.x);
            zone.maxX = Mathf.Max(nativeMin.x, nativeMax.x);
            zone.minY = Mathf.Min(nativeMin.y, nativeMax.y);
            zone.maxY = Mathf.Max(nativeMin.y, nativeMax.y);
            Repaint();
        }
    }

    private void DrawSpawnRect(MapSpawnZoneData zone, Color fillColor, string label)
    {
        Vector3 min = NativeToScene(zone.minX, zone.minY);
        Vector3 max = NativeToScene(zone.maxX, zone.maxY);
        Vector3 center = (min + max) * 0.5f;
        Vector3 size = new Vector3(Mathf.Abs(max.x - min.x), 0.02f, Mathf.Abs(max.z - min.z));

        Handles.color = fillColor;
        Handles.DrawSolidRectangleWithOutline(
            new[]
            {
                new Vector3(min.x, 0.01f, min.z),
                new Vector3(max.x, 0.01f, min.z),
                new Vector3(max.x, 0.01f, max.z),
                new Vector3(min.x, 0.01f, max.z)
            },
            fillColor,
            new Color(fillColor.r, fillColor.g, fillColor.b, 0.95f));

        if (showLabels)
            Handles.Label(center + Vector3.up * 0.15f, label);
    }

    private Vector3 NativeToScene(float nativeX, float nativeY)
    {
        return new Vector3(nativeX * displayScale, 0f, nativeY * displayScale);
    }

    private UnityEngine.Vector2 SceneToNative(Vector3 scenePos)
    {
        if (Mathf.Abs(displayScale) < 0.0001f)
            return UnityEngine.Vector2.zero;
        return new UnityEngine.Vector2(scenePos.x / displayScale, scenePos.z / displayScale);
    }

    private void ReloadFromDisk()
    {
        config = MapConfigJsonIO.LoadFromAssetPath(assetPath);
        selectedWallIndex = config.walls.Count > 0 ? 0 : -1;
        SceneView.RepaintAll();
        Repaint();
    }

    private void SaveToDisk()
    {
        if (config == null)
            return;
        if (MapConfigJsonIO.SaveToAssetPath(config, assetPath))
            Debug.Log($"地图配置已保存: {assetPath}");
        else
            Debug.LogError($"保存失败: {assetPath}");
    }

    private void ResetToDefault()
    {
        config = MapConfigData.CreateDefault();
        config.worldWidth = 1000f;
        config.worldHeight = 1000f;
        selectedWallIndex = config.walls.Count > 0 ? 0 : -1;
        SceneView.RepaintAll();
        Repaint();
    }

    private void FrameSceneView()
    {
        if (config == null)
            return;
        SceneView sceneView = SceneView.lastActiveSceneView;
        if (sceneView == null)
            return;

        Vector3 center = NativeToScene(config.worldWidth * 0.5f, config.worldHeight * 0.5f);
        float size = Mathf.Max(config.worldWidth, config.worldHeight) * displayScale * 0.6f;
        sceneView.LookAt(center, Quaternion.Euler(45f, 45f, 0f), size);
        sceneView.Repaint();
    }

    private static string FactionToKey(Faction faction)
    {
        switch (faction)
        {
        case Faction.USA: return "usa";
        case Faction.Germany: return "germany";
        case Faction.Italy: return "italy";
        default: return "soviet";
        }
    }

    private static string FactionLabel(Faction faction)
    {
        switch (faction)
        {
        case Faction.USA: return "美国 SE";
        case Faction.Germany: return "德国 NW";
        case Faction.Italy: return "意大利 NE";
        default: return "苏联 SW";
        }
    }
}
