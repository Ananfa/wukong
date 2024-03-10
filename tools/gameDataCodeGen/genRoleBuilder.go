package main

import (
	"os"
	"strings"
)

func genRoleBuilder() {
	hFile, _ := os.Create(targetPath + pkgName + "_role_builder.h")
	defer hFile.Close()

	genRoleBuilderHeadFile(hFile)

	cppFile, _ := os.Create(targetPath + pkgName + "_role_builder.cpp")
	defer cppFile.Close()

	genRoleBuilderCppFile(cppFile)
}

func genRoleBuilderHeadFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#ifndef " + pkgName + "_role_builder_h\n" +
		"#define " + pkgName + "_role_builder_h\n" +
		"\n" +
		"#include \"common.pb.h\"\n" +
		"#include \"" + pkgName + ".pb.h\"\n" +
		"#include <list>\n" +
		"#include <map>\n" +
		"#include <string>\n" +
		"\n" +
		"using namespace wukong;\n" +
		"\n" +
		"namespace " + pkgName + " {\n" +
		"\n" +
		"    class " + pkgNameCapFirst + "RoleBuilder {\n" +
		"    public:\n" +
		"        " + pkgNameCapFirst + "RoleBuilder();\n" +
		"        ~" + pkgNameCapFirst + "RoleBuilder();\n" +
		"\n" +
		"        void buildDatas(std::list<std::pair<std::string, std::string>> &datas);\n" +
		"\n")


	for _, member := range gameDataConfig.Member {
		mName := member.Attr
		mNameCapFirst := strings.Title(member.Attr)

		if member.Type == "string" {
			dstFile.WriteString("        void set" + mNameCapFirst + "(const std::string& " + mName + ");\n")
		} else if member.Type == "uint32" {
			dstFile.WriteString("        void set" + mNameCapFirst + "(uint32_t " + mName + ");\n")
		} else if member.Type == "int32" {
			dstFile.WriteString("        void set" + mNameCapFirst + "(int32_t " + mName + ");\n")
		} else if member.Type == "uint64" {
			dstFile.WriteString("        void set" + mNameCapFirst + "(uint64_t " + mName + ");\n")
		} else if member.Type == "int64" {
			dstFile.WriteString("        void set" + mNameCapFirst + "(int64_t " + mName + ");\n")
		} else if member.Type == "bool" {
			dstFile.WriteString("        void set" + mNameCapFirst + "(bool " + mName + ");\n")
		} else {
			if member.InnerList == "" {
				dstFile.WriteString("        " + member.Type + "* get" + mNameCapFirst + "();\n")
			} else {
				dstFile.WriteString("        void add" + mNameCapFirst + "(" + member.InnerListItemType + "* " + mName + ");\n")
			}
		}
	}

	dstFile.WriteString("\n" +
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
				dstFile.WriteString("        " + member.Type + "* " + member.Attr + "_;\n")
			} else {
				dstFile.WriteString("        std::map<uint32_t, " + member.InnerListItemType + "*> " + member.Attr + "_map_;\n")
			}
		}
	}

	dstFile.WriteString("    };\n" +
		"}\n" +
		"\n" +
		"#endif /* " + pkgName + "_role_builder_h */\n")
}

func genRoleBuilderCppFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#include \"" + pkgName + "_role_builder.h\"\n" +
		"\n" +
		"using namespace " + pkgName + ";\n" +
		"\n" +
		pkgNameCapFirst + "RoleBuilder::" + pkgNameCapFirst + "RoleBuilder() {\n")

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
		pkgNameCapFirst + "RoleBuilder::~" + pkgNameCapFirst + "RoleBuilder() {\n")

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

	dstFile.WriteString("}\n" +
		"\n" +
		"void " + pkgNameCapFirst + "RoleBuilder::buildDatas(std::list<std::pair<std::string, std::string>> &datas) {\n")

	fun1 := func(memberName, memberType string) {
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

	fun2 := func(memberName string) {
		dstFile.WriteString("    {\n" +
			"        std::string msgData(" + memberName + "_->ByteSizeLong(), 0);\n" +
			"        uint8_t *buf = (uint8_t *)msgData.data();\n" +
			"        " + memberName + "_->SerializeWithCachedSizesToArray(buf);\n" +
			"\n" +
			"        datas.push_back(std::make_pair(\"" + memberName + "\", std::move(msgData)));\n" +
			"    }\n")
	}

	fun3 := func(memberName, memberType, listName string) {
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
				fun2(member.Attr)
			} else {
				fun3(member.Attr, member.Type, member.InnerList)
			}
		}
	}

	dstFile.WriteString("}\n" +
		"\n")

	fun4 := func(memberName, memberType string) {
		mNameCapFirst := strings.Title(memberName)
		dstFile.WriteString("void " + pkgNameCapFirst + "RoleBuilder::set" + mNameCapFirst + "(" + memberType + " " + memberName + ") {\n" +
			"    " + memberName + "_ = " + memberName + ";\n" +
			"}\n" +
			"\n")
	}

	fun5 := func(memberName, memberType string) {
		mNameCapFirst := strings.Title(memberName)
		dstFile.WriteString(memberType + "* " + pkgNameCapFirst + "RoleBuilder::get" + mNameCapFirst + "() {\n" +
			"    return " + memberName + "_;\n" +
			"}\n" +
			"\n")
	}

	fun6 := func(memberName, itemType, itemId string) {
		mNameCapFirst := strings.Title(memberName)
		dstFile.WriteString("void " + pkgNameCapFirst + "RoleBuilder::add" + mNameCapFirst + "(" + itemType + "* " + memberName + ") {\n" +
			"    " + memberName + "_map_.insert(std::make_pair(" + memberName + "->" + itemId + "(), " + memberName + "));\n" +
			"}\n" +
			"\n")
	}

	for _, member := range gameDataConfig.Member {
		if member.Type == "string" {
			fun4(member.Attr, "const std::string&")
		} else if member.Type == "uint32" {
			fun4(member.Attr, "uint32_t")
		} else if member.Type == "int32" {
			fun4(member.Attr, "int32_t")
		} else if member.Type == "uint64" {
			fun4(member.Attr, "uint64_t")
		} else if member.Type == "int64" {
			fun4(member.Attr, "int64_t")
		} else if member.Type == "bool" {
			fun4(member.Attr, "bool")
		} else {
			if member.InnerList == "" {
				fun5(member.Attr, member.Type)
			} else {
				fun6(member.Attr, member.InnerListItemType, member.InnerListItemIdKey)
			}
		}
	}

}
