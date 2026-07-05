/*
 * 策划配置：清单加载
 */

#include "design_config_hub.h"
#include "design_json_io.h"
#include "corpc_utils.h"

#include "rapidjson/document.h"

namespace wukong {
namespace design_config {

using namespace rapidjson;

DesignConfigHub &DesignConfigHub::Instance() {
    static DesignConfigHub hub;
    return hub;
}

void DesignConfigHub::clear() {
    tables_.clear();
    loadedPath_.clear();
}

void DesignConfigHub::registerTable(const std::string &name, std::shared_ptr<DesignConfigTable> table) {
    if (name.empty() || !table) {
        return;
    }
    tables_[name] = std::move(table);
}

bool DesignConfigHub::loadFromManifest(const char *manifestPath) {
    if (!manifestPath) {
        return false;
    }
    Document doc;
    std::string err;
    if (!readJsonFile(manifestPath, &doc, &err)) {
        return false;
    }
    if (!doc.IsObject() || !doc.HasMember("tables") || !doc["tables"].IsArray()) {
        ERROR_LOG("design_config: manifest need object.tables array %s\n", manifestPath);
        return false;
    }
    const Value &arr = doc["tables"];
    for (SizeType i = 0; i < arr.Size(); ++i) {
        const Value &item = arr[i];
        if (!item.IsObject() || !item.HasMember("name") || !item.HasMember("path")) {
            ERROR_LOG("design_config: manifest tables[%u] need name and path\n", (unsigned)i);
            return false;
        }
        if (!item["name"].IsString() || !item["path"].IsString()) {
            ERROR_LOG("design_config: manifest tables[%u] name/path must be string\n", (unsigned)i);
            return false;
        }
        std::string name(item["name"].GetString(), item["name"].GetStringLength());
        std::string rel(item["path"].GetString(), item["path"].GetStringLength());
        auto it = tables_.find(name);
        if (it == tables_.end()) {
            WARN_LOG("design_config: manifest references unregistered table %s (skip)\n", name.c_str());
            continue;
        }
        std::string full = resolveDataPath(manifestPath, rel.c_str());
        if (!it->second->loadFromPath(full.c_str())) {
            ERROR_LOG("design_config: load table %s failed path=%s\n", name.c_str(), full.c_str());
            return false;
        }
        loadedPath_[name] = std::move(full);
    }
    return true;
}

bool DesignConfigHub::reloadTable(const char *name) {
    if (!name) {
        return false;
    }
    auto pit = loadedPath_.find(name);
    if (pit == loadedPath_.end()) {
        ERROR_LOG("design_config: reloadTable unknown or never loaded: %s\n", name);
        return false;
    }
    auto tit = tables_.find(name);
    if (tit == tables_.end() || !tit->second) {
        return false;
    }
    return tit->second->loadFromPath(pit->second.c_str());
}

std::shared_ptr<DesignConfigTable> DesignConfigHub::getTable(const std::string &name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return it->second;
}

} // namespace design_config
} // namespace wukong
