/*
 * Created by Xianke Liu on 2020/11/16.
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

#include "gateway_config.h"
#include "corpc_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "const.h"
#include <cstdio>
#include <map>
#include <stdlib.h>

using namespace rapidjson;
using namespace wukong;

bool GatewayConfig::parse(const char *path) {
    FILE* fp = fopen(path, "rb");
    
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    Document doc;
    doc.ParseStream(is);
    
    if (!doc.HasMember("internalIp")) {
        ERROR_LOG("config error -- internalIp not define\n");
        return false;
    }
    _internalIp = doc["internalIp"].GetString();
    
    if (!doc.HasMember("externalIp")) {
        ERROR_LOG("config error -- externalIp not define\n");
        return false;
    }
    _externalIp = doc["externalIp"].GetString();
    
    if (doc.HasMember("outerAddr")) {
        _outerAddr = doc["outerAddr"].GetString();
    } else {
        _outerAddr = _externalIp;
    }
    
    if (!doc.HasMember("rpcPort")) {
        ERROR_LOG("config error -- rpcPort not define\n");
        return false;
    }
    _rpcPort = doc["rpcPort"].GetUint();
    
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

        if (!server.HasMember("msgPort")) {
            ERROR_LOG("config error -- servers[%d] id msgPort not define\n", i);
            return false;
        }
        info.msgPort = server["msgPort"].GetUint();
        
        if (server.HasMember("outerPort")) {
            info.outerPort = server["outerPort"].GetUint();
        } else {
            info.outerPort = info.msgPort;
        }

        serverIdMap.insert(std::make_pair(info.id, true));
        _serverInfos.push_back(info);
    }

    if (!doc.HasMember("zookeeper")) {
        ERROR_LOG("config error -- zookeeper not define\n");
        return false;
    }
    _zookeeper = doc["zookeeper"].GetString();

    if (!doc.HasMember("verifyTimeout")) {
        ERROR_LOG("config error -- verifyTimeout not define\n");
        return false;
    }
    _verifyTimeout = doc["verifyTimeout"].GetUint();
    
    if (!doc.HasMember("disconnectTimeout")) {
        ERROR_LOG("config error -- disconnectTimeout not define\n");
        return false;
    }
    _disconnectTimeout = doc["disconnectTimeout"].GetUint();
    
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
    
    if (!doc.HasMember("cache")) {
        ERROR_LOG("config error -- cache not define\n");
        return false;
    }
    
    const Value& cache = doc["cache"];
    if (!cache.IsObject()) {
        ERROR_LOG("config error -- cache not object\n");
        return false;
    }
    
    if (!cache.HasMember("host")) {
        ERROR_LOG("config error -- cache.host not define\n");
        return false;
    }
    _cache.host = cache["host"].GetString();
    
    if (!cache.HasMember("port")) {
        ERROR_LOG("config error -- cache.port not define\n");
        return false;
    }
    _cache.port = cache["port"].GetUint();
    
    if (!cache.HasMember("dbIndex")) {
        ERROR_LOG("config error -- cache.dbIndex not define\n");
        return false;
    }
    _cache.dbIndex = cache["dbIndex"].GetUint();
    
    if (!cache.HasMember("maxConnect")) {
        ERROR_LOG("config error -- cache.maxConnect not define\n");
        return false;
    }
    _cache.maxConnect = cache["maxConnect"].GetUint();

    if (!doc.HasMember("enableSceneClient")) {
        _enableSceneClient = false;
    } else {
        _enableSceneClient = doc["enableSceneClient"].GetBool();
    }
    
    _zooPath = ZK_GATEWAY_SERVER + "/" + _internalIp + ":" + std::to_string(_rpcPort) + ":" + _outerAddr;
    for (const GatewayConfig::ServerInfo &info : _serverInfos) {
        _zooPath += "|" + std::to_string(info.id) + ":" + std::to_string(info.outerPort);
    }
    
    return true;
}
