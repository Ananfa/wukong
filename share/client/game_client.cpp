/*
 * Created by Xianke Liu on 2021/6/8.
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

#include "game_client.h"
#include "string_utils.h"

using namespace wukong;

bool GameClient::parseAddress(const std::string &input, AddressInfo &addressInfo) {
    std::vector<std::string> output;
    StringUtils::split(input, "|", output);

    int outputSize = output.size();
    if (outputSize < 2) return false;

    std::vector<std::string> output1;
    StringUtils::split(output[0], ":", output1);
    if (output1.size() < 2 || output1.size() > 3) return false;
    addressInfo.ip = output1[0];
    addressInfo.port = std::stoi(output1[1]);
    if (output1.size() == 3) {
        addressInfo.type = std::stoi(output1[2]);
    } else {
        addressInfo.type = 0;
    }
    addressInfo.serverIds.reserve(outputSize-1);
    for (int i = 1; i < outputSize; i++) {
        addressInfo.serverIds.push_back(std::stoi(output[i]));
    }

    return true;
}