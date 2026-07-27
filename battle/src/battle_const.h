/*
 * KCP 业务消息类型（正数，勿与 corpc 内置负数系统消息冲突）
 */
#ifndef wukong_battle_const_h
#define wukong_battle_const_h

#include <cstdint>

namespace wukong {

const int32_t BATTLE_KCP_MSG_AUTH = 2101;
// 2102 原 BattleRoomStateNotify，已废弃勿复用
const int32_t BATTLE_KCP_MSG_LEAVE_ROOM = 2103;
const int32_t BATTLE_KCP_MSG_SNAPSHOT = 2104;
const int32_t BATTLE_KCP_MSG_FRAME_SYNC = 2105;
const int32_t BATTLE_KCP_MSG_INPUT_UPLOAD = 2106;

}

#endif
