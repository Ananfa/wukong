/*
 * 策划配置：JSON 文件读取
 */

#include "design_json_io.h"
#include "corpc_utils.h"

#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h"

#include <cstdio>
#include <cstring>

namespace wukong {
namespace design_config {

namespace {

struct FileGuard {
    FILE *f;
    explicit FileGuard(FILE *fp) : f(fp) {}
    ~FileGuard() {
        if (f) {
            fclose(f);
        }
    }
};

} // namespace

bool readJsonFile(const char *path, rapidjson::Document *out, std::string *err) {
    if (!path || !out) {
        if (err) {
            *err = "invalid argument";
        }
        return false;
    }
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        if (err) {
            *err = std::string("cannot open: ") + path;
        }
        ERROR_LOG("design_config: cannot open json %s\n", path);
        return false;
    }
    FileGuard guard(fp);
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    out->ParseStream(is);
    if (out->HasParseError()) {
        if (err) {
            char buf[256];
            snprintf(buf, sizeof(buf), "parse error at byte %zu: %s",
                     out->GetErrorOffset(), rapidjson::GetParseError_En(out->GetParseError()));
            *err = buf;
        }
        ERROR_LOG("design_config: json parse error %s\n", path);
        return false;
    }
    return true;
}

std::string resolveDataPath(const char *manifestPath, const char *relativeOrAbsolutePath) {
    if (!relativeOrAbsolutePath || relativeOrAbsolutePath[0] == '\0') {
        return std::string();
    }
    if (relativeOrAbsolutePath[0] == '/') {
        return std::string(relativeOrAbsolutePath);
    }
#if defined(_WIN32)
    if (strlen(relativeOrAbsolutePath) > 2 && relativeOrAbsolutePath[1] == ':') {
        return std::string(relativeOrAbsolutePath);
    }
#endif
    std::string base;
    if (manifestPath) {
        const char *last = strrchr(manifestPath, '/');
        if (!last) {
            last = strrchr(manifestPath, '\\');
        }
        if (last) {
            base.assign(manifestPath, last - manifestPath + 1);
        }
    }
    return base + relativeOrAbsolutePath;
}

} // namespace design_config
} // namespace wukong
