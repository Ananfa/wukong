
#include "message_handler.h"
#include "game_center.h"
#include "common.pb.h"

using namespace demo;

void MessageHandler::registerMessages() {
	g_GameCenter.registerMessage(1000, new wukong::pb::StringValue, false, EchoHandle);
}

void MessageHandler::EchoHandle(std::shared_ptr<GameObject> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
	DEBUG_LOG("MessageHandler::EchoHandle\n");
	std::shared_ptr<LobbyGameObject> realObj = std::dynamic_pointer_cast<LobbyGameObject>(obj);
	std::shared_ptr<wukong::pb::StringValue> realMsg = std::dynamic_pointer_cast<wukong::pb::StringValue>(msg);

	DEBUG_LOG("MessageHandler::EchoHandle -- receive msg: %s\n", realMsg->value().c_str());

	// 加1点经验值
	realObj->setExp(realObj->getExp()+1);

	obj->send(1000, tag, *realMsg);
}