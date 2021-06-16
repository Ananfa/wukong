// TODO：此文件通过工具生成
#include "demo_game_object.h"
#include "proto_utils.h"
#include "share/const.h"

using namespace wukong;
using namespace demo;

DemoGameObject::DemoGameObject(UserId userId, RoleId roleId, ServerId serverId, uint32_t lToken, GameObjectManager *manager): wukong::GameObject(userId, roleId, serverId, lToken, manager) {
    // 设置属性初始值(基础类型可以有默认值)
    _name = "";
    _exp = 0;
    _lv = 1;
    _currency = new demo::pb::Currency;
    _signinactivity = new demo::pb::SignInActivity;
}

DemoGameObject::~DemoGameObject() {
    delete _currency;
    delete _signinactivity;

    for (auto &pair : _card_map) {
        delete pair.second;
    }

    for (auto &pair : _pet_map) {
        delete pair.second;
    }
}

bool DemoGameObject::initData(const std::string &data) {
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

            int cardNum = msg1->cards_size();
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

            int petNum = msg1->pets_size();
            for (int j = 0; j < petNum; j++) {
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
    return true;
}

void DemoGameObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) {
    // 根据_dirty_map中记录打包数据
    for (auto &pair : _dirty_map) {
        if (pair.first.compare("name") == 0) {
            auto msg = new wukong::pb::StringValue;
            msg->set_value(_name);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("exp") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(_exp);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("lv") == 0) {
            auto msg = new wukong::pb::Uint32Value;
            msg->set_value(_lv);

            std::string msgData(msg->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            msg->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            delete msg;
        } else if (pair.first.compare("currency") == 0) {
            std::string msgData(_currency->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            _currency->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
        } else if (pair.first.compare(0, 5, "card.") == 0) {
            std::string idStr = pair.first.substr(5);
            uint32_t id = atoi(idStr.c_str());
            auto it = _card_map.find(id);
            if (it != _card_map.end()) {
                std::string msgData(it->second->ByteSizeLong(), 0);
                uint8_t *buf = (uint8_t *)msgData.data();
                it->second->SerializeWithCachedSizesToArray(buf);

                datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            } else {
                removes.push_back(pair.first);
            }
        } else if (pair.first.compare(0, 4, "pet.") == 0) {
            std::string idStr = pair.first.substr(4);
            uint32_t id = atoi(idStr.c_str());
            auto it = _pet_map.find(id);
            if (it != _pet_map.end()) {
                std::string msgData(it->second->ByteSizeLong(), 0);
                uint8_t *buf = (uint8_t *)msgData.data();
                it->second->SerializeWithCachedSizesToArray(buf);

                datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            } else {
                removes.push_back(pair.first);
            }
        } else if (pair.first.compare("signinactivity") == 0) {
            std::string msgData(_signinactivity->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            _signinactivity->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
        }
    }

    _dirty_map.clear();
}

void DemoGameObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
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

void DemoGameObject::onEnterGame() {
    std::list<std::pair<std::string, std::string>> datas;
    buildAllDatas(datas);
    std::string msgData = ProtoUtils::marshalDataFragments(datas);
    send(wukong::S2C_MESSAGE_ID_ENTERGAME, 0, msgData);
}

const std::string& DemoGameObject::getName() {
    return _name;
}

void DemoGameObject::setName(const std::string& name) {
    _name = name;
    _dirty_map["name"] = true;
}

uint32_t DemoGameObject::getExp() {
    return _exp;
}

void DemoGameObject::setExp(uint32_t exp) {
    _exp = exp;
    _dirty_map["exp"] = true;
}

uint32_t DemoGameObject::getLv() {
    return _lv;
}

void DemoGameObject::setLv(uint32_t lv) {
    _lv = lv;
    _dirty_map["lv"] = true;
}

demo::pb::Currency* DemoGameObject::getCurrency() {
    return _currency;
}

void DemoGameObject::setCurrencyDirty() {
    _dirty_map["currency"] = true;
}

std::vector<uint32_t> DemoGameObject::getAllCardKeys() {
    std::vector<uint32_t> keys;
    keys.reserve(_card_map.size());

    for (auto &pair : _card_map) {
        keys.push_back(pair.first);
    }

    return keys;
}

bool DemoGameObject::hasCard(uint32_t cardid) {
    return _card_map.count(cardid) > 0;
}

uint32_t DemoGameObject::getCardNum() {
    return _card_map.size();
}

demo::pb::Card* DemoGameObject::getCard(uint32_t cardid) {
    auto it = _card_map.find(cardid);
    if (it != _card_map.end()) {
        return it->second;
    }

    return nullptr;
}

void DemoGameObject::setCardDirty(uint32_t cardid) {
    char dirtykey[50];
    sprintf(dirtykey,"card.%d",cardid);
    _dirty_map[dirtykey] = true;
}

void DemoGameObject::addCard(demo::pb::Card* card) {
    _card_map.insert(std::make_pair(card->cardid(), card));
    setCardDirty(card->cardid());
}

void DemoGameObject::removeCard(uint32_t cardid) {
    auto it = _card_map.find(cardid);
    if (it != _card_map.end()) {
        delete it->second;
        _card_map.erase(it);
        setCardDirty(cardid);
    }
}

std::vector<uint32_t> DemoGameObject::getAllPetKeys() {
    std::vector<uint32_t> keys;
    keys.reserve(_pet_map.size());

    for (auto &pair : _pet_map) {
        keys.push_back(pair.first);
    }

    return keys;
}

bool DemoGameObject::hasPet(uint32_t petid) {
    return _pet_map.count(petid) > 0;
}

uint32_t DemoGameObject::getPetNum() {
    return _pet_map.size();
}

demo::pb::Pet* DemoGameObject::getPet(uint32_t petid) {
    auto it = _pet_map.find(petid);
    if (it != _pet_map.end()) {
        return it->second;
    }

    return nullptr;
}

void DemoGameObject::setPetDirty(uint32_t petid) {
    char dirtykey[50];
    sprintf(dirtykey,"pet.%d",petid);
    _dirty_map[dirtykey] = true;
}

void DemoGameObject::addPet(demo::pb::Pet* pet) {
    _pet_map.insert(std::make_pair(pet->petid(), pet));
    setCardDirty(pet->petid());
}

void DemoGameObject::removePet(uint32_t petid) {
    auto it = _pet_map.find(petid);
    if (it != _pet_map.end()) {
        delete it->second;
        _pet_map.erase(it);
        setCardDirty(petid);
    }
}

demo::pb::SignInActivity* DemoGameObject::getSignInActivity() {
    return _signinactivity;
}

void DemoGameObject::setSignInActivityDirty() {
    _dirty_map["signinactivity"] = true;
}
