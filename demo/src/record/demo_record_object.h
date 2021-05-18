// TODO：此文件通过工具生成

#ifndef demo_record_object_h
#define demo_record_object_h

#include "record_object.h"
#include "common.pb.h"
#include "demoGame.pb.h"
#include <map>
#include <string>
#include <memory>

namespace demoGame {
        
    class DemoRecordObject: public wukong::RecordObject {
    public:
        DemoRecordObject(RoleId roleId, RecordObjectManager *manager);
        virtual ~DemoRecordObject() {}

        virtual bool initData(const std::string &data);

        virtual void syncIn(const ::wukong::pb::SyncRequest* request);

        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas);
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

#endif /* demo_record_object_h */