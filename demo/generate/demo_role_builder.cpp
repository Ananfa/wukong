// TODO：此文件通过工具生成

#include "demo_role_builder.h"

DemoRoleBuilder::DemoRoleBuilder() {
	_name = "";
    _exp = 0;
    _lv = 1;
    _currency = new demo::pb::Currency;
    _signinactivity = new demo::pb::SignInActivity;
}

DemoRoleBuilder::~DemoRoleBuilder() {
    delete _currency;
    delete _signinactivity;

    for (auto &pair : _card_map) {
        delete pair.second;
    }

    for (auto &pair : _pet_map) {
        delete pair.second;
    }
}

void DemoRoleBuilder::buildDatas(std::list<std::pair<std::string, std::string>> &datas) {
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

void DemoRoleBuilder::setName(const std::string& name) {
	_name = name;
}

void DemoRoleBuilder::setExp(uint32_t exp) {
	_exp = exp;
}

void DemoRoleBuilder::setLv(uint32_t lv) {
	_lv = lv;
}

demo::pb::Currency* DemoRoleBuilder::getCurrency() {
	return _currency;
}

void DemoRoleBuilder::addCard(demo::pb::Card* card) {
	_card_map.insert(std::make_pair(card->cardid(), card));
}

void DemoRoleBuilder::addPet(demo::pb::Pet* pet) {
	_pet_map.insert(std::make_pair(pet->petid(), pet));
}

demo::pb::SignInActivity* DemoRoleBuilder::getSignInActivity() {
	return _signinactivity;
}
