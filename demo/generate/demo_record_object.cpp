// TODO：此文件通过工具生成
#include "demo_record_object.h"
#include "corpc_utils.h"

using namespace demo;

DemoRecordObject::DemoRecordObject(RoleId roleId, ServerId serverId, uint32_t rToken, RecordManager *manager): wukong::RecordObject(roleId, serverId, rToken, manager) {
    // 设置属性初始值(基础类型可以有默认值)
    _name = "";
    _exp = 0;
    _lv = 1;
    _currency = new demo::pb::Currency;
    _signinactivity = new demo::pb::SignInActivity;
}

bool DemoRecordObject::initData(std::list<std::pair<std::string, std::string>> &datas) {
    for (auto &pair : datas) {
        if (pair.first.compare("name") == 0) {
            auto msg = new wukong::pb::StringValue;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--name failed\n", _roleId);
                delete msg;
                return false;
            }

            _name = msg->value();
            delete msg;
        } else if (pair.first.compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--exp failed\n", _roleId);
                delete msg;
                return false;
            }

            _exp = msg->value();
            delete msg;
        } else if (pair.first.compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--lv failed\n", _roleId);
                delete msg;
                return false;
            }

            _lv = msg->value();
            delete msg;
        } else if (pair.first.compare("currency") == 0) {
            auto msg = new demo::pb::Currency;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--currency failed\n", _roleId);
                delete msg;
                return false;
            }

            delete _currency;
            _currency = msg;
        } else if (pair.first.compare("card") == 0) {
            auto msg = new demo::pb::Cards;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--card failed\n", _roleId);
                delete msg;
                return false;
            }

            int cardNum = msg->cards_size();
            for (int j = 0; j < cardNum; j++) {
                auto card = new demo::pb::Card(msg->cards(j));
                _card_map.insert(std::make_pair(card->cardid(), card));
            }

            delete msg;
        } else if (pair.first.compare("pet") == 0) {
            auto msg = new demo::pb::Pets;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--pet failed\n", _roleId);
                delete msg;
                return false;
            }

            int petNum = msg->pets_size();
            for (int j = 0; j < petNum; j++) {
                auto pet = new demo::pb::Pet(msg->pets(j));
                _pet_map.insert(std::make_pair(pet->petid(), pet));
            }

            delete msg;
        } else if (pair.first.compare("signinactivity") == 0) {
            auto msg = new demo::pb::SignInActivity;
            if (!msg->ParseFromString(pair.second)) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--signinactivity failed\n", _roleId);
                delete msg;
                return false;
            }

            delete _signinactivity;
            _signinactivity = msg;
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
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--name failed\n", _roleId);
                delete msg;
                continue;
            }

            _name = msg->value();
            delete msg;
            _dirty_map["name"] = true;
        } else if (data.key().compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--exp failed\n", _roleId);
                delete msg;
                continue;
            }

            _exp = msg->value();
            delete msg;
            _dirty_map["exp"] = true;
        } else if (data.key().compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--lv failed\n", _roleId);
                delete msg;
                continue;
            }

            _lv = msg->value();
            delete msg;
            _dirty_map["lv"] = true;
        } else if (data.key().compare("currency") == 0) {
            auto msg = new demo::pb::Currency;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--currency failed\n", _roleId);
                delete msg;
                continue;
            }

            delete _currency;
            _currency = msg;
            _dirty_map["currency"] = true;
        } else if (data.key().compare(0, 5, "card.") == 0) {
            std::string idStr = data.key().substr(5);
            uint32_t id = atoi(idStr.c_str());

            auto msg = new demo::pb::Card;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--card:%d failed\n", _roleId, id);
                delete msg;
                continue;
            }

            _card_map[id] = msg;
            _dirty_map["card"] = true;
        } else if (data.key().compare(0, 4, "pet.") == 0) {
            std::string idStr = data.key().substr(4);
            uint32_t id = atoi(idStr.c_str());

            auto msg = new demo::pb::Pet;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--pet:%d failed\n", _roleId, id);
                delete msg;
                continue;
            }

            _pet_map[id] = msg;
            _dirty_map["pet"] = true;
        } else if (data.key().compare("signinactivity") == 0) {
            auto msg = new demo::pb::SignInActivity;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--signinactivity failed\n", _roleId);
                delete msg;
                continue;
            }

            delete _signinactivity;
            _signinactivity = msg;
            _dirty_map["signinactivity"] = true;
        } else {
            ERROR_LOG("DemoRecordObject::syncIn -- parse role:%d data--unknown data: %s\n", _roleId, data.key().c_str());
        }
    }

    int removeNum = request->removes_size();
    for (int i = 0; i < removeNum; ++i) {
        auto &remove = request->removes(i);
        if (remove.compare(0, 5, "card.") == 0) {
            std::string idStr = remove.substr(5);
            uint32_t id = atoi(idStr.c_str());

            auto it = _card_map.find(id);
            if (it != _card_map.end()) {
                delete it->second;
                _card_map.erase(it);
                _dirty_map["card"] = true;
            } else {
                WARN_LOG("DemoRecordObject::syncIn -- remove role:%d data--card %d not exist\n", _roleId, id);
            }
        } else if (remove.compare(0, 4, "pet.") == 0) {
            std::string idStr = remove.substr(4);
            uint32_t id = atoi(idStr.c_str());

            auto it = _pet_map.find(id);
            if (it != _pet_map.end()) {
                delete it->second;
                _pet_map.erase(it);
                _dirty_map["pet"] = true;
            } else {
                WARN_LOG("DemoRecordObject::syncIn -- remove role:%d data--pet %d not exist\n", _roleId, id);
            }
        } else {
            ERROR_LOG("DemoRecordObject::syncIn -- remove role:%d data--unknown data: %s\n", _roleId, remove.c_str());
        }
    }
}

void DemoRecordObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &profileDatas) {
    // 将脏数据打包
    for (auto &pair : _dirty_map) {
        if (pair.first.compare("name") == 0) {
            auto msg = new wukong::pb::StringValue;
            msg->set_value(_name);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            profileDatas.push_back(std::make_pair("name", msgData));
            datas.push_back(std::make_pair("name", std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(_exp);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair("exp", std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(_lv);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            profileDatas.push_back(std::make_pair("lv", msgData));
            datas.push_back(std::make_pair("lv", std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("currency") == 0) {
            std::string msgData(_currency->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            _currency->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair("currency", std::move(msgData)));
        } else if (pair.first.compare("card") == 0) {
            auto msg = new demo::pb::Cards;
            for (auto &pair1 : _card_map) {
                auto card = msg->add_cards();
                *card = *(pair1.second);
            }

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair("card", std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("pet") == 0) {
            auto msg = new demo::pb::Pets;
            for (auto &pair1 : _pet_map) {
                auto pet = msg->add_pets();
                *pet = *(pair1.second);
            }

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair("pet", std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("signinactivity") == 0) {
            std::string msgData(_signinactivity->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            _signinactivity->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair("signinactivity", std::move(msgData)));
        }
    }
}

void DemoRecordObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
    // 将所有数据打包
    {
        auto msg = new wukong::pb::StringValue;
        msg->set_value(_name);

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("name", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new wukong::pb::Uint32Value;
        msg->set_value(_exp);

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("exp", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new wukong::pb::Uint32Value;
        msg->set_value(_lv);

        std::string msgData(msg->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("lv", std::move(msgData)));
        delete msg;
    }

    {
        std::string msgData(_currency->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        _currency->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("currency", std::move(msgData)));
    }

    {
        auto msg = new demo::pb::Cards;
        for (auto &pair : _card_map) {
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
        for (auto pair : _pet_map) {
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
        std::string msgData(_signinactivity->ByteSizeLong(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        _signinactivity->SerializeWithCachedSizesToArray(buf);

        datas.push_back(std::make_pair("signinactivity", std::move(msgData)));
    }

}