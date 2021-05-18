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

#include "record_service.h"
#include "record_config.h"
#include "record_center.h"
#include "share/const.h"

using namespace wukong;

void RecordServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                const ::corpc::Void* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    _manager->shutdown();
}

void RecordServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                      const ::corpc::Void* request,
                                      ::wukong::pb::Uint32Value* response,
                                      ::google::protobuf::Closure* done) {
    response->set_value(_manager->size());
}

void RecordServiceImpl::loadRole(::google::protobuf::RpcController* controller,
                                 const ::wukong::pb::LoadRoleRequest* request,
                                 ::wukong::pb::LoadRoleResponse* response,
                                 ::google::protobuf::Closure* done) {
    if (_manager->isShutdown()) {
        response->set_errcode(1);
        return;
    }

    // TODO: 加载玩家数据
    // 先从redis加载玩家数据
}

void RecordServiceImpl::sync(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::SyncRequest* request,
                             ::wukong::pb::BoolValue* response,
                             ::google::protobuf::Closure* done) {
    // TODO: 同步数据
    // 将收到的脏数据修改本地数据，并且同步到redis的完整数据和待落地数据中
}

void RecordServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                                  const ::wukong::pb::RSHeartbeatRequest* request,
                                  ::wukong::pb::BoolValue* response,
                                  ::google::protobuf::Closure* done) {
    std::shared_ptr<RecordObject> obj = _manager->getRecordObject(request->roleid());
    if (!obj) {
        ERROR_LOG("RecordServiceImpl::heartbeat -- record object not found\n");
        return;
    }

    if (obj->getLToken() == request->ltoken()) {
        ERROR_LOG("RecordServiceImpl::heartbeat -- ltoken not match\n");
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    obj->_gameObjectHeartbeatExpire = t.tv_sec + 60;
    response->set_value(true);
}
