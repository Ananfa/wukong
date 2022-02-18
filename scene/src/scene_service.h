/*
 * Created by Xianke Liu on 2022/1/11.
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

#ifndef scene_service_h
#define scene_service_h

#include "corpc_controller.h"
#include "scene_service.pb.h"
#include "scene_manager.h"

namespace wukong {

    class SceneServiceImpl : public pb::SceneService {
    public:
        virtual void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::corpc::Void* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::corpc::Void* request,
                             ::wukong::pb::OnlineCounts* response,
                             ::google::protobuf::Closure* done);
        virtual void loadScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::wukong::pb::LoadSceneRequest* request,
                             ::wukong::pb::LoadSceneResponse* response,
                             ::google::protobuf::Closure* done);
        virtual void enterScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::wukong::pb::EnterSceneRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);

        void addInnerStub(ServerId sid, pb::InnerRecordService_Stub* stub);

    private:
        pb::InnerSceneService_Stub *getInnerStub(ServerId sid);
        void traverseInnerStubs(std::function<bool(ServerId, pb::InnerSceneService_Stub*)> handle);

    private:
        std::map<ServerId, pb::InnerSceneService_Stub*> _innerStubs; // 注意：该map只在系统启动时初始化，启动后不再修改
    };
    
    class InnerSceneServiceImpl : public pb::InnerSceneService {
    public:
        InnerSceneServiceImpl(SceneManager *sceneManager, GameObjectManager *gameObjectManager): _sceneManager(sceneManager), _gameObjectManager(gameObjectManager) {}

        virtual void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::corpc::Void* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::corpc::Void* request,
                             ::wukong::pb::Uint32Value* response,
                             ::google::protobuf::Closure* done);
        virtual void loadScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::wukong::pb::LoadSceneRequest* request,
                             ::wukong::pb::LoadSceneResponse* response,
                             ::google::protobuf::Closure* done);
        virtual void enterScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::wukong::pb::EnterSceneRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);

    private:
        SceneManager *_sceneManager;
        GameObjectManager *_gameObjectManager;
    };
    
}

#endif /* scene_service_h */
