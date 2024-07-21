/*
 * Created by Xianke Liu on 2024/7/5.
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

#include "nexus_config.h"
#include "corpc_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "const.h"
#include <cstdio>
#include <map>
#include <stdlib.h>

using namespace rapidjson;
using namespace wukong;

bool NexusConfig::parse(const char *path) {
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

    if (!doc.HasMember("accessTimeout")) {
        ERROR_LOG("config error -- accessTimeout not define\n");
        return false;
    }
    accessTimeout_ = doc["accessTimeout"].GetUint();
    
    if (!doc.HasMember("disconnectTimeout")) {
        ERROR_LOG("config error -- disconnectTimeout not define\n");
        return false;
    }
    disconnectTimeout_ = doc["disconnectTimeout"].GetUint();
    
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

    if (!doc.HasMember("concern")) {
        ERROR_LOG("config error -- concern not define\n");
        return false;
    }

    const Value& concern = doc["concern"];
    if (!concern.IsArray()) {
        ERROR_LOG("config error -- concern not array\n");
        return false;
    }

    for (SizeType i = 0; i < concern.Size(); i++) {
        const Value& concernItem = concern[i];

        if (!concernItem.HasMember("serverType")) {
            ERROR_LOG("config error -- concern[%d] serverType not define\n", i);
            return false;
        }
        int32_t serverType = concernItem["serverType"].GetInt();

        if (concern_map_.find(serverType) != concern_map_.end()) {
            ERROR_LOG("config error -- concern serverType[%d] is repeated\n", serverType);
            return false;
        }

        if (!concernItem.HasMember("concernServerTypes")) {
            ERROR_LOG("config error -- concern[%d] concernServerTypes not define\n", i);
            return false;
        }

        const Value& concernServerTypes = doc["concernServerTypes"];
        if (!concernServerTypes.IsArray()) {
            ERROR_LOG("config error -- concern[%d] concernServerTypes not array\n", i);
            return false;
        }

        for (SizeType j = 0; j < concernServerTypes.Size(); j++) {
            int32_t concernServerType = concernServerTypes[j].GetInt();

            concern_map_[serverType].insert(concernServerType);
            be_concern_map_[concernServerType].insert(serverType);
        }
    }
    
    return true;
}

const std::set<uint32_t> &NexusConfig::getConcerns(uint32_t server_type) {
    auto it = concern_map_.find(server_type);
    if (it != concern_map_.end()) {
        return it->second;
    }

    return empty_concern_set_;
}

const std::set<uint32_t> &NexusConfig::getBeConcerns(uint32_t server_type) {
    auto it = be_concern_map_.find(server_type);
    if (it != be_concern_map_.end()) {
        return it->second;
    }

    return empty_concern_set_;
}
