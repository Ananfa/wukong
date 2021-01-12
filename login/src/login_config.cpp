/*
 * Created by Xianke Liu on 2020/11/20.
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

#include "login_config.h"
#include "corpc_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "const.h"
#include <cstdio>
#include <map>
#include <stdlib.h>

using namespace rapidjson;
using namespace wukong;

bool LoginConfig::parse(const char *path) {
    FILE* fp = fopen(path, "rb");
    
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    Document doc;
    doc.ParseStream(is);
    
    if (!doc.HasMember("id")) {
        ERROR_LOG("config error -- id not define\n");
        return false;
    }
    _id = doc["id"].GetUint();
    
    if (!doc.HasMember("serviceIp")) {
        ERROR_LOG("config error -- serviceIp not define\n");
        return false;
    }
    _serviceIp = doc["serviceIp"].GetString();
    
    if (!doc.HasMember("servicePort")) {
        ERROR_LOG("config error -- servicePort not define\n");
        return false;
    }
    _servicePort = doc["servicePort"].GetUint();
    
    if (!doc.HasMember("zookeeper")) {
        ERROR_LOG("config error -- zookeeper not define\n");
        return false;
    }
    _zookeeper = doc["zookeeper"].GetString();
    _zookeeperPath = ZK_LOGIN_SERVER + "/" + std::to_string(_id);

    if (!doc.HasMember("workerThreadNum")) {
        ERROR_LOG("config error -- workerThreadNum not define\n");
        return false;
    }
    _workerThreadNum = doc["workerThreadNum"].GetUint();
    
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
    
    if (!doc.HasMember("db")) {
        ERROR_LOG("config error -- db not define\n");
        return false;
    }
    
    const Value& db = doc["db"];
    if (!db.IsObject()) {
        ERROR_LOG("config error -- db not object\n");
        return false;
    }
    
    if (!db.HasMember("host")) {
        ERROR_LOG("config error -- db.host not define\n");
        return false;
    }
    _db.host = db["host"].GetString();
    
    if (!db.HasMember("port")) {
        ERROR_LOG("config error -- db.port not define\n");
        return false;
    }
    _db.port = db["port"].GetUint();
    
    if (!db.HasMember("dbIndex")) {
        ERROR_LOG("config error -- db.dbIndex not define\n");
        return false;
    }
    _db.dbIndex = db["dbIndex"].GetUint();
    
    if (!db.HasMember("maxConnect")) {
        ERROR_LOG("config error -- db.maxConnect not define\n");
        return false;
    }
    _db.maxConnect = db["maxConnect"].GetUint();
    
    return true;
}
