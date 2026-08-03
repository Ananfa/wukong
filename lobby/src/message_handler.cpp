
#include "message_handler.h"
#include "message_handle_manager.h"
#include "lobby_object.h"
#include "lobby_config.h"
#include "share/const.h"
#include "redis_pool.h"
#include "redis_utils.h"
//#include "client_center.h"
#include "agent_manager.h"
#include "scene_agent.h"
#include "battle_agent.h"
#include "common.pb.h"
#include "game.pb.h"
#include "battle_sync.pb.h"
#include "battle_service.pb.h"
#include "demo_const.h"
#include "demo_errdef.h"
#include "demo_lobby_object_data.h"

using namespace demo;

void MessageHandler::registerMessages() {
    g_MessageHandleManager.registerMessage(C2S_MESSAGE_ID_ECHO, new wukong::pb::StringValue, false, EchoHandle);
    g_MessageHandleManager.registerMessage(C2S_MESSAGE_ID_START_BATTLE, new wukong::pb::StartBattleRequest, false, StartBattleHandle);
    g_MessageHandleManager.registerMessage(C2S_MESSAGE_ID_LEAVE_GAME, new wukong::pb::LeaveGameRequest, false, LeaveGameHandle);
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

void MessageHandler::StartBattleHandle(std::shared_ptr<MessageTarget> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    std::shared_ptr<LobbyObject> realObj = std::dynamic_pointer_cast<LobbyObject>(obj);
    std::shared_ptr<wukong::pb::StartBattleRequest> realMsg = std::dynamic_pointer_cast<wukong::pb::StartBattleRequest>(msg);
    if (!realObj || !realMsg) {
        return;
    }

    auto *battleAgent = static_cast<BattleAgent *>(g_AgentManager.getAgent(SERVER_TYPE_BATTLE));
    if (!battleAgent) {
        ERROR_LOG("MessageHandler::StartBattleHandle -- no battle agent\n");
        wukong::pb::Int32Value err;
        err.set_value(ERR_SERVER_ERROR);
        realObj->send(S2C_MESSAGE_ID_ERROR, tag, err);
        return;
    }

    // 若玩家仍处于旧战斗状态（已在战斗/等待进战斗），先通知旧 battle 服清场，避免重复占位。
    if (realObj->isInBattle() || realObj->isWaitingBattleKcp()) {
        if (realObj->getBattleServerId() != 0 && realObj->getBattleRoomId() != 0) {
            wukong::pb::RemoveBattlePlayerRequest rmReq;
            rmReq.set_room_id(realObj->getBattleRoomId());
            rmReq.set_role_id(realObj->getRoleId());
            battleAgent->removeBattlePlayer(realObj->getBattleServerId(), rmReq);
        }
        realObj->clearBattleStateFromBattleServer();
    }

    ServerId battleSid = 0;
    if (!battleAgent->randomServer(battleSid)) {
        wukong::pb::Int32Value err;
        err.set_value(ERR_NO_BATTLE_SERVER);
        realObj->send(S2C_MESSAGE_ID_ERROR, tag, err);
        return;
    }

    wukong::pb::RequestBattleAssignmentRequest req;
    // 多人：按阵营空位匹配已有房间或新建；开战刷 AI，玩家接管
    req.set_mode(wukong::pb::BATTLE_ROOM_MODE_MULTI);
    req.set_battle_def_id(realMsg->battle_def_id());
    req.set_lobby_server_id(static_cast<int32_t>(g_LobbyConfig.getId()));
    wukong::pb::BattlePlayerInitData *pd = req.mutable_player();
    pd->set_role_id(realObj->getRoleId());
    pd->set_faction(realMsg->faction());
    // combat_payload 由业务序列化角色战斗属性后填入

    wukong::pb::RequestBattleAssignmentResponse rsp;
    if (!battleAgent->requestBattleAssignment(battleSid, req, &rsp)) {
        wukong::pb::Int32Value err;
        err.set_value(ERR_BATTLE_CREATE_FAILED);
        realObj->send(S2C_MESSAGE_ID_ERROR, tag, err);
        return;
    }

    if (rsp.access().session_token().empty() || rsp.access().role_id() != realObj->getRoleId()) {
        wukong::pb::Int32Value err;
        err.set_value(ERR_BATTLE_CREATE_FAILED);
        realObj->send(S2C_MESSAGE_ID_ERROR, tag, err);
        return;
    }

    realObj->setWaitingEnterBattle(battleSid, rsp.room_id(), realMsg->battle_def_id(), rsp.kcp_host(), rsp.kcp_port());

    wukong::pb::BattleEnterInfo enter;
    enter.set_kcp_host(rsp.kcp_host());
    enter.set_kcp_port(rsp.kcp_port());
    enter.set_room_id(rsp.room_id());
    enter.set_battle_server_id(battleSid);
    enter.set_session_token(rsp.access().session_token());

    realObj->send(S2C_MESSAGE_ID_BATTLE_ENTER, tag, enter);
}

void MessageHandler::LeaveGameHandle(std::shared_ptr<MessageTarget> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    (void)tag;
    std::shared_ptr<LobbyObject> realObj = std::dynamic_pointer_cast<LobbyObject>(obj);
    std::shared_ptr<wukong::pb::LeaveGameRequest> realMsg = std::dynamic_pointer_cast<wukong::pb::LeaveGameRequest>(msg);
    if (!realObj || !realMsg) {
        return;
    }
    realObj->leaveGame();
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