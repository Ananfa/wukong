#include "string_utils.h"

using namespace wukong;

void StringUtils::split(const std::string &input, const std::string &seperator, std::vector<std::string> &output) {
    std::string::size_type pos1, pos2;
    pos2 = input.find(seperator);
    pos1 = 0;
    while (std::string::npos != pos2) {
        output.push_back(input.substr(pos1, pos2 - pos1));
        
        pos1 = pos2 + seperator.size();
        pos2 = input.find(seperator, pos1);
    }
    
    if (pos1 != input.length()) {
        output.push_back(input.substr(pos1));
    }
}

void StringUtils::split(const std::string &input, const std::string &seperator, std::map<std::string, bool> &output) {
    std::string::size_type pos1, pos2;
    pos2 = input.find(seperator);
    pos1 = 0;
    while (std::string::npos != pos2) {
        output[input.substr(pos1, pos2 - pos1)] = true;
        
        pos1 = pos2 + seperator.size();
        pos2 = input.find(seperator, pos1);
    }
    
    if (pos1 != input.length()) {
        output[input.substr(pos1)] = true;
    }
}
