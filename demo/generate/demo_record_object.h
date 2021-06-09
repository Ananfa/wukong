// TODO：此文件通过工具生成

#ifndef demo_record_object_h
#define demo_record_object_h

#include "record_object.h"
#include "common.pb.h"
#include "demo.pb.h"
#include <map>
#include <string>
#include <memory>

using namespace wukong;

namespace demo {
        
    class DemoRecordObject: public wukong::RecordObject {
    public:
        DemoRecordObject(RoleId roleId, uint32_t rToken, RecordManager *manager);
        virtual ~DemoRecordObject() {}

        virtual bool initData(const std::string &data);
        virtual void syncIn(const ::wukong::pb::SyncRequest* request);
        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &profileDatas);
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas);
        
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

#endif /* demo_record_object_h */