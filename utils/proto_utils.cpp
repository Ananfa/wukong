#include "proto_utils.h"

#include "common.pb.h"
#include "corpc_utils.h"

using namespace wukong;

std::string ProtoUtils::marshalDataFragments(const std::list<std::pair<std::string, std::string>> &datas) {
    wukong::pb::DataFragments fragments;
    for (auto &data : datas) {
        auto fragment = fragments.add_fragments();
        fragment->set_fragname(data.first);
        fragment->set_fragdata(data.second);
    }

    std::string rData(fragments.ByteSizeLong(), 0);
    uint8_t *buf = (uint8_t *)rData.data();
    fragments.SerializeWithCachedSizesToArray(buf);

    return std::move(rData);
}

bool ProtoUtils::unmarshalDataFragments(const std::string &data, std::list<std::pair<std::string, std::string>> &datas) {
    wukong::pb::DataFragments fragments;
    if (!fragments.ParseFromString(data)) {
        ERROR_LOG("ProtoUtils::unmarshalDataFragments -- parse data failed\n");
        return false;
    }

    int fragNum = fragments.fragments_size();
    for (int i = 0; i < fragNum; i++) {
        auto &fragment = fragments.fragments(i);
        datas.push_back(std::make_pair(fragment.fragname(), fragment.fragdata()));
    }
    return true;
}
