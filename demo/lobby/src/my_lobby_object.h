#ifndef my_lobby_object_h
#define my_lobby_object_h

#include "demo_lobby_object.h"
#include "common.pb.h"
#include "corpc_timer.h"

namespace demo {
    // 注意：此对象非线程安全
    class MyLobbyObject: public DemoLobbyObject {
    public:
        MyLobbyObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken);
        virtual ~MyLobbyObject();

        virtual void update(timeval now);
        virtual void onEnterGame();
        virtual void onOffline();

        virtual void onStart();
        virtual void onDestory();

        // 【测试代码】
        void onTestLocalEvent(const Event &e);
        void onTestLocalEvent1(uint32_t testValue);
        //void onTestGlobalEvent(const Event &e);

    private:
    	std::shared_ptr<Timer> leaveGameTimer_;
    };
}

#endif /* my_lobby_object_h */