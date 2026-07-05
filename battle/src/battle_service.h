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

#ifndef wukong_battle_service_h
#define wukong_battle_service_h

#include "corpc_controller.h"
#include "battle_service.pb.h"

namespace wukong {

    class BattleServiceImpl : public pb::BattleService {
    public:
        virtual void requestBattleAssignment(::google::protobuf::RpcController* controller,
                       const ::wukong::pb::RequestBattleAssignmentRequest* request,
                       ::wukong::pb::RequestBattleAssignmentResponse* response,
                       ::google::protobuf::Closure* done) override;
        virtual void removeBattlePlayer(::google::protobuf::RpcController* controller,
                       const ::wukong::pb::RemoveBattlePlayerRequest* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done) override;
    };

}

#endif /* wukong_battle_service_h */
