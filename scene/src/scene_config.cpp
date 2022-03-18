/*
 * Created by Xianke Liu on 2022/1/12.
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

#include "scene_config.h"
#include "corpc_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "const.h"
#include <cstdio>
#include <map>
#include <stdlib.h>

using namespace rapidjson;
using namespace wukong;

bool SceneConfig::parse(const char *path) {
    FILE* fp = fopen(path, "rb");
    
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    Document doc;
    doc.ParseStream(is);
    
    if (!doc.HasMember("ip")) {
        ERROR_LOG("config error -- ip not define\n");
        return false;
    }
    _ip = doc["ip"].GetString();
    
    if (!doc.HasMember("port")) {
        ERROR_LOG("config error -- port not define\n");
        return false;
    }
    _port = doc["port"].GetUint();

    if (!doc.HasMember("type")) {
        ERROR_LOG("config error -- type not define\n");
        return false;
    }
    _type = doc["type"].GetUint();

    if (!doc.HasMember("servers")) {
        ERROR_LOG("config error -- servers not define\n");
        return false;
    }

    const Value& servers = doc["servers"];
    if (!servers.IsArray()) {
        ERROR_LOG("config error -- servers not array\n");
        return false;
    }

    std::map<uint32_t, bool> serverIdMap;
    for (SizeType i = 0; i < servers.Size(); i++) {
        const Value& server = servers[i];

        ServerInfo info;
        if (!server.HasMember("id")) {
            ERROR_LOG("config error -- servers[%d] id not define\n", i);
            return false;
        }
        info.id = server["id"].GetInt();
        if (serverIdMap.find(info.id) != serverIdMap.end()) {
            ERROR_LOG("config error -- servers id %d duplicate\n", info.id);
            return false;
        }

        serverIdMap.insert(std::make_pair(info.id, true));
        _serverInfos.push_back(info);
    }

    if (!doc.HasMember("zookeeper")) {
        ERROR_LOG("config error -- zookeeper not define\n");
        return false;
    }
    _zookeeper = doc["zookeeper"].GetString();

    if (!doc.HasMember("ioRecvThreadNum")) {
        ERROR_LOG("config error -- ioRecvThreadNum not define\n");
        return false;
    }
    _ioRecvThreadNum = doc["ioRecvThreadNum"].GetUint();
    
    if (!doc.HasMember("ioSendThreadNum")) {
        ERROR_LOG("config error -- ioSendThreadNum not define\n");
        return false;
    }
    _ioSendThreadNum = doc["ioSendThreadNum"].GetUint();
    
    const Value& rediss = doc["redis"];
    if (!rediss.IsArray()) {
        ERROR_LOG("config error -- redis not array\n");
        return false;
    }

    std::map<std::string, bool> redisNameMap;
    for (SizeType i = 0; i < rediss.Size(); i++) {
        const Value& redis = rediss[i];

        RedisInfo info;
        if (!redis.HasMember("dbName")) {
            ERROR_LOG("config error -- redis[%d] dbName not define\n", i);
            return false;
        }
        info.dbName = redis["dbName"].GetString();
        if (redisNameMap.find(info.dbName) != redisNameMap.end()) {
            ERROR_LOG("config error -- redis dbName %s duplicate\n", info.dbName.c_str());
            return false;
        }

        if (!redis.HasMember("host")) {
            ERROR_LOG("config error -- redis[%s].host not define\n", info.dbName.c_str());
            return false;
        }
        info.host = redis["host"].GetString();
        
        if (redis.HasMember("pwd")) {
            info.pwd = redis["pwd"].GetString();
        }
        
        if (!redis.HasMember("port")) {
            ERROR_LOG("config error -- redis[%s].port not define\n", info.dbName.c_str());
            return false;
        }
        info.port = redis["port"].GetUint();
        
        if (!redis.HasMember("dbIndex")) {
            ERROR_LOG("config error -- redis[%s].dbIndex not define\n", info.dbName.c_str());
            return false;
        }
        info.dbIndex = redis["dbIndex"].GetUint();
        
        if (!redis.HasMember("maxConnect")) {
            ERROR_LOG("config error -- redis[%s].maxConnect not define\n", info.dbName.c_str());
            return false;
        }
        info.maxConnect = redis["maxConnect"].GetUint();

        redisNameMap.insert(std::make_pair(info.dbName, true));
        _redisInfos.push_back(info);
    }
    
    if (!doc.HasMember("coreCache")) {
        ERROR_LOG("config error -- coreCache not define\n");
        return false;
    }
    _coreCache = doc["coreCache"].GetString();
    
    if (!doc.HasMember("updatePeriod")) {
        _updatePeriod = 0;
    } else {
        _updatePeriod = doc["updatePeriod"].GetUint();
    }

    if (!doc.HasMember("enableLobbyClient")) {
        _enableLobbyClient = false;
    } else {
        _enableLobbyClient = doc["enableLobbyClient"].GetBool();
    }

    if (!doc.HasMember("enableSceneClient")) {
        _enableSceneClient = false;
    } else {
        _enableSceneClient = doc["enableSceneClient"].GetBool();
    }
    
    _zooPath = ZK_SCENE_SERVER + "/" + _ip + ":" + std::to_string(_port) + ":" + std::to_string(_type);
    for (const LobbyConfig::ServerInfo &info : _serverInfos) {
        _zooPath += "|" + std::to_string(info.id);
    }
    
    return true;
}
