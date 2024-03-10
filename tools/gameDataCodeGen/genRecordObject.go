package main

import (
	"os"
	"strconv"
)

func genRecordObject() {
	hFile, _ := os.Create(targetPath + pkgName + "_record_object.h")
	defer hFile.Close()

	genRecordObjectHeadFile(hFile)

	cppFile, _ := os.Create(targetPath + pkgName + "_record_object.cpp")
	defer cppFile.Close()

	genRecordObjectCppFile(cppFile)
}

func genRecordObjectHeadFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#ifndef " + pkgName + "_record_object_h\n" +
		"#define " + pkgName + "_record_object_h\n" +
		"\n" +
		"#include \"record_object.h\"\n" +
		"#include \"common.pb.h\"\n" +
		"#include \"" + pkgName + ".pb.h\"\n" +
		"#include <map>\n" +
		"#include <string>\n" +
		"\n" +
		"using namespace wukong;\n" +
		"\n" +
		"namespace " + pkgName + " {\n" +
		"        \n" +
		"    class " + pkgNameCapFirst + "RecordObject: public wukong::RecordObject {\n" +
		"    public:\n" +
		"        " + pkgNameCapFirst + "RecordObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &rToken, RecordObjectManager *manager);\n" +
		"        virtual ~" + pkgNameCapFirst + "RecordObject() {}\n" +
		"\n" +
		"        virtual bool initData(const std::list<std::pair<std::string, std::string>> &datas);\n" +
		"        virtual void syncIn(const ::wukong::pb::SyncRequest* request);\n" +
		"        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas);\n" +
		"        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas);\n" +
		"        \n" +
		"    private:\n")

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
				dstFile.WriteString("        " + member.Type + "* " + member.Attr + "_ = nullptr;\n")
			} else {
				dstFile.WriteString("        std::map<uint32_t, " + member.InnerListItemType + "*> " + member.Attr + "_map_;\n")
			}
		}
	}

	dstFile.WriteString("\n" +
		"    };\n" +
		"\n" +
		"}\n" +
		"\n" +
		"#endif /* " + pkgName + "_record_object_h */\n")
}

func genRecordObjectCppFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#include \"" + pkgName + "_record_object.h\"\n" +
		"#include \"corpc_utils.h\"\n" +
		"\n" +
		"using namespace " + pkgName + ";\n" +
		"\n" +
		pkgNameCapFirst + "RecordObject::" + pkgNameCapFirst + "RecordObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &rToken, RecordObjectManager *manager): wukong::RecordObject(userId, roleId, serverId, rToken, manager) {\n")

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
		"bool " + pkgNameCapFirst + "RecordObject::initData(const std::list<std::pair<std::string, std::string>> &datas) {\n" +
		"    for (auto &pair : datas) {\n")

	fun1 := func(memberName, memberType string) {
		dstFile.WriteString("            auto msg = new " + memberType + ";\n" +
			"            if (!msg->ParseFromString(pair.second)) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "GameObject::initData -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg;\n" +
			"                return false;\n" +
			"            }\n" +
			"\n" +
			"            " + memberName + "_ = msg->value();\n" +
			"            delete msg;\n")
	}

	fun2 := func(memberName, memberType string) {
		dstFile.WriteString("            auto msg = new " + memberType + ";\n" +
			"            if (!msg->ParseFromString(pair.second)) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "GameObject::initData -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg;\n" +
			"                return false;\n" +
			"            }\n" +
			"\n" +
			"            delete " + memberName + "_;\n" +
			"            " + memberName + "_ = msg;\n")
	}

	fun3 := func(memberName, memberType, listName, itemType, itemId string) {
		dstFile.WriteString("            auto msg = new " + memberType + ";\n" +
			"            if (!msg->ParseFromString(pair.second)) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "GameObject::initData -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg;\n" +
			"                return false;\n" +
			"            }\n" +
			"\n" +
			"            int " + memberName + "Num = msg->" + listName + "_size();\n" +
			"            for (int j = 0; j < " + memberName + "Num; j++) {\n" +
			"                auto " + memberName + " = new " + itemType + "(msg->" + listName + "(j));\n" +
			"                " + memberName + "_map_.insert(std::make_pair(" + memberName + "->" + itemId + "(), " + memberName + "));\n" +
			"            }\n" +
			"\n" +
			"            delete msg;\n")
	}

	for i, member := range gameDataConfig.Member {
		if i == 0 {
			dstFile.WriteString("        if ")
		} else {
			dstFile.WriteString("        } else if ")
		}
		dstFile.WriteString("(pair.first.compare(\"" + member.Attr + "\") == 0) {\n")

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
		"    return true;\n" +
		"}\n" +
		"\n" +
		"void " + pkgNameCapFirst + "RecordObject::syncIn(const ::wukong::pb::SyncRequest* request) {\n" +
		"    int dataNum = request->datas_size();\n" +
		"\n" +
		"    for (int i = 0; i < dataNum; ++i) {\n" +
		"        auto &data = request->datas(i);\n" +
		"\n")

	fun4 := func(memberName, memberType string) {
		dstFile.WriteString("(data.key().compare(\"" + memberName + "\") == 0) {\n" +
			"            auto msg = new " + memberType + ";\n" +
			"            if (!msg->ParseFromString(data.value())) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "RecordObject::syncIn -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg;\n" +
			"                continue;\n" +
			"            }\n" +
			"\n" +
			"            " + memberName + "_ = msg->value();\n" +
			"            delete msg;\n" +
			"            dirty_map_[\"" + memberName + "\"] = true;\n")
	}

	fun5 := func(memberName, memberType string) {
		dstFile.WriteString("(data.key().compare(\"" + memberName + "\") == 0) {\n" +
			"            auto msg = new " + memberType + ";\n" +
			"            if (!msg->ParseFromString(data.value())) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "RecordObject::syncIn -- parse role:%d data--" + memberName + " failed\\n\", roleId_);\n" +
			"                delete msg;\n" +
			"                continue;\n" +
			"            }\n" +
			"\n" +
			"            delete " + memberName + "_;\n" +
			"            " + memberName + "_ = msg;\n" +
			"            dirty_map_[\"" + memberName + "\"] = true;\n")
	}

	fun6 := func(memberName, itemType string) {
		dstFile.WriteString("(data.key().compare(0, " + strconv.Itoa(len(memberName)+1) + ", \"" + memberName + ".\") == 0) {\n" +
			"            std::string idStr = data.key().substr(" + strconv.Itoa(len(memberName)+1) + ");\n" +
			"            uint32_t id = atoi(idStr.c_str());\n" +
			"\n" +
			"            auto msg = new " + itemType + ";\n" +
			"            if (!msg->ParseFromString(data.value())) {\n" +
			"                ERROR_LOG(\"" + pkgNameCapFirst + "RecordObject::syncIn -- parse role:%d data--" + memberName + ":%d failed\\n\", roleId_, id);\n" +
			"                delete msg;\n" +
			"                continue;\n" +
			"            }\n" +
			"\n" +
			"            " + memberName + "_map_[id] = msg;\n" +
			"            dirty_map_[\"" + memberName + "\"] = true;\n")
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
				fun5(member.Attr, member.Type)
			} else {
				fun6(member.Attr, member.InnerListItemType)
			}
		}
	}

	dstFile.WriteString("        } else {\n" +
		"            ERROR_LOG(\"" + pkgNameCapFirst + "RecordObject::syncIn -- parse role:%d data--unknown data: %s\\n\", roleId_, data.key().c_str());\n" +
		"        }\n" +
		"    }\n" +
		"\n" +
		"    int removeNum = request->removes_size();\n" +
		"    for (int i = 0; i < removeNum; ++i) {\n" +
		"        auto &remove = request->removes(i);\n" +
		"        ")

	for _, member := range gameDataConfig.Member {
		if member.InnerList != "" {
			dstFile.WriteString("if (remove.compare(0, " + strconv.Itoa(len(member.Attr)+1) + ", \"" + member.Attr + ".\") == 0) {\n" +
				"            std::string idStr = remove.substr(" + strconv.Itoa(len(member.Attr)+1) + ");\n" +
				"            uint32_t id = atoi(idStr.c_str());\n" +
				"\n" +
				"            auto it = " + member.Attr + "_map_.find(id);\n" +
				"            if (it != " + member.Attr + "_map_.end()) {\n" +
				"                delete it->second;\n" +
				"                " + member.Attr + "_map_.erase(it);\n" +
				"                dirty_map_[\"" + member.Attr + "\"] = true;\n" +
				"            } else {\n" +
				"                WARN_LOG(\"" + pkgNameCapFirst + "RecordObject::syncIn -- remove role:%d data--" + member.Attr + " %d not exist\\n\", roleId_, id);\n" +
				"            }\n" +
				"        } else ")
		}
	}

	dstFile.WriteString("{\n" +
		"            ERROR_LOG(\"" + pkgNameCapFirst + "RecordObject::syncIn -- remove role:%d data--unknown data: %s\\n\", roleId_, remove.c_str());\n" +
		"        }\n" +
		"    }\n" +
		"}\n" +
		"\n" +
		"void " + pkgNameCapFirst + "RecordObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas) {\n" +
		"    for (auto &pair : dirty_map_) {\n")

	fun7 := func(memberName, memberType string) {
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

	fun8 := func(memberName string) {
		dstFile.WriteString("(pair.first.compare(\"" + memberName + "\") == 0) {\n" +
			"            std::string msgData(" + memberName + "_->ByteSizeLong(), 0);\n" +
			"            uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"            " + memberName + "_->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"            datas.push_back(std::make_pair(pair.first, std::move(msgData)));\n")
	}

	fun9 := func(memberName, memberType, listName string) {
		dstFile.WriteString("(pair.first.compare(\"" + memberName + "\") == 0) {\n" +
			"            auto msg = new " + memberType + ";\n" +
			"            for (auto &pair1 : " + memberName + "_map_) {\n" +
			"                auto " + memberName + " = msg->add_" + listName + "();\n" +
			"                *" + memberName + " = *(pair1.second);\n" +
			"            }\n" +
			"\n" +
			"            std::string msgData(msg->ByteSizeLong(), 0);\n" +
			"            uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"            msg->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"            datas.push_back(std::make_pair(pair.first, std::move(msgData)));\n" +
			"            delete msg;\n")
	}

	for i, member := range gameDataConfig.Member {
		if i == 0 {
			dstFile.WriteString("        if ")
		} else {
			dstFile.WriteString("        } else if ")
		}

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
	}

	dstFile.WriteString("        }\n" +
		"    }\n" +
		"}\n" +
		"\n" +
		"void " + pkgNameCapFirst + "RecordObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {\n")

	fun10 := func(memberName, memberType string) {
		dstFile.WriteString("    {\n" +
			"        auto msg = new " + memberType + ";\n" +
			"        msg->set_value(" + memberName + "_);\n" +
			"\n" +
			"        std::string msgData(msg->ByteSizeLong(), 0);\n" +
			"        uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"        msg->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"        datas.push_back(std::make_pair(\"" + memberName + "\", std::move(msgData)));\n" +
			"        delete msg;\n" +
			"    }\n")
	}

	fun11 := func(memberName string) {
		dstFile.WriteString("    {\n" +
			"        std::string msgData(" + memberName + "_->ByteSizeLong(), 0);\n" +
			"        uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"        " + memberName + "_->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"        datas.push_back(std::make_pair(\"" + memberName + "\", std::move(msgData)));\n" +
			"    }\n")
	}

	fun12 := func(memberName, memberType, listName string) {
		dstFile.WriteString("    {\n" +
			"        auto msg = new " + memberType + ";\n" +
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
			"        delete msg;\n" +
			"    }\n")
	}

	for _, member := range gameDataConfig.Member {
		if member.Type == "string" {
			fun10(member.Attr, "wukong::pb::StringValue")
		} else if member.Type == "uint32" {
			fun10(member.Attr, "wukong::pb::Uint32Value")
		} else if member.Type == "int32" {
			fun10(member.Attr, "wukong::pb::Int32Value")
		} else if member.Type == "uint64" {
			fun10(member.Attr, "wukong::pb::Uint64Value")
		} else if member.Type == "int64" {
			fun10(member.Attr, "wukong::pb::Int64Value")
		} else if member.Type == "bool" {
			fun10(member.Attr, "wukong::pb::BoolValue")
		} else {
			if member.InnerList == "" {
				fun11(member.Attr)
			} else {
				fun12(member.Attr, member.Type, member.InnerList)
			}
		}
	}

	dstFile.WriteString("}")
}
