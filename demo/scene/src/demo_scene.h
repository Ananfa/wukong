#ifndef demo_scene_object_h
#define demo_scene_object_h

#include "scene.h"
#include "scene_manager.h"

using namespace wukong;

namespace demo {
	class DemoScene: public Scene {
	public:
        DemoScene(uint32_t defId, SceneType type, const std::string &sceneId, const std::string &sToken, SceneManager *manager);
        virtual ~DemoScene();

        virtual void update(timeval now); // 周期处理逻辑（注意：不要有产生协程切换的逻辑）
        virtual void onEnter(RoleId roleId); // 如：可以进行进入场景AOI通知
        virtual void onLeave(RoleId roleId); // 如：可以进行离开场景AOI通知
        virtual void onDestory();

	};
}

#endif /* demo_scene_object_h */
