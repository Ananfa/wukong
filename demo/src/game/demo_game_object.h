// TODO：此文件通过工具生成

#ifndef demo_game_object_h
#define demo_game_object_h

#include "game_object.h"
#include "common.pb.h"
#include "demoGame.pb.h"
#include <map>
#include <string>
#include <vector>

namespace demo {
    // 注意：此对象非线程安全
    class DemoGameObject: public wukong::GameObject {
    public:
        DemoGameObject(UserId userId, RoleId roleId, uint32_t lToken, GameObjectManager *manager);
        virtual ~DemoGameObject();

        virtual bool initData(const std::string &data);

        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes);

        const std::string& getName();
        void setName(const std::string& name);
        uint32_t getExp();
        void setExp(uint32_t exp);
        uint32_t getLv();
        void setLv(uint32_t lv);

        demoGame::pb::Currency* getCurrency();
        void setCurrencyDirty();

        std::vector<uint32_t> getAllCardKeys();
        bool hasCard(uint32_t cardid);
        uint32_t getCardNum();
        demoGame::pb::Card* getCard(uint32_t cardid);
        void setCardDirty(uint32_t cardid);
        void addCard(demoGame::pb::Card* card);
        void removeCard(uint32_t cardid);

        std::vector<uint32_t> getAllPetKeys();
        bool hasPet(uint32_t petid);
        uint32_t getPetNum();
        demoGame::pb::Pet* getPet(uint32_t petid);
        void setPetDirty(uint32_t petid);
        void addPet(demoGame::pb::Pet* pet);
        void removePet(uint32_t petid);

        demoGame::pb::SignInActivity* getSignInActivity();
        void setSignInActivityDirty();

    private:
        std::string _name;
        uint32_t _exp;
        uint32_t _lv;
        demoGame::pb::Currency* _currency;
        std::map<uint32_t, demoGame::pb::Card*> _card_map;
        std::map<uint32_t, demoGame::pb::Pet*> _pet_map;
        demoGame::pb::SignInActivity* _signinactivity;

    };

}

#endif /* demo_game_object_h */