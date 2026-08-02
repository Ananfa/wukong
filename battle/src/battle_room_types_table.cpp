/*
 * 战斗房间策划表
 */

#include "battle_room_types_table.h"
#include "corpc_utils.h"
#include "design_config_hub.h"

#include "rapidjson/document.h"

#include <memory>

namespace wukong {

using namespace rapidjson;

namespace {

std::shared_ptr<BattleRoomTypesTable> g_roomTypes;

const Value *pickRoomTypeArray(const Document &doc) {
    if (doc.IsArray()) {
        return &doc;
    }
    if (doc.IsObject() && doc.HasMember("battleRoomTypes") && doc["battleRoomTypes"].IsArray()) {
        return &doc["battleRoomTypes"];
    }
    return nullptr;
}

} // namespace

bool BattleRoomTypesTable::onParse(const Document &doc) {
    const Value *arr = pickRoomTypeArray(doc);
    if (!arr) {
        ERROR_LOG("BattleRoomTypes: root must be array or object.battleRoomTypes array\n");
        return false;
    }
    std::unordered_map<uint32_t, BattleRoomTypeDef> next;
    for (SizeType i = 0; i < arr->Size(); ++i) {
        const Value &o = (*arr)[i];
        if (!o.IsObject() || !o.HasMember("battleDefId") || !o.HasMember("maxPlayers")) {
            ERROR_LOG("BattleRoomTypes[%u] need battleDefId and maxPlayers\n", (unsigned)i);
            return false;
        }
        BattleRoomTypeDef row;
        row.battleDefId = o["battleDefId"].GetUint();
        if (row.battleDefId == 0) {
            ERROR_LOG("BattleRoomTypes[%u] battleDefId must be non-zero\n", (unsigned)i);
            return false;
        }
        row.maxPlayers = o["maxPlayers"].GetUint();
        if (row.maxPlayers < 1) {
            row.maxPlayers = 1;
        }
        if (row.maxPlayers > 64) {
            row.maxPlayers = 64;
        }
        if (o.HasMember("minPlayers") && o["minPlayers"].IsUint()) {
            row.minPlayers = o["minPlayers"].GetUint();
        }
        if (row.minPlayers < 1) {
            row.minPlayers = 1;
        }
        if (row.minPlayers > row.maxPlayers) {
            ERROR_LOG("BattleRoomTypes[%u] minPlayers > maxPlayers for def %u\n", (unsigned)i, row.battleDefId);
            return false;
        }
        if (o.HasMember("displayName") && o["displayName"].IsString()) {
            row.displayName.assign(o["displayName"].GetString(), o["displayName"].GetStringLength());
        }
        if (o.HasMember("frameRateOverride") && o["frameRateOverride"].IsUint()) {
            row.frameRateOverride = o["frameRateOverride"].GetUint();
        }
        next[row.battleDefId] = std::move(row);
    }
    if (next.empty()) {
        ERROR_LOG("BattleRoomTypes: empty table\n");
        return false;
    }
    defs_.swap(next);
    return true;
}

uint32_t BattleRoomTypesTable::getMaxPlayers(uint32_t battleDefId) const {
    auto it = defs_.find(battleDefId);
    if (it == defs_.end()) {
        return 0;
    }
    return it->second.maxPlayers;
}

const BattleRoomTypeDef *BattleRoomTypesTable::find(uint32_t battleDefId) const {
    auto it = defs_.find(battleDefId);
    if (it == defs_.end()) {
        return nullptr;
    }
    return &it->second;
}

void registerBattleDesignTables() {
    g_roomTypes = std::make_shared<BattleRoomTypesTable>();
    g_DesignConfigHub.registerTable("BattleRoomTypes", g_roomTypes);
}

BattleRoomTypesTable &getBattleRoomTypesTable() {
    if (!g_roomTypes) {
        ERROR_LOG("getBattleRoomTypesTable -- not registered; call registerBattleDesignTables + loadFromManifest in BattleServer::init\n");
        // 避免空指针解引用崩溃；调用方仍可能因表为空得到 maxPlayers=0
        static BattleRoomTypesTable emptyFallback;
        return emptyFallback;
    }
    return *g_roomTypes;
}

} // namespace wukong
