#ifndef lobby_game_object_h
#define lobby_game_object_h

#include "demo_game_object.h"
#include "common.pb.h"

namespace demo {
    // 注意：此对象非线程安全
    class LobbyGameObject: public DemoGameObject {
    public:
        LobbyGameObject(UserId userId, RoleId roleId, ServerId serverId, uint32_t lToken, GameObjectManager *manager);
        virtual ~LobbyGameObject();

        virtual void update(uint64_t nowSec);
        virtual void onEnterGame();
    };
}

#endif /* lobby_game_object_h */