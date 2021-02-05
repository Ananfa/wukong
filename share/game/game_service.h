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

#ifndef game_service_h
#define game_service_h

#include "corpc_controller.h"
#include "game_service.pb.h"
#include "game_object_manager.h"

namespace wukong {

    class GameServiceImpl : public pb::GameService {
    public:
        GameServiceImpl(GameObjectManager *manager): _manager(manager) {}

        virtual void forwardIn(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::ForwardInRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done);
        
        virtual void enterGame(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::EnterGameRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done);

    private:
        GameObjectManager *_manager;
    };

}

#endif /* game_service_h */
