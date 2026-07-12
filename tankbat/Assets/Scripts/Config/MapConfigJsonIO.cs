using System.Globalization;
using System.IO;
using System.Text;
using UnityEngine;

public static class MapConfigJsonIO
{
    public const string DefaultAssetPath = "Assets/Config/MapObstacles.json";
    private const int PosScale = 64;
    private const int AngleUnits = 65536;

    public static MapConfigData LoadFromText(string json)
    {
        if (string.IsNullOrWhiteSpace(json))
            return MapConfigData.CreateDefault();

        MapConfigJsonRoot root = JsonUtility.FromJson<MapConfigJsonRoot>(json);
        if (root == null)
            return MapConfigData.CreateDefault();
        return root.ToRuntimeData();
    }

    public static MapConfigData LoadFromAssetPath(string assetPath)
    {
        TextAsset asset = LoadTextAsset(assetPath);
        if (asset == null)
            return MapConfigData.CreateDefault();
        return LoadFromText(asset.text);
    }

    public static bool SaveToAssetPath(MapConfigData config, string assetPath)
    {
        if (config == null)
            return false;

        string fullPath = ToFullPath(assetPath);
        string directory = Path.GetDirectoryName(fullPath);
        if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
            Directory.CreateDirectory(directory);

        File.WriteAllText(fullPath, SerializePretty(config), Encoding.UTF8);
#if UNITY_EDITOR
        UnityEditor.AssetDatabase.Refresh();
#endif
        return true;
    }

    public static TextAsset LoadTextAsset(string assetPath)
    {
#if UNITY_EDITOR
        return UnityEditor.AssetDatabase.LoadAssetAtPath<TextAsset>(assetPath);
#else
        return Resources.Load<TextAsset>(Path.GetFileNameWithoutExtension(assetPath));
#endif
    }

    public static string SerializePretty(MapConfigData config)
    {
        var sb = new StringBuilder(1024);
        sb.AppendLine("{");
        sb.AppendLine($"  \"worldWidth\": {WorldToPos(config.worldWidth)},");
        sb.AppendLine($"  \"worldHeight\": {WorldToPos(config.worldHeight)},");
        sb.AppendLine("  \"walls\": [");

        for (int i = 0; i < config.walls.Count; i++)
        {
            MapWallData wall = config.walls[i];
            sb.Append("    { ");
            sb.Append($"\"x\": {WorldToPos(wall.x)}, \"y\": {WorldToPos(wall.y)}, ");
            sb.Append($"\"width\": {WorldToPos(wall.width)}, \"height\": {WorldToPos(wall.height)}, ");
            sb.Append($"\"rotation\": {RadiansToAngleUnits(wall.rotation)} ");
            sb.Append(i + 1 < config.walls.Count ? "}," : "}");
            sb.AppendLine();
        }

        sb.AppendLine("  ],");
        sb.AppendLine("  \"spawnZones\": [");

        for (int i = 0; i < config.spawnZones.Count; i++)
        {
            MapSpawnZoneData zone = config.spawnZones[i];
            sb.Append("    { ");
            sb.Append($"\"faction\": \"{zone.faction}\", ");
            sb.Append($"\"minX\": {WorldToPos(zone.minX)}, \"maxX\": {WorldToPos(zone.maxX)}, ");
            sb.Append($"\"minY\": {WorldToPos(zone.minY)}, \"maxY\": {WorldToPos(zone.maxY)} ");
            sb.Append(i + 1 < config.spawnZones.Count ? "}," : "}");
            sb.AppendLine();
        }

        sb.AppendLine("  ]");
        sb.AppendLine("}");
        return sb.ToString();
    }

    private static int WorldToPos(float worldUnits)
    {
        return Mathf.RoundToInt(worldUnits * PosScale);
    }

    private static float PosToWorld(int pos)
    {
        return pos / (float)PosScale;
    }

    private static int RadiansToAngleUnits(float radians)
    {
        return Mathf.RoundToInt(radians / (Mathf.PI * 2f) * AngleUnits);
    }

    private static float AngleUnitsToRadians(int angleUnits)
    {
        return angleUnits / (float)AngleUnits * (Mathf.PI * 2f);
    }

    private static string ToFullPath(string assetPath)
    {
        string projectRoot = Directory.GetParent(Application.dataPath).FullName;
        return Path.GetFullPath(Path.Combine(projectRoot, assetPath.Replace('/', Path.DirectorySeparatorChar)));
    }

    [System.Serializable]
    private class MapConfigJsonRoot
    {
        public int worldWidth;
        public int worldHeight;
        public MapWallJson[] walls;
        public MapSpawnZoneJson[] spawnZones;

        public MapConfigData ToRuntimeData()
        {
            var data = new MapConfigData
            {
                worldWidth = worldWidth > 0 ? PosToWorld(worldWidth) : 1000f,
                worldHeight = worldHeight > 0 ? PosToWorld(worldHeight) : 1000f
            };

            if (walls != null)
            {
                foreach (MapWallJson wall in walls)
                    data.walls.Add(wall.ToRuntimeData());
            }

            if (spawnZones != null && spawnZones.Length > 0)
            {
                foreach (MapSpawnZoneJson zone in spawnZones)
                    data.spawnZones.Add(zone.ToRuntimeData());
            }
            else
            {
                data.spawnZones.Add(MapSpawnZoneData.FromFaction(TankBattle.Faction.Soviet));
                data.spawnZones.Add(MapSpawnZoneData.FromFaction(TankBattle.Faction.USA));
                data.spawnZones.Add(MapSpawnZoneData.FromFaction(TankBattle.Faction.Germany));
                data.spawnZones.Add(MapSpawnZoneData.FromFaction(TankBattle.Faction.Italy));
            }

            return data;
        }
    }

    [System.Serializable]
    private class MapWallJson
    {
        public int x;
        public int y;
        public int width;
        public int height;
        public int rotation;

        public MapWallData ToRuntimeData()
        {
            return new MapWallData
            {
                x = PosToWorld(x),
                y = PosToWorld(y),
                width = PosToWorld(width),
                height = PosToWorld(height),
                rotation = AngleUnitsToRadians(rotation)
            };
        }
    }

    [System.Serializable]
    private class MapSpawnZoneJson
    {
        public string faction;
        public int minX;
        public int maxX;
        public int minY;
        public int maxY;

        public MapSpawnZoneData ToRuntimeData()
        {
            return new MapSpawnZoneData
            {
                faction = faction,
                minX = PosToWorld(minX),
                maxX = PosToWorld(maxX),
                minY = PosToWorld(minY),
                maxY = PosToWorld(maxY)
            };
        }
    }
}
