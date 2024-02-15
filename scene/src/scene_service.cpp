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

#include "scene_service.h"
#include "scene_config.h"
#include "scene_server.h"
#include "share/errcode.h"

using namespace wukong;

void SceneServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                              const ::corpc::Void* request,
                              ::corpc::Void* response,
                              ::google::protobuf::Closure* done) {
    traverseInnerStubs([](ServerId sid, pb::InnerSceneService_Stub *stub) -> bool {
        stub->shutdown(NULL, NULL, NULL, NULL);
        return true;
    });
}

void SceneServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                    const ::corpc::Void* request,
                                    ::wukong::pb::OnlineCounts* response,
                                    ::google::protobuf::Closure* done) {
    // 注意：这里处理中间会产生协程切换，遍历的Map可能在过程中被修改，因此traverseInnerStubs方法中对map进行了镜像复制
    traverseInnerStubs([request, response](ServerId sid, pb::InnerSceneService_Stub *stub) -> bool {
        pb::Uint32Value *resp = new pb::Uint32Value();
        corpc::Controller *ctl = new corpc::Controller();
        stub->getOnlineCount(ctl, request, resp, NULL);
        pb::OnlineCount *onlineCount = response->add_counts();
        onlineCount->set_serverid(sid);
        if (ctl->Failed()) {
            onlineCount->set_count(-1);
        } else {
            onlineCount->set_count(resp->value());
        }
        delete ctl;
        delete resp;

        return true;
    });
}

void SceneServiceImpl::loadScene(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LoadSceneRequest* request,
                             ::wukong::pb::LoadSceneResponse* response,
                             ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->loadScene(controller, request, response, done);
}

void SceneServiceImpl::enterScene(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::EnterSceneRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done) {
    // 注意：enterScene接口在pb接口定义中将delete_in_done选项设置为true，当done->Run()调用时才销毁controller和request
    auto stub = getInnerStub(request->serverid());
    stub->enterScene(controller, request, NULL, done);
}

void SceneServiceImpl::addInnerStub(ServerId sid, pb::InnerSceneService_Stub* stub) {
    innerStubs_.insert(std::make_pair(sid, stub));
}

pb::InnerSceneService_Stub *SceneServiceImpl::getInnerStub(ServerId sid) {
    auto it = innerStubs_.find(sid);

    if (it == innerStubs_.end()) {
        return nullptr;
    }

    return it->second;
}

void SceneServiceImpl::traverseInnerStubs(std::function<bool(ServerId, pb::InnerSceneService_Stub*)> handle) {
    for (auto &pair : innerStubs_) {
        if (!handle(pair.first, pair.second)) {
            return;
        }
    }
}

void InnerSceneServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                const ::corpc::Void* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    manager_->shutdown();
}

void InnerSceneServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                      const ::corpc::Void* request,
                                      ::wukong::pb::Uint32Value* response,
                                      ::google::protobuf::Closure* done) {
    response->set_value(manager_->roleCount());
}

void InnerSceneServiceImpl::loadScene(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LoadSceneRequest* request,
                             ::wukong::pb::LoadSceneResponse* response,
                             ::google::protobuf::Closure* done) {
    // 判断manager中场景是否已经存在
    if (!request->sceneid().empty() && manager_->existScene(request->sceneid())) {
        response->set_errcode(SCENE_ALREADY_EXIST);
        return;
    }
    
    std::string sceneId = manager_->loadScene(request->defid(), request->sceneid(), request->roleid(), request->teamid());
    if (sceneId.empty()) {
        response->set_errcode(LOAD_SCENE_FAILED);
        return;
    }
    
    response->set_sceneid(sceneId);
}

void InnerSceneServiceImpl::enterScene(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::EnterSceneRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done) {
    auto scene = manager_->getScene(request->sceneid());

    if (!scene) {
        ERROR_LOG("InnerSceneServiceImpl::enterScene -- scene[%s] not found\n", request->sceneid().c_str());
        return;
    }

    // 如果是单人场景或队伍场景，记录警告日志后退出。 单人场景和队伍场景中的角色是场景创建时一并加载的，不需要通过enterScene接口
    // 注意：队伍场景中人员变化时相当于解散旧队伍创建新队伍
    if (scene->getType() == SCENE_TYPE_SINGLE_PLAYER || scene->getType() == SCENE_TYPE_TEAM) {
        ERROR_LOG("InnerSceneServiceImpl::enterScene -- scene[%s] type invalid\n", request->sceneid().c_str());
        return;
    }

    RoleId roleId = request->roleid();
    // 在gameObjectManager中查询游戏对象，如果存在写错误日志并退出
    auto gameObj = manager_->getGameObject(roleId);
    if (gameObj) {
        ERROR_LOG("InnerSceneServiceImpl::enterScene -- gameObj[%d] already exist\n", roleId);
        return;
    }

    // 创建游戏对象，如果创建游戏对象失败，写错误日志并退出
    if (!manager_->loadRole(roleId, request->gatewayid())) {
        ERROR_LOG("InnerSceneServiceImpl::enterScene -- load role[%d] failed\n", roleId);
        return;
    }

    // 问题：游戏对象加载后到进入场景过程中是否会进入场景失败导致游戏对象悬空？加载角色对象完成时发现场景销毁了？遇到这种情况直接销毁游戏对象
    if (!scene->running_) {
        ERROR_LOG("InnerSceneServiceImpl::enterScene -- after loaded role[%d] scene stoped\n", roleId);
        gameObj->leaveGame();
        return;
    }

    gameObj = manager_->getGameObject(roleId);
    assert(gameObj);

    scene->enter(gameObj);
}
