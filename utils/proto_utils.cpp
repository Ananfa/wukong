#include "proto_utils.h"

#include "common.pb.h"

using namespace wukong;

std::string ProtoUtils::marshalDataFragments(std::list<std::pair<std::string, std::string>> &datas) {
    wukong::pb::DataFragments fragments;
    for (auto &data : datas) {
        auto fragment = fragments.add_fragments();
        fragment->set_fragname(data.first);
        fragment->set_fragdata(data.second);
    }

    std::string rData(fragments.ByteSize(), 0);
    uint8_t *buf = (uint8_t *)rData.data();
    fragments.SerializeWithCachedSizesToArray(buf);

    return std::move(rData);
}