// TODO：此文件通过工具生成

#ifndef demo_game_object_h
#define demo_game_object_h

#include "game_object.h"
#include "game_object_manager.h"
#include "common.pb.h"
#include "demo.pb.h"
#include <map>
#include <string>
#include <vector>

using namespace wukong;

namespace demo {
    // 注意：此对象非线程安全
    class DemoGameObject: public wukong::GameObject {
    public:
        DemoGameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager);
        virtual ~DemoGameObject();

        virtual bool initData(const std::string &data);

        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes);
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas);
        virtual void onEnterGame();

        const std::string& getName();
        void setName(const std::string& name);
        uint32_t getExp();
        void setExp(uint32_t exp);
        uint32_t getLv();
        void setLv(uint32_t lv);

        demo::pb::Currency* getCurrency();
        void setCurrencyDirty();

        std::vector<uint32_t> getAllCardKeys();
        bool hasCard(uint32_t cardid);
        uint32_t getCardNum();
        demo::pb::Card* getCard(uint32_t cardid);
        void setCardDirty(uint32_t cardid);
        void addCard(demo::pb::Card* card);
        void removeCard(uint32_t cardid);

        std::vector<uint32_t> getAllPetKeys();
        bool hasPet(uint32_t petid);
        uint32_t getPetNum();
        demo::pb::Pet* getPet(uint32_t petid);
        void setPetDirty(uint32_t petid);
        void addPet(demo::pb::Pet* pet);
        void removePet(uint32_t petid);

        demo::pb::SignInActivity* getSignInActivity();
        void setSignInActivityDirty();

    private:
        std::string _name;
        uint32_t _exp;
        uint32_t _lv;
        demo::pb::Currency* _currency = nullptr;
        std::map<uint32_t, demo::pb::Card*> _card_map;
        std::map<uint32_t, demo::pb::Pet*> _pet_map;
        demo::pb::SignInActivity* _signinactivity = nullptr;
    };

}

#endif /* demo_game_object_h */