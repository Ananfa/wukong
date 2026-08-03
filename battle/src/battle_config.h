/*
 * Created by Xianke Liu on 2026/1/4.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef wukong_battle_config_h
#define wukong_battle_config_h

#include <stdio.h>
#include <cstdint>
#include <string>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class BattleConfig {
    public:
        static BattleConfig& Instance() {
            static BattleConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        uint32_t getId() const { return id_; }
        const std::string& getInternalIp() const { return internalIp_; }
        const std::string& getExternalIp() const { return externalIp_; }
        uint16_t getRpcPort() const { return rpcPort_; }
        uint16_t getMsgPort() const { return msgPort_; }

        uint32_t getVerifyTimeout() const { return verifyTimeout_; }
        uint32_t getDisconnectTimeout() const { return disconnectTimeout_; }
        uint32_t getRoomEmptyDestroySec() const { return roomEmptyDestroySec_; }
        uint32_t getSyncFrameRate() const { return syncFrameRate_; }
        uint32_t getIoRecvThreadNum() const { return ioRecvThreadNum_; }
        uint32_t getIoSendThreadNum() const { return ioSendThreadNum_; }

        const Address& getNexusAddr() const { return nexusAddr_; }

        // 策划配置清单路径（相对 battle 主配置 JSON 所在目录，或绝对路径）
        const std::string &getDesignConfigManifest() const { return designConfigManifest_; }

        // 场景阻挡/出生区 JSON 原文（与 Unity Assets/Config/MapObstacles.json 同内容）
        const std::string &getMapObstaclesJson() const { return mapObstaclesJson_; }
        const std::string &getMapObstaclesPath() const { return mapObstaclesPathResolved_; }

    private:
        uint32_t id_;                // 服务号（Gateway服务唯一标识）

        std::string internalIp_;    // 提供rpc服务的ip
        std::string externalIp_;    // 对客户端提供服务的ip
        uint16_t rpcPort_;          // rpc服务端口
        uint16_t msgPort_;          // udp消息服务端口

        uint32_t verifyTimeout_;        // RPC 登记后进房：未在时限内完成 KCP 鉴权则取消席位（秒）
        uint32_t disconnectTimeout_;    // 已鉴权玩家掉线后未重连则踢出房间（秒）
        uint32_t roomEmptyDestroySec_;  // 房间内无任何玩家席位后持续该秒数则销毁房间（秒）
        uint32_t syncFrameRate_;        // 帧同步逻辑帧率（Hz），至少一名已鉴权玩家在线时才推进
        uint32_t ioRecvThreadNum_;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t ioSendThreadNum_;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        Address nexusAddr_;     // nexus服务地址

        std::string designConfigManifest_;
        std::string mapObstaclesPathResolved_;
        std::string mapObstaclesJson_;

    private:
        BattleConfig() = default;                                 // ctor hidden
        BattleConfig(BattleConfig const&) = delete;               // copy ctor hidden
        BattleConfig(BattleConfig &&) = delete;                   // move ctor hidden
        BattleConfig& operator=(BattleConfig const&) = delete;    // assign op. hidden
        BattleConfig& operator=(BattleConfig &&) = delete;        // move assign op. hidden
        ~BattleConfig() = default;                                // dtor hidden
    };

}

#define g_BattleConfig wukong::BattleConfig::Instance()

#endif /* wukong_battle_config_h */
