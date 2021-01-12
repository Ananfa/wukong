#ifndef wukong_utility_h
#define wukong_utility_h

#include <string>
#include <map>
#include <vector>

namespace wukong {
    
    class Utility {
    public:
        // 建立多级目录
        // 模拟mkdir -p的功能
        static bool mkdirp(const char * p);
        
        // 判断是否合法标识符
        static bool isValidIdentifier(const std::string& str);
        
        // 解析GameServer注册到ZooKeeper的信息
        static bool parseAddress(const std::string &input, std::map<uint16_t, std::pair<std::string, uint32_t> > &addresses);
        
        // 解析其他Server注册到ZooKeeper的信息
        static bool parseAddress(const std::string &input, std::vector<std::pair<std::string, uint32_t> > &addresses);
    };

}

#endif /* wukong_utility_h */
