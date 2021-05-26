/*
 * Created by Xianke Liu on 2021/5/6.
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

#include "record_config.h"
#include "corpc_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "const.h"
#include <cstdio>
#include <map>
#include <stdlib.h>

using namespace rapidjson;
using namespace wukong;

bool RecordConfig::parse(const char *path) {
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

        if (!server.HasMember("rpcPort")) {
            ERROR_LOG("config error -- servers[%d] id rpcPort not define\n", i);
            return false;
        }
        info.rpcPort = server["rpcPort"].GetUint();
        
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
    
    if (!doc.HasMember("mysql")) {
        ERROR_LOG("config error -- mysql not define\n");
        return false;
    }
    
    const Value& mysql = doc["mysql"];
    if (!mysql.IsObject()) {
        ERROR_LOG("config error -- mysql not object\n");
        return false;
    }
    
    if (!mysql.HasMember("host")) {
        ERROR_LOG("config error -- mysql.host not define\n");
        return false;
    }
    _mysql.host = mysql["host"].GetString();
    
    if (!mysql.HasMember("port")) {
        ERROR_LOG("config error -- mysql.port not define\n");
        return false;
    }
    _mysql.port = mysql["port"].GetUint();
    
    if (!mysql.HasMember("user")) {
        ERROR_LOG("config error -- mysql.user not define\n");
        return false;
    }
    _mysql.user = mysql["user"].GetString();
    
    if (!mysql.HasMember("pwd")) {
        ERROR_LOG("config error -- mysql.pwd not define\n");
        return false;
    }
    _mysql.pwd = mysql["pwd"].GetString();
    
    if (!mysql.HasMember("maxConnect")) {
        ERROR_LOG("config error -- mysql.maxConnect not define\n");
        return false;
    }
    _mysql.maxConnect = mysql["maxConnect"].GetUint();
    
    if (!mysql.HasMember("dbName")) {
        ERROR_LOG("config error -- mysql.dbName not define\n");
        return false;
    }
    _mysql.dbName = mysql["dbName"].GetString();
    
    return true;
}
