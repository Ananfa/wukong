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
            // TODO: 设置dirty

            auto msg = new wukong::pb::StringValue;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::sync -- parse role:%d data--name failed\n", _roleId);
                delete msg;
                continue;
            }

            _name = msg->value();
            delete msg;
        } else if (data.key().compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::sync -- parse role:%d data--exp failed\n", _roleId);
                delete msg;
                continue;
            }

            _exp = msg->value();
            delete msg;
        } else if (data.key().compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::sync -- parse role:%d data--lv failed\n", _roleId);
                delete msg;
                continue;
            }

            _lv = msg->value();
            delete msg;
        } else if (data.key().compare("currency") == 0) {
            auto msg = new demo::pb::Currency;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::sync -- parse role:%d data--currency failed\n", _roleId);
                delete msg;
                continue;
            }

            delete _currency;
            _currency = msg;
        } else if (data.key().compare(0, 5, "card.") == 0) {
            std::string idStr = it->first.substr(5);
            uint32_t id = atoi(idStr.c_str());

            auto msg = new demo::pb::Card;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::sync -- parse role:%d data--card:%d failed\n", _roleId, id);
                delete msg;
                continue;
            }

            _card_map[id] = msg;
        } else if (data.key().compare(0, 4, "pet.") == 0) {
            std::string idStr = it->first.substr(4);
            uint32_t id = atoi(idStr.c_str());

            auto msg = new demo::pb::Pet;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::sync -- parse role:%d data--pet:%d failed\n", _roleId, id);
                delete msg;
                continue;
            }

            _pet_map[id] = msg;
        } else if (data.key().compare("signinactivity") == 0) {
            auto msg = new demo::pb::SignInActivity;
            if (!msg->ParseFromString(data.value())) {
                ERROR_LOG("DemoRecordObject::sync -- parse role:%d data--signinactivity failed\n", _roleId);
                delete msg;
                continue;
            }

            delete _signinactivity;
            _signinactivity = msg;
        }
    }

    int removeNum = request->removes_size();
    for (int i = 0; i < removeNum; ++i) {

    }
}

void DemoRecordObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas) {
    // TODO: 
}