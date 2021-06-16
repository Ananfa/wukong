
#include "message_handler.h"
#include "common.pb.h"

using namespace demo;

void MessageHandler::XXXHandle(std::shared_ptr<GameObject> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
	std::shared_ptr<LobbyGameObject> realObj = std::dynamic_pointer_cast<LobbyGameObject>(obj);
	std::shared_ptr<wukong::pb::Int32Value> realMsg = std::dynamic_pointer_cast<wukong::pb::Int32Value>(msg);

}