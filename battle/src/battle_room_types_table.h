/*
 * 战斗房间策划表：按 battle_def_id 配置人数上限及扩展字段（清单见 battle/design/）
 */

#ifndef wukong_battle_room_types_table_h
#define wukong_battle_room_types_table_h

#include "design_config/design_config_table.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace wukong {

// 单条战斗房间类型；后续策划字段直接加成员并在 onParse 里赋值即可
struct BattleRoomTypeDef {
    uint32_t battleDefId = 0;
    uint32_t maxPlayers = 0;
    uint32_t minPlayers = 1;
    // 每阵营可被玩家接管的坦克槽位数（默认 2）
    uint32_t slotsPerFaction = 2;
    // 整场战斗时长（秒，默认 300）；0 表示不限时
    uint32_t battleDurationSec = 300;
    // 开战后再允许加入的窗口（秒，默认 120）；0 表示开战即禁止中途加入
    uint32_t joinWindowSec = 120;
    std::string displayName;
    // 预留：与 battle_config.syncFrameRate 独立时可启用（0 表示沿用服务器默认）
    uint32_t frameRateOverride = 0;
};

class BattleRoomTypesTable : public design_config::DesignConfigTable {
public:
    const char *designName() const override { return "BattleRoomTypes"; }

    uint32_t getMaxPlayers(uint32_t battleDefId) const;
    uint32_t getSlotsPerFaction(uint32_t battleDefId) const;
    const BattleRoomTypeDef *find(uint32_t battleDefId) const;
    const std::unordered_map<uint32_t, BattleRoomTypeDef> &defs() const { return defs_; }

protected:
    bool onParse(const rapidjson::Document &doc) override;

private:
    std::unordered_map<uint32_t, BattleRoomTypeDef> defs_;
};

void registerBattleDesignTables();
BattleRoomTypesTable &getBattleRoomTypesTable();

} // namespace wukong

#define g_BattleRoomTypesTable wukong::getBattleRoomTypesTable()

#endif
