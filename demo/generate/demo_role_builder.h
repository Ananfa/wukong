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
        std::string name_;
        uint32_t exp_;
        uint32_t lv_;
        demo::pb::Currency* currency_ = nullptr;
        std::map<uint32_t, demo::pb::Card*> card_map_;
        std::map<uint32_t, demo::pb::Pet*> pet_map_;
        demo::pb::SignInActivity* signinactivity_ = nullptr;
    };

}

#endif /* demo_role_builder_h */