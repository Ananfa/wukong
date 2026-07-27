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

#include "battle_config.h"
#include "corpc_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "const.h"
#include <cstdio>
#include <stdlib.h>

using namespace rapidjson;
using namespace wukong;

namespace {

struct FileGuard {
    FILE *f;
    explicit FileGuard(FILE *fp) : f(fp) {}
    ~FileGuard() {
        if (f) {
            fclose(f);
        }
    }
};

}

bool BattleConfig::parse(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ERROR_LOG("config error -- cannot open %s\n", path);
        return false;
    }
    FileGuard guard(fp);

    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    Document doc;
    doc.ParseStream(is);
    
    if (!doc.HasMember("id")) {
        ERROR_LOG("config error -- id not define\n");
        return false;
    }
    id_ = doc["id"].GetInt();

    if (!doc.HasMember("internalIp")) {
        ERROR_LOG("config error -- internalIp not define\n");
        return false;
    }
    internalIp_ = doc["internalIp"].GetString();
    
    if (!doc.HasMember("externalIp")) {
        ERROR_LOG("config error -- externalIp not define\n");
        return false;
    }
    externalIp_ = doc["externalIp"].GetString();
    
    if (!doc.HasMember("rpcPort")) {
        ERROR_LOG("config error -- rpcPort not define\n");
        return false;
    }
    rpcPort_ = doc["rpcPort"].GetUint();
    
    if (!doc.HasMember("msgPort")) {
        ERROR_LOG("config error -- msgPort not define\n");
        return false;
    }
    msgPort_ = doc["msgPort"].GetUint();
    
    if (doc.HasMember("verifyTimeout")) {
        verifyTimeout_ = doc["verifyTimeout"].GetUint();
    } else {
        verifyTimeout_ = 60;
    }
    
    if (doc.HasMember("disconnectTimeout")) {
        disconnectTimeout_ = doc["disconnectTimeout"].GetUint();
    } else {
        disconnectTimeout_ = 60;
    }

    if (doc.HasMember("roomEmptyDestroySec")) {
        roomEmptyDestroySec_ = doc["roomEmptyDestroySec"].GetUint();
    } else {
        roomEmptyDestroySec_ = 60;
    }

    if (doc.HasMember("syncFrameRate")) {
        syncFrameRate_ = doc["syncFrameRate"].GetUint();
    } else {
        syncFrameRate_ = 30;
    }
    if (syncFrameRate_ == 0) {
        syncFrameRate_ = 30;
    }
    
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
    
    if (!doc.HasMember("nexus")) {
        ERROR_LOG("config error -- nexus not define\n");
        return false;
    } else {
        const Value& nexus = doc["nexus"];

        if (!nexus.HasMember("host")) {
            ERROR_LOG("config error -- nexus.host not define\n");
            return false;
        }
        nexusAddr_.host = nexus["host"].GetString();

        if (!nexus.HasMember("port")) {
            ERROR_LOG("config error -- nexus.port not define\n");
            return false;
        }
        nexusAddr_.port = nexus["port"].GetUint();
    }

    if (!doc.HasMember("designConfigManifest") || !doc["designConfigManifest"].IsString()) {
        ERROR_LOG("config error -- designConfigManifest string required (策划配置清单，相对本配置文件目录)\n");
        return false;
    }
    designConfigManifest_ = doc["designConfigManifest"].GetString();

    return true;
}
