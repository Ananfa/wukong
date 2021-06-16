// TODO：此文件通过工具生成

#ifndef demo_utils_h
#define demo_utils_h

#include <string>
#include <list>
#include <map>

namespace demo {
    
    class DemoUtils {
    public:
        static void MakeProfile(const std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &pDatas);
    };

}

#endif /* demo_utils_h */
