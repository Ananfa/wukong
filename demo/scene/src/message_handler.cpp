
#include "message_handler.h"
#include "game_center.h"
#include "scene_manager.h"
#include "common.pb.h"
#include "demo_const.h"

using namespace demo;

void MessageHandler::registerMessages() {
	g_GameCenter.registerMessage(C2S_MESSAGE_ID_ECHO, new wukong::pb::StringValue, false, EchoHandle);
	g_GameCenter.registerMessage(C2S_MESSAGE_ID_ENTERSCENE, new wukong::pb::Int32Value, true, EnterSceneHandle);
}

void MessageHandler::EchoHandle(std::shared_ptr<GameObject> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
	DEBUG_LOG("MessageHandler::EchoHandle\n");
	std::shared_ptr<SceneGameObject> realObj = std::dynamic_pointer_cast<SceneGameObject>(obj);
	std::shared_ptr<wukong::pb::StringValue> realMsg = std::dynamic_pointer_cast<wukong::pb::StringValue>(msg);

	DEBUG_LOG("MessageHandler::EchoHandle -- receive msg: %s\n", realMsg->value().c_str());

	// 加1点经验值
	realObj->setExp(realObj->getExp()+1);

	realObj->send(S2C_MESSAGE_ID_ECHO, tag, *realMsg);
}

void MessageHandler::EnterSceneHandle(std::shared_ptr<GameObject> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    DEBUG_LOG("MessageHandler::EnterSceneHandle\n");

    std::shared_ptr<SceneGameObject> realObj = std::dynamic_pointer_cast<SceneGameObject>(obj);
    std::shared_ptr<wukong::pb::Int32Value> realMsg = std::dynamic_pointer_cast<wukong::pb::Int32Value>(msg);

    // 若已经在场景中

    // 加1点经验值
    realObj->setExp(realObj->getExp()+1);

    SceneManager *manager = (SceneManager *)realObj->getManager();
    auto scene = manager->getScene(realObj->getSceneId());

    wukong::pb::Int32Value resp;
    resp.set_value(scene->getDefId());

    realObj->send(S2C_MESSAGE_ID_ENTERSCENE, 0, resp);
}