#ifndef message_handle_h
#define message_handle_h

#include "scene_game_object.h"
#include "common.pb.h"

using namespace wukong;

namespace demo {
    // 注意：应该每个功能模块有各自的MessageHandler实现
    class MessageHandler {
    public:
        static void registerMessages();

        static void EchoHandle(std::shared_ptr<GameObject> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg);
        
        static void EnterSceneHandle(std::shared_ptr<GameObject> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg);
    };
}

#endif /* message_handle_h */