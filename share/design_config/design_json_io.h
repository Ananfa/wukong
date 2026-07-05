/*
 * 策划配置：JSON 文件读取（RapidJSON）
 */

#ifndef wukong_design_json_io_h
#define wukong_design_json_io_h

#include "rapidjson/document.h"
#include <string>

namespace wukong {
namespace design_config {

// 将 UTF-8 JSON 文件解析为 Document；失败时 err 可填可读说明（可为 nullptr）
bool readJsonFile(const char *path, rapidjson::Document *out, std::string *err);

// manifest 中 path 相对路径时，与 manifest 所在目录拼接得到绝对路径
std::string resolveDataPath(const char *manifestPath, const char *relativeOrAbsolutePath);

} // namespace design_config
} // namespace wukong

#endif
