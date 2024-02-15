// TODO：此文件通过工具生成

#include "demo_role_builder.h"

using namespace demo;

DemoRoleBuilder::DemoRoleBuilder() {
	name_ = "";
    exp_ = 0;
    lv_ = 1;
    currency_ = new demo::pb::Currency;
    signinactivity_ = new demo::pb::SignInActivity;
}

DemoRoleBuilder::~DemoRoleBuilder() {
    delete currency_;
    delete signinactivity_;

    for (auto &pair : card_map_) {
        delete pair.second;
    }

    for (auto &pair : pet_map_) {
        delete pair.second;
    }
}

void DemoRoleBuilder::buildDatas(std::list<std::pair<std::string, std::string>> &datas) {
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

void DemoRoleBuilder::setName(const std::string& name) {
	name_ = name;
}

void DemoRoleBuilder::setExp(uint32_t exp) {
	exp_ = exp;
}

void DemoRoleBuilder::setLv(uint32_t lv) {
	lv_ = lv;
}

demo::pb::Currency* DemoRoleBuilder::getCurrency() {
	return currency_;
}

void DemoRoleBuilder::addCard(demo::pb::Card* card) {
	card_map_.insert(std::make_pair(card->cardid(), card));
}

void DemoRoleBuilder::addPet(demo::pb::Pet* pet) {
	pet_map_.insert(std::make_pair(pet->petid(), pet));
}

demo::pb::SignInActivity* DemoRoleBuilder::getSignInActivity() {
	return signinactivity_;
}
