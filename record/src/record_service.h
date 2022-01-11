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

#ifndef record_service_h
#define record_service_h

#include "corpc_controller.h"
#include "record_service.pb.h"
#include "record_manager.h"

namespace wukong {

    class RecordServiceImpl : public pb::RecordService {
    public:
        virtual void shutdown(::google::protobuf::RpcController* controller,
                              const ::corpc::Void* request,
                              ::corpc::Void* response,
                              ::google::protobuf::Closure* done);

        virtual void getOnlineCount(::google::protobuf::RpcController* controller,
                                    const ::corpc::Void* request,
                                    ::wukong::pb::OnlineCounts* response,
                                    ::google::protobuf::Closure* done);

        virtual void loadRoleData(::google::protobuf::RpcController* controller,
                                  const ::wukong::pb::LoadRoleDataRequest* request,
                                  ::wukong::pb::LoadRoleDataResponse* response,
                                  ::google::protobuf::Closure* done);

        virtual void sync(::google::protobuf::RpcController* controller,
                          const ::wukong::pb::SyncRequest* request,
                          ::wukong::pb::BoolValue* response,
                          ::google::protobuf::Closure* done);

        virtual void heartbeat(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::RSHeartbeatRequest* request,
                               ::wukong::pb::BoolValue* response,
                               ::google::protobuf::Closure* done);

        void addInnerStub(ServerId sid, pb::InnerRecordService_Stub* stub);

    private:
        pb::InnerRecordService_Stub *getInnerStub(ServerId sid);
        void traverseInnerStubs(std::function<bool(ServerId, pb::InnerRecordService_Stub*)> handle);

    private:
        std::map<ServerId, pb::InnerRecordService_Stub*> _innerStubs; // 注意：该map只在系统启动时初始化，启动后不再修改
    };
    
    class InnerRecordServiceImpl : public pb::InnerRecordService {
    public:
        InnerRecordServiceImpl(RecordManager *manager): _manager(manager) {}

        virtual void shutdown(::google::protobuf::RpcController* controller,
                              const ::corpc::Void* request,
                              ::corpc::Void* response,
                              ::google::protobuf::Closure* done);

        virtual void getOnlineCount(::google::protobuf::RpcController* controller,
                                    const ::corpc::Void* request,
                                    ::wukong::pb::Uint32Value* response,
                                    ::google::protobuf::Closure* done);

        virtual void loadRoleData(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LoadRoleDataRequest* request,
                             ::wukong::pb::LoadRoleDataResponse* response,
                             ::google::protobuf::Closure* done);

        virtual void sync(::google::protobuf::RpcController* controller,
                          const ::wukong::pb::SyncRequest* request,
                          ::wukong::pb::BoolValue* response,
                          ::google::protobuf::Closure* done);

        virtual void heartbeat(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::RSHeartbeatRequest* request,
                               ::wukong::pb::BoolValue* response,
                               ::google::protobuf::Closure* done);

    private:
        RecordManager *_manager;
    };
    
}

#endif /* record_service_h */
