#include "MapObstacleLoader.h"
#include "../Common/Constants.h"
#include "../Common/FixedMath.h"
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace TankBattle
{
    namespace
    {
        const char* SkipWhitespace(const char* p)
        {
            while (*p && std::isspace(static_cast<unsigned char>(*p)))
                ++p;
            return p;
        }

        bool ParseInt(const char*& p, int64_t& outValue)
        {
            p = SkipWhitespace(p);
            char* end = nullptr;
            outValue = std::strtol(p, &end, 10);
            if (end == p)
                return false;
            p = end;
            return true;
        }

        bool ExpectChar(const char*& p, char expected)
        {
            p = SkipWhitespace(p);
            if (*p != expected)
                return false;
            ++p;
            return true;
        }

        bool ParseWallObject(const char*& p, ObstacleWall& wall)
        {
            if (!ExpectChar(p, '{'))
                return false;

            Pos x = 0;
            Pos y = 0;
            Pos width = 0;
            Pos height = 0;
            Angle rotation = 0;
            bool hasX = false;
            bool hasY = false;
            bool hasWidth = false;
            bool hasHeight = false;

            while (true)
            {
                p = SkipWhitespace(p);
                if (*p == '}')
                {
                    ++p;
                    break;
                }
                if (*p == ',')
                {
                    ++p;
                    continue;
                }
                if (*p != '"')
                    return false;

                ++p;
                const char* keyStart = p;
                while (*p && *p != '"')
                    ++p;
                if (*p != '"')
                    return false;

                std::string key(keyStart, p - keyStart);
                ++p;
                if (!ExpectChar(p, ':'))
                    return false;

                int64_t value = 0;
                if (!ParseInt(p, value))
                    return false;

                if (key == "x") { x = static_cast<Pos>(value); hasX = true; }
                else if (key == "y") { y = static_cast<Pos>(value); hasY = true; }
                else if (key == "width") { width = static_cast<Pos>(value); hasWidth = true; }
                else if (key == "height") { height = static_cast<Pos>(value); hasHeight = true; }
                else if (key == "rotation" || key == "rotationAngle")
                {
                    rotation = static_cast<Angle>(value);
                }
            }

            if (!hasX || !hasY || !hasWidth || !hasHeight || width <= 0 || height <= 0)
                return false;

            wall = MakeObstacleWallFromPos(x, y, width / 2, height / 2, rotation);
            return true;
        }

        const char* FindKey(const std::string& json, const char* key)
        {
            std::string needle = std::string("\"") + key + "\"";
            size_t pos = json.find(needle);
            if (pos == std::string::npos)
                return nullptr;
            const char* p = json.c_str() + pos + needle.size();
            p = SkipWhitespace(p);
            if (*p != ':')
                return nullptr;
            ++p;
            return SkipWhitespace(p);
        }

        bool ParseWallsArray(const char* p, std::vector<ObstacleWall>& outWalls)
        {
            if (!p || *p != '[')
                return false;
            ++p;

            while (true)
            {
                p = SkipWhitespace(p);
                if (*p == ']')
                {
                    ++p;
                    return true;
                }
                if (*p == ',')
                {
                    ++p;
                    continue;
                }

                ObstacleWall wall;
                if (!ParseWallObject(p, wall))
                    return false;
                outWalls.push_back(wall);
            }
        }

        bool ParseFactionName(const char* start, const char* end, Faction& outFaction)
        {
            size_t len = static_cast<size_t>(end - start);
            if (len == 6 && strncmp(start, "soviet", 6) == 0)
            {
                outFaction = Faction::Soviet;
                return true;
            }
            if (len == 3 && strncmp(start, "usa", 3) == 0)
            {
                outFaction = Faction::USA;
                return true;
            }
            if (len == 7 && strncmp(start, "germany", 7) == 0)
            {
                outFaction = Faction::Germany;
                return true;
            }
            if (len == 5 && strncmp(start, "italy", 5) == 0)
            {
                outFaction = Faction::Italy;
                return true;
            }
            return false;
        }

        bool ParseSpawnZoneObject(const char*& p, FactionSpawnZone& zone, Faction& faction)
        {
            if (!ExpectChar(p, '{'))
                return false;

            bool hasFaction = false;
            bool hasMinX = false;
            bool hasMaxX = false;
            bool hasMinY = false;
            bool hasMaxY = false;
            faction = Faction::Soviet;

            while (true)
            {
                p = SkipWhitespace(p);
                if (*p == '}')
                {
                    ++p;
                    break;
                }
                if (*p == ',')
                {
                    ++p;
                    continue;
                }
                if (*p != '"')
                    return false;

                ++p;
                const char* keyStart = p;
                while (*p && *p != '"')
                    ++p;
                if (*p != '"')
                    return false;

                std::string key(keyStart, p - keyStart);
                ++p;
                if (!ExpectChar(p, ':'))
                    return false;

                if (key == "faction")
                {
                    p = SkipWhitespace(p);
                    if (*p != '"')
                        return false;
                    ++p;
                    const char* valueStart = p;
                    while (*p && *p != '"')
                        ++p;
                    if (*p != '"')
                        return false;
                    if (!ParseFactionName(valueStart, p, faction))
                        return false;
                    ++p;
                    hasFaction = true;
                    continue;
                }

                int64_t value = 0;
                if (!ParseInt(p, value))
                    return false;

                if (key == "minX") { zone.minX = static_cast<Pos>(value); hasMinX = true; }
                else if (key == "maxX") { zone.maxX = static_cast<Pos>(value); hasMaxX = true; }
                else if (key == "minY") { zone.minY = static_cast<Pos>(value); hasMinY = true; }
                else if (key == "maxY") { zone.maxY = static_cast<Pos>(value); hasMaxY = true; }
            }

            return hasFaction && hasMinX && hasMaxX && hasMinY && hasMaxY &&
                zone.maxX > zone.minX && zone.maxY > zone.minY;
        }

        bool ParseSpawnZonesArray(
            const char* p,
            FactionSpawnZone outSpawnZones[kFactionCount],
            bool& outHasSpawnZones)
        {
            outHasSpawnZones = false;
            if (!p || *p != '[')
                return false;
            ++p;

            while (true)
            {
                p = SkipWhitespace(p);
                if (*p == ']')
                {
                    ++p;
                    return true;
                }
                if (*p == ',')
                {
                    ++p;
                    continue;
                }

                FactionSpawnZone zone;
                Faction faction = Faction::Soviet;
                if (!ParseSpawnZoneObject(p, zone, faction))
                    return false;

                int idx = static_cast<int>(faction);
                if (idx >= 0 && idx < kFactionCount)
                {
                    outSpawnZones[idx] = zone;
                    outHasSpawnZones = true;
                }
            }
        }
    }

    void LoadDefaultMapObstacles(
        std::vector<ObstacleWall>& outWalls,
        Pos& outWorldWidthPos,
        Pos& outWorldHeightPos)
    {
        outWalls.clear();
        outWorldWidthPos = static_cast<Pos>(kDefaultWorldSizePosValue);
        outWorldHeightPos = static_cast<Pos>(kDefaultWorldSizePosValue);

        auto addWall = [&](Pos x, Pos y, Pos width, Pos height, Angle rotation) {
            outWalls.push_back(MakeObstacleWallFromPos(x, y, width / 2, height / 2, rotation));
        };

        const Pos u = static_cast<Pos>(kRngWorldSubunitsPerUnit);
        addWall(500 * u, 500 * u, 360 * u, 18 * u, 0);
        addWall(500 * u, 500 * u, 18 * u, 360 * u, 0);
        // 45° ≈ 8192 angle units
        addWall(320 * u, 680 * u, 180 * u, 16 * u, 8192);
        addWall(680 * u, 320 * u, 180 * u, 16 * u, static_cast<Angle>(-8192));
    }

    void LoadDefaultSpawnZones(FactionSpawnZone outSpawnZones[kFactionCount])
    {
        const Pos margin = static_cast<Pos>(kFactionSpawnMarginPosValue);
        const Pos spread = static_cast<Pos>(kFactionSpawnSpreadPosValue);
        const Pos worldWidth = static_cast<Pos>(kDefaultWorldSizePosValue);
        const Pos worldHeight = static_cast<Pos>(kDefaultWorldSizePosValue);

        outSpawnZones[0] = {margin, margin + spread, margin, margin + spread};
        outSpawnZones[1] = {worldWidth - margin - spread, worldWidth - margin, margin, margin + spread};
        outSpawnZones[2] = {margin, margin + spread, worldHeight - margin - spread, worldHeight - margin};
        outSpawnZones[3] = {
            worldWidth - margin - spread,
            worldWidth - margin,
            worldHeight - margin - spread,
            worldHeight - margin
        };
    }

    bool ParseMapConfigFromJson(
        const std::string& json,
        std::vector<ObstacleWall>& outWalls,
        Pos& outWorldWidthPos,
        Pos& outWorldHeightPos,
        FactionSpawnZone outSpawnZones[kFactionCount],
        bool& outHasSpawnZones)
    {
        outWalls.clear();
        outWorldWidthPos = static_cast<Pos>(kDefaultWorldSizePosValue);
        outWorldHeightPos = static_cast<Pos>(kDefaultWorldSizePosValue);
        outHasSpawnZones = false;
        LoadDefaultSpawnZones(outSpawnZones);

        if (json.empty())
            return false;

        const char* widthPtr = FindKey(json, "worldWidth");
        if (widthPtr)
        {
            int64_t value = 0;
            if (ParseInt(widthPtr, value) && value > 0)
                outWorldWidthPos = static_cast<Pos>(value);
        }

        const char* heightPtr = FindKey(json, "worldHeight");
        if (heightPtr)
        {
            int64_t value = 0;
            if (ParseInt(heightPtr, value) && value > 0)
                outWorldHeightPos = static_cast<Pos>(value);
        }

        const char* wallsPtr = FindKey(json, "walls");
        if (wallsPtr && !ParseWallsArray(wallsPtr, outWalls))
            return false;

        const char* spawnPtr = FindKey(json, "spawnZones");
        if (spawnPtr && !ParseSpawnZonesArray(spawnPtr, outSpawnZones, outHasSpawnZones))
            return false;

        return true;
    }
}
