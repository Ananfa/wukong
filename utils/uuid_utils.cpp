#include <cstring>
#include <uuid/uuid.h>

#include "uuid_utils.h"

using namespace wukong;

std::string UUIDUtils::genUUID() {
    char buf[64] = {};

    uuid_t uu;
    uuid_generate(uu);

    int32_t index = 0;
    for (int32_t i = 0; i < 16; i++) {
        sprintf(buf+index, "%02x", uu[i]);
        index += 2;
    }

    return std::move(std::string(buf));
}