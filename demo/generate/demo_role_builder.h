// TODO：此文件通过工具生成

#ifndef demo_role_builder_h
#define demo_role_builder_h

#include "common.pb.h"
#include "demo.pb.h"
#include <list>
#include <map>
#include <string>

using namespace wukong;

namespace demo {
        
    class DemoRoleBuilder {
    public:
        DemoRoleBuilder();
        ~DemoRoleBuilder();

        void buildDatas(std::list<std::pair<std::string, std::string>> &datas);
        
        void setName(const std::string& name);
        void setExp(uint32_t exp);
        void setLv(uint32_t lv);

        demo::pb::Currency* getCurrency();

        void addCard(demo::pb::Card* card);

        void addPet(demo::pb::Pet* pet);

        demo::pb::SignInActivity* getSignInActivity();

    private:
        std::string _name;
        uint32_t _exp;
        uint32_t _lv;
        demo::pb::Currency* _currency;
        std::map<uint32_t, demo::pb::Card*> _card_map;
        std::map<uint32_t, demo::pb::Pet*> _pet_map;
        demo::pb::SignInActivity* _signinactivity;
    };

}

#endif /* demo_role_builder_h */