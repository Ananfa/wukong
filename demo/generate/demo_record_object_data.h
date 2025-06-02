// This file is generated. Don't edit it

#ifndef demo_record_object_data_h
#define demo_record_object_data_h

#include "record_object_data.h"
#include "common.pb.h"
#include "demo.pb.h"
#include <map>
#include <string>

using namespace wukong;

namespace demo {
        
    class DemoRecordObjectData: public wukong::RecordObjectData {
    public:
        DemoRecordObjectData();
        virtual ~DemoRecordObjectData() {}

        virtual bool initData(const std::list<std::pair<std::string, std::string>> &datas);
        virtual void syncIn(const ::wukong::pb::SyncRequest* request);
        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas);
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas);
        
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

#endif /* demo_record_object_data_h */
