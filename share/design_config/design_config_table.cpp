/*
 * 策划配置：单表加载默认实现
 */

#include "design_config_table.h"
#include "design_json_io.h"
#include "corpc_utils.h"

namespace wukong {
namespace design_config {

bool DesignConfigTable::loadFromPath(const char *path) {
    if (!path) {
        ERROR_LOG("design_config: %s loadFromPath null path\n", designName());
        return false;
    }
    rapidjson::Document doc;
    std::string err;
    if (!readJsonFile(path, &doc, &err)) {
        ERROR_LOG("design_config: %s load failed %s (%s)\n", designName(), path, err.c_str());
        return false;
    }
    if (!onParse(doc)) {
        ERROR_LOG("design_config: %s onParse failed %s\n", designName(), path);
        return false;
    }
    return true;
}

} // namespace design_config
} // namespace wukong
