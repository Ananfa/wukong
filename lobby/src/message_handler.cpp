
#include "message_handler.h"
#include "message_handle_manager.h"
#include "lobby_object.h"
#include "redis_pool.h"
#include "redis_utils.h"
//#include "client_center.h"
#include "agent_manager.h"
#include "scene_agent.h"
#include "common.pb.h"
#include "demo_const.h"
#include "demo_errdef.h"
#include "demo_lobby_object_data.h"

using namespace demo;

void MessageHandler::registerMessages() {
    g_MessageHandleManager.registerMessage(C2S_MESSAGE_ID_ECHO, new wukong::pb::StringValue, false, EchoHandle);
    //g_MessageHandleManager.registerMessage(C2S_MESSAGE_ID_ENTERSCENE, new wukong::pb::Int32Value, true, EnterSceneHandle);
}

void MessageHandler::EchoHandle(std::shared_ptr<MessageTarget> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    DEBUG_LOG("MessageHandler::EchoHandle\n");
    std::shared_ptr<LobbyObject> realObj = std::dynamic_pointer_cast<LobbyObject>(obj);
    std::shared_ptr<wukong::pb::StringValue> realMsg = std::dynamic_pointer_cast<wukong::pb::StringValue>(msg);

    DEBUG_LOG("MessageHandler::EchoHandle -- receive msg: %s\n", realMsg->value().c_str());

    DemoLobbyObjectData *objData = (DemoLobbyObjectData *)realObj->getObjectData();
    // 加1点经验值
    objData->setExp(objData->getExp()+1);

    realObj->send(S2C_MESSAGE_ID_ECHO, tag, *realMsg);
}

/*
void MessageHandler::EnterSceneHandle(std::shared_ptr<MessageTarget> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    ERROR_LOG("MessageHandler::EnterSceneHandle\n");
    std::shared_ptr<LobbyObject> realObj = std::dynamic_pointer_cast<LobbyObject>(obj);
    std::shared_ptr<wukong::pb::Int32Value> realMsg = std::dynamic_pointer_cast<wukong::pb::Int32Value>(msg);

    // 校验场景定义号（假设目前只支持世界场景，定义号小于100）
    uint32_t defId = realMsg->value();
    if (defId > 100) {
        ERROR_LOG("MessageHandler::EnterSceneHandle -- unknown scene id: %d\n", defId);
        wukong::pb::Int32Value errMsg;
        errMsg.set_value(ERR_UNKNOWN_SCENE);
        realObj->send(S2C_MESSAGE_ID_ERROR, tag, errMsg);
        return;
    }

    // 判断是否允许进入目标场景
    if (!realObj->canEnterScene(defId)) {
        ERROR_LOG("MessageHandler::EnterSceneHandle -- forbit enter scene id: %d\n", defId);
        wukong::pb::Int32Value errMsg;
        errMsg.set_value(ERR_FORBIT_SCENE);
        realObj->send(S2C_MESSAGE_ID_ERROR, tag, errMsg);
        return;
    }

    // 离开原场景，进入新场景
    ServerId orgSceneServerId = 0;
    std::string orgSceneId;

    realObj->getSceneAddr(orgSceneServerId, orgSceneId);

    // 查找场景所在
    char *buf = new char[10];
    sprintf(buf, "GS_%d", defId);
    std::string sceneId = buf;

    if (orgSceneId == sceneId) {
        WARN_LOG("MessageHandler::EnterSceneHandle already in scene:%s\n", sceneId.c_str());
        wukong::pb::Int32Value errMsg;
        errMsg.set_value(ERR_ALREADY_IN_SCENE);
        realObj->send(S2C_MESSAGE_ID_ERROR, tag, errMsg);
        return;
    }

    SceneAgent *sceneAgent = (SceneAgent*)g_AgentManager.getAgent(SERVER_TYPE_SCENE);
    if (!orgSceneId.empty()) {
        // 离开原场景
        int32_t err = sceneAgent->leaveScene(orgSceneServerId, orgSceneId, realObj->getRoleId());
        if (err != 0) {
            WARN_LOG("MessageHandler::EnterSceneHandle leave origin scene:%s error:%d\n", orgSceneId.c_str(), err);
        }
    }

    ServerId sceneServerId;
    // 加载场景失败时等待1秒重新尝试，3次失败才返回失败
    for (int i = 0; i < 3; i++) {
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("MessageHandler::EnterSceneHandle -- connect to cache failed\n");
            wukong::pb::Int32Value errMsg;
            errMsg.set_value(ERR_SERVER_ERROR);
            realObj->send(S2C_MESSAGE_ID_ERROR, tag, errMsg);
            return;
        }

        RedisAccessResult result = wukong::RedisUtils::GetSceneAddress(cache, sceneId, sceneServerId);
        if (result == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("MessageHandler::EnterSceneHandle -- get location failed for db error\n");
            wukong::pb::Int32Value errMsg;
            errMsg.set_value(ERR_SERVER_ERROR);
            realObj->send(S2C_MESSAGE_ID_ERROR, tag, errMsg);
            return;
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);

        if (result == REDIS_FAIL) { // 场景不存在，加载场景
            // 负载均衡找一个场景服
            if (!sceneAgent->randomServer(sceneServerId)) {
                ERROR_LOG("MessageHandler::EnterSceneHandle -- random scene server failed\n");
                wukong::pb::Int32Value errMsg;
                errMsg.set_value(ERR_SERVER_ERROR);
                realObj->send(S2C_MESSAGE_ID_ERROR, tag, errMsg);
                return;
            }

            // 通知Scene服加载场景对象
            std::string tmpId = sceneAgent->loadScene(sceneServerId, defId, sceneId, 0, "");
            if (tmpId.empty()) {
                // 加载失败，可能有其他玩家在同时加载场景，等待1秒后重新查询
                sleep(1); // 1秒后重试
                continue;
            }

            assert(tmpId == sceneId);
        }

        break;
    }

    // 注意：进入场景不需要离开大厅服
    // 离开大厅服（注意：这里将gameobj销毁后其中的_gatewayServerStub还存在，因此在出错时还能发消息给客户端）
    //realObj->leaveGame();

    // 进入场景
    sceneAgent->enterScene(sceneServerId, sceneId, realObj->getRoleId(), realObj->getGatewayServerId());
}
*/