package main

import (
	"os"
	"strconv"
	"strings"
)

func genGameObject() {
	hFile, _ := os.Create(targetPath + pkgName + "_game_object.h")
	defer hFile.Close()

	genGameObjectHeadFile(hFile)

	cppFile, _ := os.Create(targetPath + pkgName + "_game_object.cpp")
	defer cppFile.Close()

	genGameObjectCppFile(cppFile)
}

func genGameObjectHeadFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#ifndef " + pkgName + "_game_object_h\n" +
		"#define " + pkgName + "_game_object_h\n\n" +
		"#include \"" + pkgName +  ".pb.h\"\n" +
		"#include \"game_object.h\"\n" +
		"#include \"common.pb.h\"\n" +
		"#include \"game_object_manager.h\"\n" +
		"#include <map>\n" +
		"#include <string>\n" +
		"#include <vector>\n" +
		"\n" +
		"using namespace wukong;\n" +
		"\n" +
		"namespace " + pkgName +  " {\n" +
		"    class " + pkgNameCapFirst + "GameObject: public wukong::GameObject {\n" +
		"    public:\n" +
		"        " + pkgNameCapFirst + "GameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager);\n" +
		"        virtual ~" + pkgNameCapFirst + "GameObject();\n" +
		"\n" +
		"        virtual bool initData(const std::string &data);\n" +
		"\n" +
		"        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes);\n" +
		"        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas);\n" +
		"        virtual void onEnterGame();\n")

	for _, member := range gameDataConfig.Member {
		mName := member.Attr
		mNameCapFirst := strings.Title(member.Attr)

		if member.Type == "string" {
			dstFile.WriteString("        const std::string& get" + mNameCapFirst + "();\n")
			dstFile.WriteString("        void set" + mNameCapFirst + "(const std::string& " + mName + ");\n")
		} else if member.Type == "uint32" {
			dstFile.WriteString("        uint32_t get" + mNameCapFirst + "();\n")
			dstFile.WriteString("        void set" + mNameCapFirst + "(uint32_t " + mName + ");\n")
		} else if member.Type == "int32" {
			dstFile.WriteString("        int32_t get" + mNameCapFirst + "();\n")
			dstFile.WriteString("        void set" + mNameCapFirst + "(int32_t " + mName + ");\n")
		} else if member.Type == "uint64" {
			dstFile.WriteString("        uint64_t get" + mNameCapFirst + "();\n")
			dstFile.WriteString("        void set" + mNameCapFirst + "(uint64_t " + mName + ");\n")
		} else if member.Type == "int64" {
			dstFile.WriteString("        int64_t get" + mNameCapFirst + "();\n")
			dstFile.WriteString("        void set" + mNameCapFirst + "(int64_t " + mName + ");\n")
		} else if member.Type == "bool" {
			dstFile.WriteString("        bool get" + mNameCapFirst + "();\n")
			dstFile.WriteString("        void set" + mNameCapFirst + "(bool " + mName + ");\n")
		} else {
			if member.InnerList == "" {
				dstFile.WriteString("        " + member.Type + "* get" + mNameCapFirst + "();\n")
				dstFile.WriteString("        void set" + mNameCapFirst + "Dirty();\n")
			} else {
				mInnerId := member.InnerListItemIdKey

				dstFile.WriteString("        std::vector<uint32_t> getAll" + mNameCapFirst + "Keys();\n" +
					"        bool has" + mNameCapFirst + "(uint32_t " + mInnerId + ");\n" +
					"        uint32_t get" + mNameCapFirst + "Num();\n" +
					"        " + member.InnerListItemType + "* get" + mNameCapFirst + "(uint32_t " + mInnerId + ");\n" +
					"        void set" + mNameCapFirst + "Dirty(uint32_t " + mInnerId + ");\n" +
					"        void add" + mNameCapFirst + "(" + member.InnerListItemType + "* " + mName + ");\n" +
					"        void remove" + mNameCapFirst + "(uint32_t " + mInnerId + ");\n")
			}
		}
	}

	dstFile.WriteString("\n")
	dstFile.WriteString("    private:\n")
	for _, member := range gameDataConfig.Member {
		if member.Type == "string" {
			dstFile.WriteString("        std::string " + member.Attr + "_;\n")
		} else if member.Type == "uint32" {
			dstFile.WriteString("        uint32_t " + member.Attr + "_;\n")
		} else if member.Type == "int32" {
			dstFile.WriteString("        int32_t " + member.Attr + "_;\n")
		} else if member.Type == "uint64" {
			dstFile.WriteString("        uint64_t " + member.Attr + "_;\n")
		} else if member.Type == "int64" {
			dstFile.WriteString("        int64_t " + member.Attr + "_;\n")
		} else if member.Type == "bool" {
			dstFile.WriteString("        bool " + member.Attr + "_;\n")
		} else {
			if member.InnerList == "" {
				dstFile.WriteString("        " + member.Type + "* " + member.Attr + "_;\n")
			} else {
				dstFile.WriteString("        std::map<uint32_t, " + member.InnerListItemType + "*> " + member.Attr + "_map_;\n")
			}
		}
	}

	dstFile.WriteString("    };\n" +
		"}\n" +
		"\n" +
		"#endif /* " + pkgName + "_game_object_h */\n")
}

func genGameObjectCppFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#include \"" + pkgName + "_game_object.h\"\n" +
		"#include \"proto_utils.h\"\n" +
		"#include \"share/const.h\"\n" +
		"\n" +
		"using namespace wukong;\n" +
		"using namespace " + pkgName + ";\n" +
		"\n" +
		pkgNameCapFirst + "GameObject::" + pkgNameCapFirst + "GameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager): wukong::GameObject(userId, roleId, serverId, lToken, manager) {\n")

	for _, member := range gameDataConfig.Member {
		if member.Type == "string" {
			dstFile.WriteString("    " + member.Attr + "_ = \"\";\n")
		} else if member.Type == "uint32" ||
			member.Type == "int32" ||
			member.Type == "uint64" ||
			member.Type == "int64" {
			dstFile.WriteString("    " + member.Attr + "_ = 0;\n")
		} else if member.Type == "bool" {
			dstFile.WriteString("    " + member.Attr + "_ = false;\n")
		} else if member.InnerList == "" {
			dstFile.WriteString("    " + member.Attr + "_ = new " + member.Type + ";\n")
		}
	}

	dstFile.WriteString("}\n" +
		"\n" +
		pkgNameCapFirst + "GameObject::~" + pkgNameCapFirst + "GameObject() {\n")

	for _, member := range gameDataConfig.Member {
		if member.Type == "string" ||
			member.Type == "uint32" ||
			member.Type == "int32" ||
			member.Type == "uint64" ||
			member.Type == "int64" ||
			member.Type == "bool" {

		} else {
			if member.InnerList == "" {
				dstFile.WriteString("    delete " + member.Attr + "_;\n")
			} else {
				dstFile.WriteString("    for (auto &pair : " + member.Attr + "_map_) {\n" +
					"        delete pair.second;\n" +
					"    }\n")
			}
		}
	}

	dstFile.WriteString("}\n")

	dstFile.WriteString("\n" +
		"bool " + pkgNameCapFirst + "GameObject::initData(const std::string &data) {\n" +
		"    auto msg = new wukong::pb::DataFragments;\n" +
		"    if (!msg->ParseFromString(data)) {\n" +
		"        ERROR_LOG(\"" + pkgNameCapFirst + "GameObject::initData -- parse role:%d data failed\\n\", roleId_);\n" +
		"        delete msg;\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    int fragmentNum = msg->fragments_size();\n")

	dstFile.WriteString("    for (int i = 0; i < fragmentNum; i++) {\n" +
		"        const ::wukong::pb::DataFragment& fragment = msg->fragments(i);\n")

	fun1 := func(memberName, memberType string) {
		dstFile.WriteString("            auto msg1 = new " + memberType + ";\n" +
			"            if (!msg1->ParseFromString(fragment.fragdata())) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "GameObject::initData -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg1;\n" +
			"                delete msg;\n" +
			"                return false;\n" +
			"            }\n" +
			"\n" +
			"            " + memberName + "_ = msg1->value();\n" +
			"            delete msg1;\n")
	}

	fun2 := func(memberName, memberType string) {
		dstFile.WriteString("            auto msg1 = new " + memberType + ";\n" +
			"            if (!msg1->ParseFromString(fragment.fragdata())) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "GameObject::initData -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg1;\n" +
			"                delete msg;\n" +
			"                return false;\n" +
			"            }\n" +
			"\n" +
			"            delete " + memberName + "_;\n" +
			"            " + memberName + "_ = msg1;\n")
	}

	fun3 := func(memberName, memberType, listName, itemType, itemId string) {
		dstFile.WriteString("            auto msg1 = new " + memberType + ";\n" +
			"            if (!msg1->ParseFromString(fragment.fragdata())) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "GameObject::initData -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg1;\n" +
			"                delete msg;\n" +
			"                return false;\n" +
			"            }\n" +
			"\n" +
			"            int " + memberName + "Num = msg1->" + listName + "_size();\n" +
			"            for (int j = 0; j < " + memberName + "Num; j++) {\n" +
			"                auto " + memberName + " = new " + itemType + "(msg1->" + listName + "(j));\n" +
			"                " + memberName + "_map_.insert(std::make_pair(" + memberName + "->" + itemId + "(), " + memberName + "));\n" +
			"            }\n" +
			"\n" +
			"            delete msg1;\n")
	}

	for i, member := range gameDataConfig.Member {
		if i == 0 {
			dstFile.WriteString("        if ")
		} else {
			dstFile.WriteString("        } else if ")
		}
		dstFile.WriteString("(fragment.fragname().compare(\"" + member.Attr + "\") == 0) {\n")

		if member.Type == "string" {
			fun1(member.Attr, "wukong::pb::StringValue")
		} else if member.Type == "uint32" {
			fun1(member.Attr, "wukong::pb::Uint32Value")
		} else if member.Type == "int32" {
			fun1(member.Attr, "wukong::pb::Int32Value")
		} else if member.Type == "uint64" {
			fun1(member.Attr, "wukong::pb::Uint64Value")
		} else if member.Type == "int64" {
			fun1(member.Attr, "wukong::pb::Int64Value")
		} else if member.Type == "bool" {
			fun1(member.Attr, "wukong::pb::BoolValue")
		} else {
			if member.InnerList == "" {
				fun2(member.Attr, member.Type)
			} else {
				fun3(member.Attr, member.Type, member.InnerList, member.InnerListItemType, member.InnerListItemIdKey)
			}
		}
	}

	dstFile.WriteString("        }\n" +
		"    }\n" +
		"\n" +
		"    delete msg;\n" +
		"    return true;\n" +
		"}\n" +
		"\n")

	dstFile.WriteString("void " + pkgNameCapFirst + "GameObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) {\n" +
		"    for (auto &pair : dirty_map_) {\n")

	fun4 := func(memberName, memberType string) {
		dstFile.WriteString("(pair.first.compare(\"" + memberName + "\") == 0) {\n" +
			"            auto msg = new " + memberType + ";\n" +
			"            msg->set_value(" + memberName + "_);\n" +
			"\n" +
			"            std::string msgData(msg->ByteSizeLong(), 0);\n" +
			"            uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"            msg->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"            datas.push_back(std::make_pair(pair.first, std::move(msgData)));\n" +
			"            delete msg;\n")
	}

	fun5 := func(memberName string) {
		dstFile.WriteString("(pair.first.compare(\"" + memberName + "\") == 0) {\n" +
			"            std::string msgData(" + memberName + "_->ByteSizeLong(), 0);\n" +
			"            uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"            " + memberName + "_->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"            datas.push_back(std::make_pair(pair.first, std::move(msgData)));\n")
	}

	fun6 := func(memberName string) {
		dstFile.WriteString("(pair.first.compare(0, " + strconv.Itoa(len(memberName)+1) + ", \"" + memberName + ".\") == 0) {\n" +
			"            std::string idStr = pair.first.substr(" + strconv.Itoa(len(memberName)+1) + ");\n" +
			"            uint32_t id = atoi(idStr.c_str());\n" +
			"            auto it = " + memberName + "_map_.find(id);\n" +
			"            if (it != " + memberName + "_map_.end()) {\n" +
			"                std::string msgData(it->second->ByteSizeLong(), 0);\n" +
			"                uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"                it->second->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"                datas.push_back(std::make_pair(pair.first, std::move(msgData)));\n" +
			"            } else {\n" +
			"                removes.push_back(pair.first);\n" +
			"            }\n")
	}

	for i, member := range gameDataConfig.Member {
		if i == 0 {
			dstFile.WriteString("        if ")
		} else {
			dstFile.WriteString("        } else if ")
		}

		if member.Type == "string" {
			fun4(member.Attr, "wukong::pb::StringValue")
		} else if member.Type == "uint32" {
			fun4(member.Attr, "wukong::pb::Uint32Value")
		} else if member.Type == "int32" {
			fun4(member.Attr, "wukong::pb::Int32Value")
		} else if member.Type == "uint64" {
			fun4(member.Attr, "wukong::pb::Uint64Value")
		} else if member.Type == "int64" {
			fun4(member.Attr, "wukong::pb::Int64Value")
		} else if member.Type == "bool" {
			fun4(member.Attr, "wukong::pb::BoolValue")
		} else {
			if member.InnerList == "" {
				fun5(member.Attr)
			} else {
				fun6(member.Attr)
			}
		}
	}

	dstFile.WriteString("        }\n" +
		"    }\n" +
		"\n" +
		"    dirty_map_.clear();\n" +
		"}\n" +
		"\n")

	dstFile.WriteString("void " + pkgNameCapFirst + "GameObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {\n")

	fun7 := func(memberName, memberType string) {
		dstFile.WriteString("        auto msg = new " + memberType + ";\n" +
			"        msg->set_value(" + memberName + "_);\n" +
			"\n" +
			"        std::string msgData(msg->ByteSizeLong(), 0);\n" +
			"        uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"        msg->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"        datas.push_back(std::make_pair(\"" + memberName + "\", std::move(msgData)));\n" +
			"        delete msg;\n")
	}

	fun8 := func(memberName string) {
		dstFile.WriteString("        std::string msgData(" + memberName + "_->ByteSizeLong(), 0);\n" +
			"        uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"        " + memberName + "_->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"        datas.push_back(std::make_pair(\"" + memberName + "\", std::move(msgData)));\n")
	}

	fun9 := func(memberName, memberType, listName string) {
		dstFile.WriteString("        auto msg = new " + memberType + ";\n" +
			"        for (auto &pair : " + memberName + "_map_) {\n" +
			"            auto " + memberName + " = msg->add_" + listName + "();\n" +
			"            *" + memberName + " = *(pair.second);\n" +
			"        }\n" +
			"\n" +
			"        std::string msgData(msg->ByteSizeLong(), 0);\n" +
			"        uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"        msg->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"        datas.push_back(std::make_pair(\"" + memberName + "\", std::move(msgData)));\n" +
			"        delete msg;\n")
	}

	for _, member := range gameDataConfig.Member {
		dstFile.WriteString("    {\n")

		if member.Type == "string" {
			fun7(member.Attr, "wukong::pb::StringValue")
		} else if member.Type == "uint32" {
			fun7(member.Attr, "wukong::pb::Uint32Value")
		} else if member.Type == "int32" {
			fun7(member.Attr, "wukong::pb::Int32Value")
		} else if member.Type == "uint64" {
			fun7(member.Attr, "wukong::pb::Uint64Value")
		} else if member.Type == "int64" {
			fun7(member.Attr, "wukong::pb::Int64Value")
		} else if member.Type == "bool" {
			fun7(member.Attr, "wukong::pb::BoolValue")
		} else {
			if member.InnerList == "" {
				fun8(member.Attr)
			} else {
				fun9(member.Attr, member.Type, member.InnerList)
			}
		}

		dstFile.WriteString("    }\n")
	}

	dstFile.WriteString("}\n" +
		"\n")

	dstFile.WriteString("void " + pkgNameCapFirst + "GameObject::onEnterGame() {\n" +
		"    std::list<std::pair<std::string, std::string>> datas;\n" +
		"    buildAllDatas(datas);\n" +
		"    std::string msgData = ProtoUtils::marshalDataFragments(datas);\n" +
		"    send(wukong::S2C_MESSAGE_ID_ENTERGAME, 0, msgData);\n" +
		"}\n" +
		"\n")

	fun10 := func(memberName, memberType string) {
		mNameCapFirst := strings.Title(memberName)
		dstFile.WriteString(memberType + " " + pkgNameCapFirst + "GameObject::get" + mNameCapFirst + "() {\n" +
			"    return " + memberName + "_;\n" +
			"}\n" +
			"\n" +
			"void " + pkgNameCapFirst + "GameObject::set" + mNameCapFirst + "(" + memberType + " " + memberName + ") {\n" +
			"    " + memberName + "_ = " + memberName + ";\n" +
			"    dirty_map_[\"" + memberName + "\"] = true;\n" +
			"}\n" +
			"\n")
	}

	fun11 := func(memberName, memberType string) {
		mNameCapFirst := strings.Title(memberName)
		dstFile.WriteString(memberType + "* " + pkgNameCapFirst + "GameObject::get" + mNameCapFirst + "() {\n" +
			"    return " + memberName + "_;\n" +
			"}\n" +
			"\n" +
			"void " + pkgNameCapFirst + "GameObject::set" + mNameCapFirst + "Dirty() {\n" +
			"    dirty_map_[\"" + memberName + "\"] = true;\n" +
			"}\n" +
			"\n")
	}

	fun12 := func(memberName, memberType, itemType, itemId string) {
		mNameCapFirst := strings.Title(memberName)
		dstFile.WriteString("std::vector<uint32_t> " + pkgNameCapFirst + "GameObject::getAll" + mNameCapFirst + "Keys() {\n" +
			"    std::vector<uint32_t> keys;\n" +
			"    keys.reserve(" + memberName + "_map_.size());\n" +
			"\n" +
			"    for (auto &pair : " + memberName + "_map_) {\n" +
			"        keys.push_back(pair.first);\n" +
			"    }\n" +
			"\n" +
			"    return keys;\n" +
			"}\n" +
			"\n" +
			"bool " + pkgNameCapFirst + "GameObject::has" + mNameCapFirst + "(uint32_t " + itemId + ") {\n" +
			"    return " + memberName + "_map_.count(" + itemId + ") > 0;\n" +
			"}\n" +
			"\n" +
			"uint32_t " + pkgNameCapFirst + "GameObject::get" + mNameCapFirst + "Num() {\n" +
			"    return " + memberName + "_map_.size();\n" +
			"}\n" +
			"\n" +
			itemType + "* " + pkgNameCapFirst + "GameObject::get" + mNameCapFirst + "(uint32_t " + itemId + ") {\n" +
			"    auto it = " + memberName + "_map_.find(" + itemId + ");\n" +
			"    if (it != " + memberName + "_map_.end()) {\n" +
			"        return it->second;\n" +
			"    }\n" +
			"\n" +
			"    return nullptr;\n" +
			"}\n" +
			"\n" +
			"void " + pkgNameCapFirst + "GameObject::set" + mNameCapFirst + "Dirty(uint32_t " + itemId + ") {\n" +
			"    char dirtykey[50];\n" +
			"    sprintf(dirtykey,\"" + memberName + ".%d\"," + itemId + ");\n" +
			"    dirty_map_[dirtykey] = true;\n" +
			"}\n" +
			"\n" +
			"void " + pkgNameCapFirst + "GameObject::add" + mNameCapFirst + "(" + itemType + "* " + memberName + ") {\n" +
			"    " + memberName + "_map_.insert(std::make_pair(" + memberName + "->" + itemId + "(), " + memberName + "));\n" +
			"    set" + mNameCapFirst + "Dirty(" + memberName + "->" + itemId + "());\n" +
			"}\n" +
			"\n" +
			"void " + pkgNameCapFirst + "GameObject::remove" + mNameCapFirst + "(uint32_t " + itemId + ") {\n" +
			"    auto it = " + memberName + "_map_.find(" + itemId + ");\n" +
			"    if (it != " + memberName + "_map_.end()) {\n" +
			"        delete it->second;\n" +
			"        " + memberName + "_map_.erase(it);\n" +
			"        set" + mNameCapFirst + "Dirty(" + itemId + ");\n" +
			"    }\n" +
			"}\n" +
			"\n")
	}

	for _, member := range gameDataConfig.Member {
		if member.Type == "string" {
			fun10(member.Attr, "const std::string&")
		} else if member.Type == "uint32" {
			fun10(member.Attr, "uint32_t")
		} else if member.Type == "int32" {
			fun10(member.Attr, "int32_t")
		} else if member.Type == "uint64" {
			fun10(member.Attr, "uint64_t")
		} else if member.Type == "int64" {
			fun10(member.Attr, "int64_t")
		} else if member.Type == "bool" {
			fun10(member.Attr, "bool")
		} else {
			if member.InnerList == "" {
				fun11(member.Attr, member.Type)
			} else {
				fun12(member.Attr, member.Type, member.InnerListItemType, member.InnerListItemIdKey)
			}
		}
	}
}
