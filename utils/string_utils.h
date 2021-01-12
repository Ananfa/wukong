#ifndef wukong_string_utils_h
#define wukong_string_utils_h

#include <string>
#include <vector>
#include <map>

namespace wukong {
    
    class StringUtils {
    public:
        static void split(const std::string &input, const std::string &seperator, std::vector<std::string> &output);
        static void split(const std::string &input, const std::string &seperator, std::map<std::string, bool> &output);
    };

}

#endif /* wukong_string_utils_h */
