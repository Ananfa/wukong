// This file is generated. Don't edit it

#include "demo_lobby_object_data.h"
#include "share/const.h"

using namespace wukong;
using namespace demo;

DemoLobbyObjectData::DemoLobbyObjectData() {
    name_ = "";
    exp_ = 0;
    lv_ = 0;
    currency_ = new demo::pb::Currency;
    signinactivity_ = new demo::pb::SignInActivity;
}

DemoLobbyObjectData::~DemoLobbyObjectData() {
    delete currency_;
    for (auto &pair : card_map_) {
        delete pair.second;
    }
    for (auto &pair : pet_map_) {
        delete pair.second;
    }
    delete signinactivity_;
}

bool DemoLobbyObjectData::initData(const std::string &data) {
    auto msg = new wukong::pb::DataFragments;
    if (!msg->ParseFromString(data)) {
        ERROR_LOG("DemoLobbyObjectData::initData -- parse role data failed\n");
        delete msg;
        return false;
    }

    int fragmentNum = msg->fragments_size();
    for (int i = 0; i < fragmentNum; i++) {
        const ::wukong::pb::DataFragment& fragment = msg->fragments(i);
        if (fragment.fragname().compare("name") == 0) {
            auto msg1 = new wukong::pb::StringValue;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoLobbyObjectData::initData -- parse role data--name failed\n");
                delete msg1;
                delete msg;
                return false;
            }

            name_ = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("exp") == 0) {
            auto msg1 = new wukong::pb::Uint32Value;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoLobbyObjectData::initData -- parse role data--exp failed\n");
                delete msg1;
                delete msg;
                return false;
            }

            exp_ = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("lv") == 0) {
            auto msg1 = new wukong::pb::Uint32Value;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoLobbyObjectData::initData -- parse role data--lv failed\n");
                delete msg1;
                delete msg;
                return false;
            }

            lv_ = msg1->value();
            delete msg1;
        } else if (fragment.fragname().compare("currency") == 0) {
            auto msg1 = new demo::pb::Currency;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoLobbyObjectData::initData -- parse role data--currency failed\n");
                delete msg1;
                delete msg;
                return false;
            }

            delete currency_;
            currency_ = msg1;
        } else if (fragment.fragname().compare("card") == 0) {
            auto msg1 = new demo::pb::Cards;
            if (!msg1->ParseFromString(fragment.fragdata())) {
                ERROR_LOG("DemoLobbyObjectData::initData -- parse role data--card failed\n");
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
                ERROR_LOG("DemoLobbyObjectData::initData -- parse role data--pet failed\n");
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
                ERROR_LOG("DemoLobbyObjectData::initData -- parse role data--signinactivity failed\n");
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

void DemoLobbyObjectData::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) {
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

void DemoLobbyObjectData::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
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
        for (auto &pair : pet_map_) {
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

const std::string& DemoLobbyObjectData::getName() {
    return name_;
}

void DemoLobbyObjectData::setName(const std::string& name) {
    name_ = name;
    dirty_map_["name"] = true;
}

uint32_t DemoLobbyObjectData::getExp() {
    return exp_;
}

void DemoLobbyObjectData::setExp(uint32_t exp) {
    exp_ = exp;
    dirty_map_["exp"] = true;
}

uint32_t DemoLobbyObjectData::getLv() {
    return lv_;
}

void DemoLobbyObjectData::setLv(uint32_t lv) {
    lv_ = lv;
    dirty_map_["lv"] = true;
}

demo::pb::Currency* DemoLobbyObjectData::getCurrency() {
    return currency_;
}

void DemoLobbyObjectData::setCurrencyDirty() {
    dirty_map_["currency"] = true;
}

std::vector<uint32_t> DemoLobbyObjectData::getAllCardKeys() {
    std::vector<uint32_t> keys;
    keys.reserve(card_map_.size());

    for (auto &pair : card_map_) {
        keys.push_back(pair.first);
    }

    return keys;
}

bool DemoLobbyObjectData::hasCard(uint32_t cardid) {
    return card_map_.count(cardid) > 0;
}

uint32_t DemoLobbyObjectData::getCardNum() {
    return card_map_.size();
}

demo::pb::Card* DemoLobbyObjectData::getCard(uint32_t cardid) {
    auto it = card_map_.find(cardid);
    if (it != card_map_.end()) {
        return it->second;
    }

    return nullptr;
}

void DemoLobbyObjectData::setCardDirty(uint32_t cardid) {
    char dirtykey[50];
    sprintf(dirtykey,"card.%d",cardid);
    dirty_map_[dirtykey] = true;
}

void DemoLobbyObjectData::addCard(demo::pb::Card* card) {
    card_map_.insert(std::make_pair(card->cardid(), card));
    setCardDirty(card->cardid());
}

void DemoLobbyObjectData::removeCard(uint32_t cardid) {
    auto it = card_map_.find(cardid);
    if (it != card_map_.end()) {
        delete it->second;
        card_map_.erase(it);
        setCardDirty(cardid);
    }
}

std::vector<uint32_t> DemoLobbyObjectData::getAllPetKeys() {
    std::vector<uint32_t> keys;
    keys.reserve(pet_map_.size());

    for (auto &pair : pet_map_) {
        keys.push_back(pair.first);
    }

    return keys;
}

bool DemoLobbyObjectData::hasPet(uint32_t petid) {
    return pet_map_.count(petid) > 0;
}

uint32_t DemoLobbyObjectData::getPetNum() {
    return pet_map_.size();
}

demo::pb::Pet* DemoLobbyObjectData::getPet(uint32_t petid) {
    auto it = pet_map_.find(petid);
    if (it != pet_map_.end()) {
        return it->second;
    }

    return nullptr;
}

void DemoLobbyObjectData::setPetDirty(uint32_t petid) {
    char dirtykey[50];
    sprintf(dirtykey,"pet.%d",petid);
    dirty_map_[dirtykey] = true;
}

void DemoLobbyObjectData::addPet(demo::pb::Pet* pet) {
    pet_map_.insert(std::make_pair(pet->petid(), pet));
    setPetDirty(pet->petid());
}

void DemoLobbyObjectData::removePet(uint32_t petid) {
    auto it = pet_map_.find(petid);
    if (it != pet_map_.end()) {
        delete it->second;
        pet_map_.erase(it);
        setPetDirty(petid);
    }
}

demo::pb::SignInActivity* DemoLobbyObjectData::getSigninactivity() {
    return signinactivity_;
}

void DemoLobbyObjectData::setSigninactivityDirty() {
    dirty_map_["signinactivity"] = true;
}

