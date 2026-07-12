using System;
using System.Collections.Generic;
using TankBattle;

[Serializable]
public class MapWallData
{
    public float x = 500f;
    public float y = 500f;
    public float width = 100f;
    public float height = 18f;
    public float rotation = 0f;

    public MapWallData Clone()
    {
        return new MapWallData
        {
            x = x,
            y = y,
            width = width,
            height = height,
            rotation = rotation
        };
    }
}

[Serializable]
public class MapSpawnZoneData
{
    public string faction = "soviet";
    public float minX = 80f;
    public float maxX = 220f;
    public float minY = 80f;
    public float maxY = 220f;

    public MapSpawnZoneData Clone()
    {
        return new MapSpawnZoneData
        {
            faction = faction,
            minX = minX,
            maxX = maxX,
            minY = minY,
            maxY = maxY
        };
    }

    public static MapSpawnZoneData FromFaction(Faction factionEnum)
    {
        const float margin = 80f;
        const float spread = 140f;
        const float world = 1000f;

        switch (factionEnum)
        {
        case Faction.USA:
            return new MapSpawnZoneData
            {
                faction = "usa",
                minX = world - margin - spread,
                maxX = world - margin,
                minY = margin,
                maxY = margin + spread
            };
        case Faction.Germany:
            return new MapSpawnZoneData
            {
                faction = "germany",
                minX = margin,
                maxX = margin + spread,
                minY = world - margin - spread,
                maxY = world - margin
            };
        case Faction.Italy:
            return new MapSpawnZoneData
            {
                faction = "italy",
                minX = world - margin - spread,
                maxX = world - margin,
                minY = world - margin - spread,
                maxY = world - margin
            };
        default:
            return new MapSpawnZoneData
            {
                faction = "soviet",
                minX = margin,
                maxX = margin + spread,
                minY = margin,
                maxY = margin + spread
            };
        }
    }

    public Faction ToFaction()
    {
        switch (faction)
        {
        case "usa": return Faction.USA;
        case "germany": return Faction.Germany;
        case "italy": return Faction.Italy;
        default: return Faction.Soviet;
        }
    }
}

[Serializable]
public class MapConfigData
{
    public float worldWidth = 1000f;
    public float worldHeight = 1000f;
    public List<MapWallData> walls = new List<MapWallData>();
    public List<MapSpawnZoneData> spawnZones = new List<MapSpawnZoneData>();

    public static MapConfigData CreateDefault()
    {
        var config = new MapConfigData();
        config.walls.Add(new MapWallData { x = 180f, y = 500f, width = 360f, height = 18f });
        config.walls.Add(new MapWallData { x = 820f, y = 500f, width = 360f, height = 18f });
        config.walls.Add(new MapWallData { x = 500f, y = 180f, width = 18f, height = 360f });
        config.walls.Add(new MapWallData { x = 500f, y = 820f, width = 18f, height = 360f });

        config.spawnZones.Add(MapSpawnZoneData.FromFaction(Faction.Soviet));
        config.spawnZones.Add(MapSpawnZoneData.FromFaction(Faction.USA));
        config.spawnZones.Add(MapSpawnZoneData.FromFaction(Faction.Germany));
        config.spawnZones.Add(MapSpawnZoneData.FromFaction(Faction.Italy));
        return config;
    }

    public MapConfigData Clone()
    {
        var copy = new MapConfigData
        {
            worldWidth = worldWidth,
            worldHeight = worldHeight
        };
        foreach (MapWallData wall in walls)
            copy.walls.Add(wall.Clone());
        foreach (MapSpawnZoneData zone in spawnZones)
            copy.spawnZones.Add(zone.Clone());
        return copy;
    }

    public MapSpawnZoneData GetSpawnZone(Faction faction)
    {
        foreach (MapSpawnZoneData zone in spawnZones)
        {
            if (zone.ToFaction() == faction)
                return zone;
        }

        MapSpawnZoneData created = MapSpawnZoneData.FromFaction(faction);
        spawnZones.Add(created);
        return created;
    }
}
