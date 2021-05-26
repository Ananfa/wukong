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

    if (!doc.HasMember("roleNumForPlayer")) {
        ERROR_LOG("config error -- roleNumForPlayer not define\n");
        return false;
    }
    _roleNumForPlayer = doc["roleNumForPlayer"].GetUint();

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
    
    if (!doc.HasMember("redis")) {
        ERROR_LOG("config error -- db not define\n");
        return false;
    }
    
    const Value& redis = doc["redis"];
    if (!redis.IsObject()) {
        ERROR_LOG("config error -- redis not object\n");
        return false;
    }
    
    if (!redis.HasMember("host")) {
        ERROR_LOG("config error -- redis.host not define\n");
        return false;
    }
    _redis.host = redis["host"].GetString();
    
    if (!redis.HasMember("port")) {
        ERROR_LOG("config error -- redis.port not define\n");
        return false;
    }
    _redis.port = redis["port"].GetUint();
    
    if (!redis.HasMember("dbIndex")) {
        ERROR_LOG("config error -- redis.dbIndex not define\n");
        return false;
    }
    _redis.dbIndex = redis["dbIndex"].GetUint();
    
    if (!redis.HasMember("maxConnect")) {
        ERROR_LOG("config error -- redis.maxConnect not define\n");
        return false;
    }
    _redis.maxConnect = redis["maxConnect"].GetUint();
    
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
