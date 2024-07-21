/*
 * Created by Xianke Liu on 2024/7/6.
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

#ifndef wukong_server_object_h
#define wukong_server_object_h

#include "define.h"
#include "corpc_message_terminal.h"

#include "inner_common.pb.h"

namespace wukong {

    class ServerObject {
    public:
    	ServerObject(const pb::ServerInfo& info): info_(info) {}
    	~ServerObject() {}

        void resetConn() { conn_.reset(); }
        void setConn(std::shared_ptr<MessageTerminal::Connection> &conn) { conn_ = conn; }
        std::shared_ptr<corpc::MessageTerminal::Connection> &getConn() { return conn_; }

        ServerType getType() { return info_.server_type(); }
        ServerId getId() { return info_.server_id(); }

        void setInfo(const pb::ServerInfo& info) { info_ = info; }
        const pb::ServerInfo &getInfo() { return info_; }

        void send(int16_t type, std::shared_ptr<google::protobuf::Message> msg);

    private:
        pb::ServerInfo info_;

        std::shared_ptr<MessageTerminal::Connection> conn_; // 客户端连接
    };

}

#endif /* wukong_server_manager_h */
