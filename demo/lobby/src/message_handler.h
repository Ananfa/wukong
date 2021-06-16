#ifndef message_handle_h
#define message_handle_h

#include "lobby_game_object.h"
#include "common.pb.h"

using namespace wukong;

namespace demo {
	class MessageHandler {
	public:
		static void registerMessages();

		static void EchoHandle(std::shared_ptr<GameObject> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg);
	};
}

#endif /* message_handle_h */