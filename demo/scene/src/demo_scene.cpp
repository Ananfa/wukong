#include "demo_scene.h"
#include "scene_game_object.h"
#include "demo_const.h"

using namespace demo;

DemoScene::DemoScene(uint32_t defId, SceneType type, const std::string &sceneId, const std::string &sToken, SceneManager *manager): Scene(defId, type, sceneId, sToken, manager) {

}

DemoScene::~DemoScene() {

}

void DemoScene::update(timeval now) {

}

void DemoScene::onEnter(RoleId roleId) {
    auto it = roles_.find(roleId);
    assert(it != roles_.end());

    std::shared_ptr<SceneGameObject> realObj = std::dynamic_pointer_cast<SceneGameObject>(it->second);

    // 加1点经验值
    realObj->setExp(realObj->getExp()+1);

    wukong::pb::Int32Value resp;
    resp.set_value(defId_);

    realObj->send(S2C_MESSAGE_ID_ENTERSCENE, 0, resp);
}

void DemoScene::onLeave(RoleId roleId) {

}

void DemoScene::onDestory() {

}
