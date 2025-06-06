/*
 * Created by Xianke Liu on 2021/1/19.
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
#if 0
#ifndef wukong_game_service_h
#define wukong_game_service_h

#include "corpc_controller.h"
#include "game_service.pb.h"
#include "game_object_manager.h"

namespace wukong {

    class GameServiceImpl : public pb::GameService {
    public:
        virtual void forwardIn(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::ForwardInRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done);
        
        virtual void enterGame(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::EnterGameRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done);

        void addInnerStub(ServerId sid, pb::InnerGameService_Stub* stub);

    private:
        pb::InnerGameService_Stub *getInnerStub(ServerId sid);
        void traverseInnerStubs(std::function<bool(ServerId, pb::InnerGameService_Stub*)> handle);

    private:
        std::map<ServerId, pb::InnerGameService_Stub*> innerStubs_; // 注意：该map只在系统启动时初始化，启动后不再修改
    };

    class InnerGameServiceImpl : public pb::InnerGameService {
    public:
        InnerGameServiceImpl(GameObjectManager *manager): manager_(manager) {}

        virtual void forwardIn(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::ForwardInRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done);
        
        virtual void enterGame(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::EnterGameRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done);

    private:
        GameObjectManager *manager_;
    };

}

#endif /* wukong_game_service_h */
#endif