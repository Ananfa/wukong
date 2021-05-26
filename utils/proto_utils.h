#ifndef wukong_proto_utils_h
#define wukong_proto_utils_h

#include <string>

namespace wukong {
    
    class ProtoUtils {
    public:
        static std::string marshalDataFragments(std::list<std::pair<std::string, std::string>> &datas);
    };

}

#endif /* wukong_proto_utils_h */
