#ifndef wukong_proto_utils_h
#define wukong_proto_utils_h

#include <string>
#include <list>

namespace wukong {
    
    class ProtoUtils {
    public:
        static std::string marshalDataFragments(const std::list<std::pair<std::string, std::string>> &datas);
        static bool unmarshalDataFragments(const std::string &data, std::list<std::pair<std::string, std::string>> &datas);
    };

}

#endif /* wukong_proto_utils_h */
