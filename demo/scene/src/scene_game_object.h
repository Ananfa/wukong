#ifndef scene_game_object_h
#define scene_game_object_h

#include "demo_game_object.h"
#include "common.pb.h"
#include "corpc_timer.h"

namespace demo {
    // 注意：此对象非线程安全
    // 最好能统一场景服和大厅服的GameObject实现
    class SceneGameObject: public DemoGameObject {
    public:
        SceneGameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager);
        virtual ~SceneGameObject();

        virtual void update(timeval now);
        virtual void onEnterGame();
        virtual void onOffline();

        virtual void onStart();
        virtual void onDestory();

        // 【测试代码】
        void onTestLocalEvent(const Event &e);
        void onTestLocalEvent1(uint32_t testValue);
        void onTestGlobalEvent(const Event &e);

    private:
        std::shared_ptr<Timer> leaveGameTimer_;
    };
}

#endif /* scene_game_object_h */