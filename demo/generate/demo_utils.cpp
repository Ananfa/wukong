// TODO：此文件通过工具生成

#include "demo_utils.h"

using namespace demo;

void DemoUtils::MakeProfile(const std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &pDatas) {
    // 注意：下面这段循环在不同生成代码中会不一样
    for (auto &data : datas) {
        if (data.first.compare("name") == 0 || 
            data.first.compare("lv") == 0) {
            pDatas.push_back(data);
        }
    }
}