// TODO：此文件通过工具生成
#include "demo_record_object.h"
#include "corpc_utils.h"

using namespace demo;

DemoRecordObject::DemoRecordObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &rToken, RecordObjectManager *manager): wukong::RecordObject(userId, roleId, serverId, rToken, manager) {
    // 设置属性初始值(基础类型可以有默认值)
    name_ = "";
    exp_ = 0;
    lv_ = 1;
    currency_ = new demo::pb::Currency;
    signinactivity_ = new demo::pb::SignInActivity;
}

bool DemoRecordObject::initData(const std::list<std::pair<std::string, std::string>> &datas) {
    for (auto &pair : datas) {
        if (pair.first.compare("name") == 0) {
            auto msg = new wukong::pb::StringValue;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--name failed\n", roleId_);
                delete msg;
                return false;
            }

            name_ = msg->value();
            delete msg;
        } else if (pair.first.compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--exp failed\n", roleId_);
                delete msg;
                return false;
            }

            exp_ = msg->value();
            delete msg;
        } else if (pair.first.compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--lv failed\n", roleId_);
                delete msg;
                return false;
            }

            lv_ = msg->value();
            delete msg;
        } else if (pair.first.compare("currency") == 0) {
            auto msg = new demo::pb::Currency;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--currency failed\n", roleId_);
                delete msg;
                return false;
            }

            delete currency_;
            currency_ = msg;
        } else if (pair.first.compare("card") == 0) {
            auto msg = new demo::pb::Cards;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--card failed\n", roleId_);
                delete msg;
                return false;
            }

            int cardNum = msg->cards_size();
            for (int j = 0; j < cardNum; j++) {
                auto card = new demo::pb::Card(msg->cards(j));
                card_map_.insert(std::make_pair(card->cardid(), card));
            }

            delete msg;
        } else if (pair.first.compare("pet") == 0) {
            auto msg = new demo::pb::Pets;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--pet failed\n", roleId_);
                delete msg;
                return false;
            }

            int petNum = msg->pets_size();
            for (int j = 0; j < petNum; j++) {
                auto pet = new demo::pb::Pet(msg->pets(j));
                pet_map_.insert(std::make_pair(pet->petid(), pet));
            }

            delete msg;
        } else if (pair.first.compare("signinactivity") == 0) {
            auto msg = new demo::pb::SignInActivity;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--signinactivity failed\n", roleId_);
                delete msg;
                return false;
            }

            delete signinactivity_;
            signinactivity_ = msg;
        }
    }

    return true;
}

void DemoRecordObject::syncIn(const ::wukong::pb::SyncRequest* request) {
    int dataNum = request->datas_size();

    for (int i = 0; i < dataNum; ++i) {
        auto &data = request->datas(i);

        if (data.key().compare("name") == 0) {
            auto msg = new wukong::pb::StringValue;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--name failed\n", roleId_);
                delete msg;
                continue;
            }

            name_ = msg->value();
            delete msg;
            dirty_map_["name"] = true;
        } else if (data.key().compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--exp failed\n", roleId_);
                delete msg;
                continue;
            }

            exp_ = msg->value();
            delete msg;
            dirty_map_["exp"] = true;
        } else if (data.key().compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--lv failed\n", roleId_);
                delete msg;
                continue;
            }

            lv_ = msg->value();
            delete msg;
            dirty_map_["lv"] = true;
        } else if (data.key().compare("currency") == 0) {
            auto msg = new demo::pb::Currency;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--currency failed\n", roleId_);
                delete msg;
                continue;
            }

            delete currency_;
            currency_ = msg;
            dirty_map_["currency"] = true;
        } else if (data.key().compare(0, 5, "card.") == 0) {
            std::string idStr = data.key().substr(5);
            uint32_t id = atoi(idStr.c_str());

            auto msg = new demo::pb::Card;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--card:%d failed\n", roleId_, id);
                delete msg;
                continue;
            }

            card_map_[id] = msg;
            dirty_map_["card"] = true;
        } else if (data.key().compare(0, 4, "pet.") == 0) {
            std::string idStr = data.key().substr(4);
            uint32_t id = atoi(idStr.c_str());

            auto msg = new demo::pb::Pet;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--pet:%d failed\n", roleId_, id);
                delete msg;
                continue;
            }

            pet_map_[id] = msg;
            dirty_map_["pet"] = true;
        } else if (data.key().compare("signinactivity") == 0) {
            auto msg = new demo::pb::SignInActivity;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--signinactivity failed\n", roleId_);
                delete msg;
                continue;
            }

            delete signinactivity_;
            signinactivity_ = msg;
            dirty_map_["signinactivity"] = true;
        } else {
            ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--unknown data: %s\n", roleId_, data.key().c_str());
        }
    }

    int removeNum = request->removes_size();
    for (int i = 0; i < removeNum; ++i) {
        auto &remove = request->removes(i);
        if (remove.compare(0, 5, "card.") == 0) {
            std::string idStr = remove.substr(5);
            uint32_t id = atoi(idStr.c_str());

            auto it = card_map_.find(id);
            if (it != card_map_.end()) {
                delete it->second;
                card_map_.erase(it);
                dirty_map_["card"] = true;
            } else {
                WARN_LOG("DemoRecordObject::syncIn -- remove role:%d data--card %d not exist\n", roleId_, id);
            }
        } else if (remove.compare(0, 4, "pet.") == 0) {
            std::string idStr = remove.substr(4);
            uint32_t id = atoi(idStr.c_str());

            auto it = pet_map_.find(id);
            if (it != pet_map_.end()) {
                delete it->second;
                pet_map_.erase(it);
                dirty_map_["pet"] = true;
            } else {
                WARN_LOG("DemoRecordObject::syncIn -- remove role:%d data--pet %d not exist\n", roleId_, id);
            }
        } else {
            ERROR_LOG("DemoRecordObject::syncIn -- remove role:%d data--unknown data: %s\n", roleId_, remove.c_str());
        }
    }
}

void DemoRecordObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas) {
    // 将脏数据打包
    for (auto &pair : dirty_map_) {
        if (pair.first.compare("name") == 0) {
            auto msg = new wukong::pb::StringValue;
            msg->set_value(name_);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(exp_);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(lv_);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("currency") == 0) {
            std::string msgData(currency_->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            currency_->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
        } else if (pair.first.compare("card") == 0) {
            auto msg = new demo::pb::Cards;
            for (auto &pair1 : card_map_) {
                auto card = msg->add_cards();
                *card = *(pair1.second);
            }

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("pet") == 0) {
            auto msg = new demo::pb::Pets;
            for (auto &pair1 : pet_map_) {
                auto pet = msg->add_pets();
                *pet = *(pair1.second);
            }

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("signinactivity") == 0) {
            std::string msgData(signinactivity_->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            signinactivity_->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
        }
    }
}

void DemoRecordObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
    // 将所有数据打包
    {
        auto msg = new wukong::pb::StringValue;
        msg->set_value(name_);

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("name", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new wukong::pb::Uint32Value;
        msg->set_value(exp_);

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("exp", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new wukong::pb::Uint32Value;
        msg->set_value(lv_);

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("lv", std::move(msgData)));
        delete msg;
    }

    {
        std::string msgData(currency_->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        currency_->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("currency", std::move(msgData)));
    }

    {
        auto msg = new demo::pb::Cards;
        for (auto &pair : card_map_) {
            auto card = msg->add_cards();
            *card = *(pair.second);
        }

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("card", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new demo::pb::Pets;
        for (auto pair : pet_map_) {
            auto pet = msg->add_pets();
            *pet = *(pair.second);
        }

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("pet", std::move(msgData)));
        delete msg;
    }

    {
        std::string msgData(signinactivity_->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        signinactivity_->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("signinactivity", std::move(msgData)));
    }

}