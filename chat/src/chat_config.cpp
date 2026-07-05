/*
 * Created by Xianke Liu on 2025/5/21.
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

#include "chat_config.h"
#include "corpc_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "const.h"
#include <cstdio>
#include <map>
#include <stdlib.h>

using namespace rapidjson;
using namespace wukong;

bool ChatConfig::parse(const char *path) {
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

    return true;
}
