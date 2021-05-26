// TODO：此文件通过工具生成

DemoRecordObject::DemoRecordObject(RoleId roleId, RecordObjectManager *manager): wukong::RecordObject(roleId, manager) {
    // 设置属性初始值(基础类型可以有默认值)
    _name = "";
    _exp = 0;
    _lv = 1;
    _currency = new demo::pb::Currency;
    _signinactivity = new demo::pb::SignInActivity;
}

bool DemoRecordObject::initData(const std::string &data) {
    auto msg = new wukong::pb::DataFragments;
    if (!msg->ParseFromString(data)) {
        ERROR_LOG("DemoGameObject::initData -- parse role:%d data failed\n", _roleId);
        delete msg;
        return false;
    }

    int fragmentNum = msg->fragments_size();
    for (int i = 0; i < fragmentNum; i++) {
        const ::wukong::pb::DataFragment& fragment = msg->fragments(i);
        if (fragment.fragname().compare("name") == 0) {
            auto msg1 = new wukong::pb::StringValue;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--name failed\n", _roleId);
                delete msg1;
                delete msg;
                return false;
            }

            _name = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("exp") == 0) {
            auto msg1 = new wukong::pb::Uint32Value;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--exp failed\n", _roleId);
                delete msg1;
                delete msg;
                return false;
            }

            _exp = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("lv") == 0) {
            auto msg1 = new wukong::pb::Uint32Value;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--lv failed\n", _roleId);
                delete msg1;
                delete msg;
                return false;
            }

            _lv = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("currency") == 0) {
            auto msg1 = new demo::pb::Currency;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--currency failed\n", _roleId);
                delete msg1;
                delete msg;
                return false;
            }

            delete _currency;
            _currency = msg1;
        } else if (fragment.fragname().compare("card") == 0) {
            auto msg1 = new demo::pb::Cards;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--card failed\n", _roleId);
                delete msg1;
                delete msg;
                return false;
            }

            cardNum = msg1->cards_size();
            for (int j = 0; j < cardNum; j++) {
                auto card = new demo::pb::Card(msg1->cards(j));
                _card_map.insert(std::make_pair(card->cardid(), card));
            }

            delete msg1;
        } else if (fragment.fragname().compare("pet") == 0) {
            auto msg1 = new demo::pb::Pets;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--pet failed\n", _roleId);
                delete msg1;
                delete msg;
                return false;
            }

            petNum = msg1->pets_size();
            for (int j = 0; j < cardNum; j++) {
                auto pet = new demo::pb::Pet(msg1->pets(j));
                _pet_map.insert(std::make_pair(pet->petid(), pet));
            }

            delete msg1;
        } else if (fragment.fragname().compare("signinactivity") == 0) {
            auto msg1 = new demo::pb::SignInActivity;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--signinactivity failed\n", _roleId);
                delete msg1;
                delete msg;
                return false;
            }

            delete _signinactivity;
            _signinactivity = msg1;
        }
    }

    delete msg;
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
        if (remove.key().compare(0, 5, "card.") == 0) {
            std::string idStr = remove.key().substr(5);
            uint32_t id = atoi(idStr.c_str());

            auto it = _card_map.find(id);
            if (it != _card_map.end()) {
                delete it->second;
                _card_map.erase(it);
                _dirty_map["card"] = true;
            } else {
                WARN_LOG("DemoRecordObject::syncIn -- remove role:%d data--card %d not exist\n", _roleId, id);
            }
        } else if (remove.key().compare(0, 4, "pet.") == 0) {
            std::string idStr = remove.key().substr(4);
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
            ERROR_LOG("DemoRecordObject::syncIn -- remove role:%d data--unknown data: %s\n", _roleId, remove.key().c_str());
        }
    }
}

void DemoRecordObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &profileDatas) {
    // 将脏数据打包
    for (auto it = _dirty_map.begin(); it != _dirty_map.end(); ++it) {
        if (it->first.compare("name") == 0) {
            auto msg = new wukong::pb::StringValue;
            msg->set_value(_name);

            std::string msgData(msg->ByteSize(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            profileDatas.insert(std::make_pair("name", msgData));
            datas.insert(std::make_pair("name", std::move(msgData)));
            delete msg;
        } else if (it->first.compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(_exp);

            std::string msgData(msg->ByteSize(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.insert(std::make_pair("exp", std::move(msgData)));
            delete msg;
        } else if (it->first.compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(_lv);

            std::string msgData(msg->ByteSize(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            profileDatas.insert(std::make_pair("lv", msgData));
            datas.insert(std::make_pair("lv", std::move(msgData)));
            delete msg;
        } else if (it->first.compare("currency") == 0) {
            std::string msgData(_currency->ByteSize(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            _currency->SerializeWithCachedSizesToArray(buf);

            datas.insert(std::make_pair("currency", std::move(msgData)));
        } else if (it->first.compare("card") == 0) {
            auto msg = new demoGame::pb::Cards;
            for (auto it1 = _card_map.begin(); it1 != _card_map.end(); ++it1) {
                auto card = msg->add_cards();
                *card = *(it1->second);
            }

            std::string msgData(msg->ByteSize(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.insert(std::make_pair("card", std::move(msgData)));
            delete msg;
        } else if (it->first.compare("pet") == 0) {
            auto msg = new demoGame::pb::Pets;
            for (auto it1 = _pet_map.begin(); it1 != _pet_map.end(); ++it1) {
                auto pet = msg->add_pets();
                *pet = *(it1->second);
            }

            std::string msgData(msg->ByteSize(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.insert(std::make_pair("pet", std::move(msgData)));
            delete msg;
        } else if (it->first.compare("signinactivity") == 0) {
            std::string msgData(_signinactivity->ByteSize(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            _signinactivity->SerializeWithCachedSizesToArray(buf);

            datas.insert(std::make_pair("signinactivity", std::move(msgData)));
        }
    }
}

void DemoRecordObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
    // 将所有数据打包
    {
        auto msg = new wukong::pb::StringValue;
        msg->set_value(_name);

        std::string msgData(msg->ByteSize(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.insert(std::make_pair("name", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new wukong::pb::Uint32Value;
        msg->set_value(_exp);

        std::string msgData(msg->ByteSize(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.insert(std::make_pair("exp", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new wukong::pb::Uint32Value;
        msg->set_value(_lv);

        std::string msgData(msg->ByteSize(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.insert(std::make_pair("lv", std::move(msgData)));
        delete msg;
    }

    {
        std::string msgData(_currency->ByteSize(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        _currency->SerializeWithCachedSizesToArray(buf);

        datas.insert(std::make_pair("currency", std::move(msgData)));
    }

    {
        auto msg = new demoGame::pb::Cards;
        for (auto it = _card_map.begin(); it != _card_map.end(); ++it) {
            auto card = msg->add_cards();
            *card = *(it->second);
        }

        std::string msgData(msg->ByteSize(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.insert(std::make_pair("card", std::move(msgData)));
        delete msg;
    }

    {
        auto msg = new demoGame::pb::Pets;
        for (auto it = _pet_map.begin(); it != _pet_map.end(); ++it) {
            auto pet = msg->add_pets();
            *pet = *(it->second);
        }

        std::string msgData(msg->ByteSize(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        msg->SerializeWithCachedSizesToArray(buf);

        datas.insert(std::make_pair("pet", std::move(msgData)));
        delete msg;
    }

    {
        std::string msgData(_signinactivity->ByteSize(), 0);
        uint8_t *buf = (uint8_t *)msgData.data();
        _signinactivity->SerializeWithCachedSizesToArray(buf);

        datas.insert(std::make_pair("signinactivity", std::move(msgData)));
    }

}