/*
 * 策划配置：单表抽象。业务侧继承并实现 onParse，由 Hub 或自行调用 loadFromPath。
 */

#ifndef wukong_design_config_table_h
#define wukong_design_config_table_h

#include "rapidjson/document.h"

namespace wukong {
namespace design_config {

class DesignConfigTable {
public:
    virtual ~DesignConfigTable() = default;

    // 逻辑名，与 manifest 里 tables[].name 一致
    virtual const char *designName() const = 0;

    // 从已解析 JSON 根节点加载（整文件即一张表，或根下再分子节点由子类约定）
    bool loadFromPath(const char *path);

protected:
    virtual bool onParse(const rapidjson::Document &doc) = 0;
};

} // namespace design_config
} // namespace wukong

#endif
