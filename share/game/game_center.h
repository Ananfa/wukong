/*
 * Created by Xianke Liu on 2021/1/15.
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

#ifndef game_center_h
#define game_center_h

#include "corpc_redis.h"
#include "game_object.h"
#include "game_delegate.h"
#include "share/define.h"

#include <google/protobuf/message.h>

#include <vector>
#include <mutex>
#include <atomic>

using namespace corpc;

namespace wukong {
    class GameCenter
    {
        typedef std::function<void (std::shared_ptr<GameObject>, uint16_t, std::shared_ptr<google::protobuf::Message>)> MessageHandle;

        struct RegisterMessageInfo {
            google::protobuf::Message *proto;
            MessageHandle handle;
            bool needCoroutine;
        };

        struct HandleMessageInfo {
            std::shared_ptr<GameObject> obj;
            std::shared_ptr<google::protobuf::Message> msg;
            uint16_t tag;
            MessageHandle handle;
        };

    public:
        static GameCenter& Instance() {
            static GameCenter instance;
            return instance;
        }

        void init(GameServerType stype, uint32_t gameObjectUpdatePeriod, const char *dbHost, uint16_t dbPort, uint16_t dbIndex, uint32_t maxConnectNum);
        
        GameServerType getType() { return _type; }
        uint32_t getGameObjectUpdatePeriod() { return _gameObjectUpdatePeriod; }
        RedisConnectPool *getCachePool() { return _cache; }

        const std::string &setLocationSha1() { return _setLocationSha1; }
        const std::string &removeLocationSha1() { return _removeLocationSha1; }
        const std::string &updateLocationSha1() { return _updateLocationSha1; }
        const std::string &setLocationExpireSha1() { return _setLocationExpireSha1; }

        void setDelegate(GameDelegate delegate) { _delegate = delegate; }
        CreateGameObjectHandle getCreateGameObjectHandle() { return _delegate.createGameObject; }

        bool registerMessage(int msgType,
                             google::protobuf::Message *proto,
                             bool needCoroutine,
                             MessageHandle handle);

        void handleMessage(std::shared_ptr<GameObject>, int msgType, uint16_t tag, const std::string &rawMsg);

    private:
        static void *initRoutine(void *arg);

        static void *handleMessageRoutine( void * arg );

    private:
        GameServerType _type;
        uint32_t _gameObjectUpdatePeriod; // 游戏对象update执行周期，单位毫秒，为0时表示不进行update
        RedisConnectPool *_cache;

        std::string _setLocationSha1; // 设置location的lua脚本sha1值
        std::string _removeLocationSha1; // 删除location的lua脚本sha1值
        std::string _updateLocationSha1; // 更新location的lua脚本sha1值
        std::string _setLocationExpireSha1; // 设置location的超时lua脚本sha1值

        GameDelegate _delegate; // 委托对象（不同游戏有不同的委托实现）

        std::map<int, RegisterMessageInfo> _registerMessageMap;

    private:
        GameCenter(): _cache(nullptr), _gameObjectUpdatePeriod(0) {}                  // ctor hidden
        ~GameCenter() = default;                                   // destruct hidden
        GameCenter(GameCenter const&) = delete;                    // copy ctor delete
        GameCenter(GameCenter &&) = delete;                        // move ctor delete
        GameCenter& operator=(GameCenter const&) = delete;         // assign op. delete
        GameCenter& operator=(GameCenter &&) = delete;             // move assign op. delete
    };
}

#define g_GameCenter GameCenter::Instance()

#endif /* game_center_h */
