package main

import "os"

func genUtils() {
	hFile, _ := os.Create(targetPath + pkgName + "_utils.h")
	defer hFile.Close()

	genUtilsHeadFile(hFile)

	cppFile, _ := os.Create(targetPath + pkgName + "_utils.cpp")
	defer cppFile.Close()

	genUtilsCppFile(cppFile)
}

func genUtilsHeadFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#ifndef " + pkgName + "_utils_h\n" +
		"#define " + pkgName + "_utils_h\n" +
		"\n" +
		"#include <string>\n" +
		"#include <list>\n" +
		"#include <map>\n" +
		"#include \"share/define.h\"\n" +
		"\n" +
		"namespace " + pkgName + " {\n" +
		"    \n" +
		"    class " + pkgNameCapFirst + "Utils {\n" +
		"    public:\n" +
		"        static bool LoadProfile(RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas);\n" +
		"        static void MakeProfile(const std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &pDatas);\n" +
		"    };\n" +
		"\n" +
		"}\n" +
		"\n" +
		"#endif /* " + pkgName + "_utils_h */\n")
}

func genUtilsCppFile(dstFile *os.File) {
	dstFile.WriteString("// This file is generated. Don't edit it\n" +
		"\n" +
		"#include \"" + pkgName + "_utils.h\"\n" +
		"#include \"redis_utils.h\"\n" +
		"#include \"mysql_utils.h\"\n" +
		"#include \"redis_pool.h\"\n" +
		"#include \"mysql_pool.h\"\n" +
		"#include \"common.pb.h\"\n" +
		"\n" +
		"using namespace " + pkgName + ";\n" +
		"using namespace wukong;\n" +
		"\n" +
		"bool " + pkgNameCapFirst + "Utils::LoadProfile(RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas) {\n" +
		"    serverId = 0;\n" +
		"    pDatas.clear();\n" +
		"    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();\n" +
		"    if (!cache) {\n" +
		"        ERROR_LOG(\"connect to cache failed\\n\");\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    switch (RedisUtils::LoadProfile(cache, roleId, userId, serverId, pDatas)) {\n" +
		"        case REDIS_SUCCESS: {\n" +
		"            g_RedisPoolManager.getCoreCache()->put(cache, false);\n" +
		"            return true;\n" +
		"        }\n" +
		"        case REDIS_DB_ERROR: {\n" +
		"            g_RedisPoolManager.getCoreCache()->put(cache, true);\n" +
		"            ERROR_LOG(\"load role profile failed for db error\\n\");\n" +
		"            return false;\n" +
		"        }\n" +
		"    }\n" +
		"\n" +
		"    assert(pDatas.size() == 0);\n" +
		"\n" +
		"    std::list<std::pair<std::string, std::string>> rDatas;\n" +
		"\n" +
		"    if (RedisUtils::LoadRole(cache, roleId, userId, serverId, rDatas, false) == REDIS_DB_ERROR) {\n" +
		"        g_RedisPoolManager.getCoreCache()->put(cache, true);\n" +
		"        ERROR_LOG(\"load role data failed for db error\\n\");\n" +
		"        return false;\n" +
		"    }\n" +
		"    \n" +
		"    if (rDatas.size() > 0) {\n" +
		"        MakeProfile(rDatas, pDatas);\n" +
		"\n" +
		"        switch (RedisUtils::SaveProfile(cache, roleId, userId, serverId, pDatas)) {\n" +
		"            case REDIS_DB_ERROR: {\n" +
		"                g_RedisPoolManager.getCoreCache()->put(cache, true);\n" +
		"                ERROR_LOG(\"save role profile failed for db error\\n\");\n" +
		"                return false;\n" +
		"            }\n" +
		"            case REDIS_FAIL: {\n" +
		"                g_RedisPoolManager.getCoreCache()->put(cache, false);\n" +
		"                ERROR_LOG(\"save role profile failed\\n\");\n" +
		"                return false;\n" +
		"            }\n" +
		"        }\n" +
		"\n" +
		"        g_RedisPoolManager.getCoreCache()->put(cache, false);\n" +
		"        return true;\n" +
		"    }\n" +
		"    \n" +
		"    g_RedisPoolManager.getCoreCache()->put(cache, false);\n" +
		"\n" +
		"    MYSQL *mysql = g_MysqlPoolManager.getCoreRecord()->take();\n" +
		"    if (!mysql) {\n" +
		"        ERROR_LOG(\"connect to mysql failed\\n\");\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    std::string data;\n" +
		"    if (!MysqlUtils::LoadRole(mysql, roleId, userId, serverId, data)) {\n" +
		"        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);\n" +
		"\n" +
		"    if (data.empty()) {\n" +
		"        ERROR_LOG(\"no role data\\n\");\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    wukong::pb::DataFragments fragments;\n" +
		"    if (!fragments.ParseFromString(data)) {\n" +
		"        ERROR_LOG(\"parse role:%d data failed\\n\", roleId);\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    int fragNum = fragments.fragments_size();\n" +
		"    for (int i = 0; i < fragNum; i++) {\n" +
		"        auto &fragment = fragments.fragments(i);\n" +
		"        pDatas.push_back(std::make_pair(fragment.fragname(), fragment.fragdata()));\n" +
		"    }\n" +
		"\n" +
		"    if (pDatas.size() == 0) {\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    MakeProfile(rDatas, pDatas);\n" +
		"\n" +
		"    cache = g_RedisPoolManager.getCoreCache()->take();\n" +
		"    if (!cache) {\n" +
		"        ERROR_LOG(\"connect to cache failed\\n\");\n" +
		"        return false;\n" +
		"    }\n" +
		"\n" +
		"    switch (RedisUtils::SaveProfile(cache, roleId, userId, serverId, pDatas)) {\n" +
		"        case REDIS_DB_ERROR: {\n" +
		"            g_RedisPoolManager.getCoreCache()->put(cache, true);\n" +
		"            ERROR_LOG(\"save role profile failed for db error\\n\");\n" +
		"            return false;\n" +
		"        }\n" +
		"        case REDIS_FAIL: {\n" +
		"            g_RedisPoolManager.getCoreCache()->put(cache, false);\n" +
		"            ERROR_LOG(\"save role profile failed\\n\");\n" +
		"            return false;\n" +
		"        }\n" +
		"    }\n" +
		"\n" +
		"    g_RedisPoolManager.getCoreCache()->put(cache, false);\n" +
		"    return true;\n" +
		"}\n" +
		"\n" +
		"void " + pkgNameCapFirst + "Utils::MakeProfile(const std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &pDatas) {\n")

	var profileItems []string
	for _, m := range gameDataConfig.Member {
		if m.NeedProfile {
			profileItems = append(profileItems, m.Attr)
		}
	}

	if len(profileItems) > 0 {
		dstFile.WriteString("    for (auto &data : datas) {\n")

		for i, profileItem := range profileItems {
			if i == 0 {
				dstFile.WriteString("        if (data.first.compare(\"" + profileItem + "\") == 0")
			} else {
				dstFile.WriteString("            data.first.compare(\"" + profileItem + "\") == 0")
			}

			if i == len(profileItems) - 1 {
				dstFile.WriteString(") {\n")
			} else {
				dstFile.WriteString("|| \n")
			}
		}

		dstFile.WriteString("            pDatas.push_back(data);\n" +
			"        }\n" +
			"    }\n")
	}
	dstFile.WriteString("}\n")
}
