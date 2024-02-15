// TODO：此文件通过工具生成
#include "demo_game_object.h"
#include "proto_utils.h"
#include "share/const.h"

using namespace wukong;
using namespace demo;

DemoGameObject::DemoGameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager): wukong::GameObject(userId, roleId, serverId, lToken, manager) {
    // 设置属性初始值(基础类型可以有默认值)
    name_ = "";
    exp_ = 0;
    lv_ = 1;
    currency_ = new demo::pb::Currency;
    signinactivity_ = new demo::pb::SignInActivity;
}

DemoGameObject::~DemoGameObject() {
    delete currency_;
    delete signinactivity_;

    for (auto &pair : card_map_) {
        delete pair.second;
    }

    for (auto &pair : pet_map_) {
        delete pair.second;
    }
}

bool DemoGameObject::initData(const std::string &data) {
    auto msg = new wukong::pb::DataFragments;
    if (!msg->ParseFromString(data)) {
        ERROR_LOG("DemoGameObject::initData -- parse role:%d data failed\n", roleId_);
        delete msg;
        return false;
    }

    int fragmentNum = msg->fragments_size();
    for (int i = 0; i < fragmentNum; i++) {
        const ::wukong::pb::DataFragment& fragment = msg->fragments(i);
        if (fragment.fragname().compare("name") == 0) {
            auto msg1 = new wukong::pb::StringValue;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--name failed\n", roleId_);
                delete msg1;
                delete msg;
                return false;
            }

            name_ = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("exp") == 0) {
            auto msg1 = new wukong::pb::Uint32Value;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--exp failed\n", roleId_);
                delete msg1;
                delete msg;
                return false;
            }

            exp_ = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("lv") == 0) {
            auto msg1 = new wukong::pb::Uint32Value;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--lv failed\n", roleId_);
                delete msg1;
                delete msg;
                return false;
            }

            lv_ = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("currency") == 0) {
            auto msg1 = new demo::pb::Currency;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--currency failed\n", roleId_);
                delete msg1;
                delete msg;
                return false;
            }

            delete currency_;
            currency_ = msg1;
        } else if (fragment.fragname().compare("card") == 0) {
            auto msg1 = new demo::pb::Cards;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--card failed\n", roleId_);
                delete msg1;
                delete msg;
                return false;
            }

            int cardNum = msg1->cards_size();
            for (int j = 0; j < cardNum; j++) {
                auto card = new demo::pb::Card(msg1->cards(j));
                card_map_.insert(std::make_pair(card->cardid(), card));
            }

            delete msg1;
        } else if (fragment.fragname().compare("pet") == 0) {
            auto msg1 = new demo::pb::Pets;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--pet failed\n", roleId_);
                delete msg1;
                delete msg;
                return false;
            }

            int petNum = msg1->pets_size();
            for (int j = 0; j < petNum; j++) {
                auto pet = new demo::pb::Pet(msg1->pets(j));
                pet_map_.insert(std::make_pair(pet->petid(), pet));
            }

            delete msg1;
        } else if (fragment.fragname().compare("signinactivity") == 0) {
            auto msg1 = new demo::pb::SignInActivity;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoGameObject::initData -- parse role:%d data--signinactivity failed\n", roleId_);
                delete msg1;
                delete msg;
                return false;
            }

            delete signinactivity_;
            signinactivity_ = msg1;
        }
    }

    delete msg;
    return true;
}

void DemoGameObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) {
    // 根据_dirty_map中记录打包数据
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
        } else if (pair.first.compare(0, 5, "card.") == 0) {
            std::string idStr = pair.first.substr(5);
            uint32_t id = atoi(idStr.c_str());
            auto it = card_map_.find(id);
            if (it != card_map_.end()) {
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
            auto it = pet_map_.find(id);
            if (it != pet_map_.end()) {
                std::string msgData(it->second->ByteSizeLong(), 0);
                uint8_t *buf = (uint8_t *)msgData.data();
                it->second->SerializeWithCachedSizesToArray(buf);

                datas.push_back(std::make_pair(pair.first, std::move(msgData)));
            } else {
                removes.push_back(pair.first);
            }
        } else if (pair.first.compare("signinactivity") == 0) {
            std::string msgData(signinactivity_->ByteSizeLong(), 0);
            uint8_t *buf = (uint8_t *)msgData.data();
            signinactivity_->SerializeWithCachedSizesToArray(buf);

            datas.push_back(std::make_pair(pair.first, std::move(msgData)));
        }
    }

    dirty_map_.clear();
}

void DemoGameObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
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

void DemoGameObject::onEnterGame() {
    std::list<std::pair<std::string, std::string>> datas;
    buildAllDatas(datas);
    std::string msgData = ProtoUtils::marshalDataFragments(datas);
    send(wukong::S2C_MESSAGE_ID_ENTERGAME, 0, msgData);
}

const std::string& DemoGameObject::getName() {
    return name_;
}

void DemoGameObject::setName(const std::string& name) {
    name_ = name;
    dirty_map_["name"] = true;
}

uint32_t DemoGameObject::getExp() {
    return exp_;
}

void DemoGameObject::setExp(uint32_t exp) {
    exp_ = exp;
    dirty_map_["exp"] = true;
}

uint32_t DemoGameObject::getLv() {
    return lv_;
}

void DemoGameObject::setLv(uint32_t lv) {
    lv_ = lv;
    dirty_map_["lv"] = true;
}

demo::pb::Currency* DemoGameObject::getCurrency() {
    return currency_;
}

void DemoGameObject::setCurrencyDirty() {
    dirty_map_["currency"] = true;
}

std::vector<uint32_t> DemoGameObject::getAllCardKeys() {
    std::vector<uint32_t> keys;
    keys.reserve(card_map_.size());

    for (auto &pair : card_map_) {
        keys.push_back(pair.first);
    }

    return keys;
}

bool DemoGameObject::hasCard(uint32_t cardid) {
    return card_map_.count(cardid) > 0;
}

uint32_t DemoGameObject::getCardNum() {
    return card_map_.size();
}

demo::pb::Card* DemoGameObject::getCard(uint32_t cardid) {
    auto it = card_map_.find(cardid);
    if (it != card_map_.end()) {
        return it->second;
    }

    return nullptr;
}

void DemoGameObject::setCardDirty(uint32_t cardid) {
    char dirtykey[50];
    sprintf(dirtykey,"card.%d",cardid);
    dirty_map_[dirtykey] = true;
}

void DemoGameObject::addCard(demo::pb::Card* card) {
    card_map_.insert(std::make_pair(card->cardid(), card));
    setCardDirty(card->cardid());
}

void DemoGameObject::removeCard(uint32_t cardid) {
    auto it = card_map_.find(cardid);
    if (it != card_map_.end()) {
        delete it->second;
        card_map_.erase(it);
        setCardDirty(cardid);
    }
}

std::vector<uint32_t> DemoGameObject::getAllPetKeys() {
    std::vector<uint32_t> keys;
    keys.reserve(pet_map_.size());

    for (auto &pair : pet_map_) {
        keys.push_back(pair.first);
    }

    return keys;
}

bool DemoGameObject::hasPet(uint32_t petid) {
    return pet_map_.count(petid) > 0;
}

uint32_t DemoGameObject::getPetNum() {
    return pet_map_.size();
}

demo::pb::Pet* DemoGameObject::getPet(uint32_t petid) {
    auto it = pet_map_.find(petid);
    if (it != pet_map_.end()) {
        return it->second;
    }

    return nullptr;
}

void DemoGameObject::setPetDirty(uint32_t petid) {
    char dirtykey[50];
    sprintf(dirtykey,"pet.%d",petid);
    dirty_map_[dirtykey] = true;
}

void DemoGameObject::addPet(demo::pb::Pet* pet) {
    pet_map_.insert(std::make_pair(pet->petid(), pet));
    setCardDirty(pet->petid());
}

void DemoGameObject::removePet(uint32_t petid) {
    auto it = pet_map_.find(petid);
    if (it != pet_map_.end()) {
        delete it->second;
        pet_map_.erase(it);
        setCardDirty(petid);
    }
}

demo::pb::SignInActivity* DemoGameObject::getSignInActivity() {
    return signinactivity_;
}

void DemoGameObject::setSignInActivityDirty() {
    dirty_map_["signinactivity"] = true;
}
