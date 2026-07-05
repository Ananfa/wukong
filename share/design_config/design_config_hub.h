/*
 * 策划配置：注册多张表，按清单 JSON 批量加载。
 *
 * 使用步骤：
 * 1) 继承 DesignConfigTable，实现 designName()、onParse()（designName 与清单、registerTable 的 name 保持一致）。
 * 2) g_DesignConfigHub.registerTable("YourTable", std::make_shared<YourTable>(...));
 * 3) g_DesignConfigHub.loadFromManifest("/path/to/design_config_index.json");
 *
 * 清单为 JSON 对象，含数组 tables：每项 { "name": "YourTable", "path": "相对清单文件所在目录的路径" }。
 * 未 register 的 name 会打 WARN 并跳过。示例数据形态见 example_drop_table.json.template。
 *
 * 子类示例：
 *
 *   class YourTable : public DesignConfigTable {
 *   public:
 *     const char *designName() const override { return "YourTable"; }
 *   protected:
 *     bool onParse(const rapidjson::Document &doc) override {
 *       if (!doc.IsArray()) return false;
 *       for (rapidjson::SizeType i = 0; i < doc.Size(); ++i) { ... }
 *       return true;
 *     }
 *   };
 */

#ifndef wukong_design_config_hub_h
#define wukong_design_config_hub_h

#include "design_config_table.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace wukong {
namespace design_config {

class DesignConfigHub {
public:
    static DesignConfigHub &Instance();

    void clear();

    // 同一 name 重复注册时覆盖旧表
    void registerTable(const std::string &name, std::shared_ptr<DesignConfigTable> table);

    bool loadFromManifest(const char *manifestPath);

    // 按上次 loadFromManifest 记录的路径重载一张表（未加载过则失败）
    bool reloadTable(const char *name);

    std::shared_ptr<DesignConfigTable> getTable(const std::string &name);

    template <typename T>
    T *getTableAs(const std::string &name) {
        auto p = getTable(name);
        if (!p) {
            return nullptr;
        }
        return dynamic_cast<T *>(p.get());
    }

private:
    DesignConfigHub() = default;
    DesignConfigHub(const DesignConfigHub &) = delete;
    DesignConfigHub &operator=(const DesignConfigHub &) = delete;

    std::unordered_map<std::string, std::shared_ptr<DesignConfigTable>> tables_;
    std::unordered_map<std::string, std::string> loadedPath_;
};

} // namespace design_config
} // namespace wukong

#define g_DesignConfigHub wukong::design_config::DesignConfigHub::Instance()

#endif
