// This file is generated. Don't edit it

#ifndef demo_lobby_object_data_h
#define demo_lobby_object_data_h

#include "demo.pb.h"
#include "lobby_object_data.h"
#include "common.pb.h"
#include <map>
#include <string>
#include <vector>

using namespace wukong;

namespace demo {
    class DemoLobbyObjectData: public wukong::LobbyObjectData {
    public:
        DemoLobbyObjectData();
        virtual ~DemoLobbyObjectData();

        virtual bool initData(const std::string &data);

        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes);
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas);

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
        demo::pb::SignInActivity* getSigninactivity();
        void setSigninactivityDirty();

    private:
        std::string name_;
        uint32_t exp_;
        uint32_t lv_;
        demo::pb::Currency* currency_;
        std::map<uint32_t, demo::pb::Card*> card_map_;
        std::map<uint32_t, demo::pb::Pet*> pet_map_;
        demo::pb::SignInActivity* signinactivity_;
    };
}

#endif /* demo_lobby_object_data_h */
