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
    ip_ = doc["ip"].GetString();

    if (!doc.HasMember("port")) {
        ERROR_LOG("config error -- port not define\n");
        return false;
    }
    port_ = doc["port"].GetUint();
    
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
        serverInfos_.push_back(info);
    }

    if (!doc.HasMember("zookeeper")) {
        ERROR_LOG("config error -- zookeeper not define\n");
        return false;
    }
    zookeeper_ = doc["zookeeper"].GetString();

    if (!doc.HasMember("ioRecvThreadNum")) {
        ERROR_LOG("config error -- ioRecvThreadNum not define\n");
        return false;
    }
    ioRecvThreadNum_ = doc["ioRecvThreadNum"].GetUint();
    
    if (!doc.HasMember("ioSendThreadNum")) {
        ERROR_LOG("config error -- ioSendThreadNum not define\n");
        return false;
    }
    ioSendThreadNum_ = doc["ioSendThreadNum"].GetUint();
    
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
        redisInfos_.push_back(info);
    }

    const Value& mysqls = doc["mysql"];
    if (!mysqls.IsArray()) {
        ERROR_LOG("config error -- mysql not array\n");
        return false;
    }

    std::map<std::string, bool> mysqlNameMap;
    for (SizeType i = 0; i < mysqls.Size(); i++) {
        const Value& mysql = mysqls[i];

        MysqlInfo info;
        
        if (!mysql.HasMember("dbName")) {
            ERROR_LOG("config error -- mysql.dbName not define\n");
            return false;
        }
        info.dbName = mysql["dbName"].GetString();
        if (mysqlNameMap.find(info.dbName) != mysqlNameMap.end()) {
            ERROR_LOG("config error -- mysql dbName %s duplicate\n", info.dbName.c_str());
            return false;
        }
    
        if (!mysql.HasMember("host")) {
            ERROR_LOG("config error -- mysql[%s].host not define\n", info.dbName.c_str());
            return false;
        }
        info.host = mysql["host"].GetString();
        
        if (!mysql.HasMember("port")) {
            ERROR_LOG("config error -- mysql[%s].port not define\n", info.dbName.c_str());
            return false;
        }
        info.port = mysql["port"].GetUint();
        
        if (!mysql.HasMember("user")) {
            ERROR_LOG("config error -- mysql[%s].user not define\n", info.dbName.c_str());
            return false;
        }
        info.user = mysql["user"].GetString();
        
        if (!mysql.HasMember("pwd")) {
            ERROR_LOG("config error -- mysql[%s].pwd not define\n", info.dbName.c_str());
            return false;
        }
        info.pwd = mysql["pwd"].GetString();
        
        if (!mysql.HasMember("maxConnect")) {
            ERROR_LOG("config error -- mysql[%s].maxConnect not define\n", info.dbName.c_str());
            return false;
        }
        info.maxConnect = mysql["maxConnect"].GetUint();
    
        mysqlNameMap.insert(std::make_pair(info.dbName, true));
        mysqlInfos_.push_back(info);
    }
    
    if (!doc.HasMember("coreCache")) {
        ERROR_LOG("config error -- coreCache not define\n");
        return false;
    }
    coreCache_ = doc["coreCache"].GetString();
    
    if (!doc.HasMember("coreRecord")) {
        ERROR_LOG("config error -- coreRecord not define\n");
        return false;
    }
    coreRecord_ = doc["coreRecord"].GetString();
    
    zooPath_ = ZK_RECORD_SERVER + "/" + ip_ + ":" + std::to_string(port_);
    for (const RecordConfig::ServerInfo &info : serverInfos_) {
        zooPath_ += "|" + std::to_string(info.id);
    }
    
    return true;
}
