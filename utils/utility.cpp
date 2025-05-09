#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

#include "utility.h"
#include "string_utils.h"

using namespace wukong;

bool Utility::mkdirp(const char * path) {
    int32_t len = strlen(path);
    char *p = (char *)std::malloc(len + 2);
    
    if (p == NULL) {
        return false;
    }
    
    std::strcpy(p, path);
    
    if (p[len - 1] != '/') {
        p[len] = '/';
        len = len + 1;
    }
    
    for (int32_t i = 1; i < len; ++i) {
        if (p[i] != '/') {
            continue;
        }
        
        p[i] = 0;
        
        if (::access(p, F_OK) != 0) {
            ::mkdir(p, 0755);
        }
        
        p[i] = '/';
    }
    
    std::free(p);
    return true;
}

bool Utility::loadFileToString(const char* filename, std::string& str) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        str.clear();
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    str.resize(size + 1);
    char *buffer = (char *)str.data();

    size_t read = fread(buffer, 1, size, file);
    if (read != size) {
        perror("Failed to read file");
        str.clear();
        fclose(file);
        return false;
    }

    buffer[size] = '\0'; // Null-terminate the string
    fclose(file);

    return true;
}

bool Utility::isValidIdentifier(const std::string &str) {
    int32_t len = strlen(str.c_str());
    
    if (len > 0) {
        if(str[0] == '_' || (str[0] >= 'A' && str[0] <= 'Z') || (str[0] >= 'a' && str[0] <= 'z')) {
            for(int32_t i = 1; i < len; i++) {
                if (str[i] < '0' || (str[i] > '9' && str[i] < 'A') || (str[i] != '_' && (str[i] > 'Z' && str[i] < 'a')) || str[i] > 'z') {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

// 解析GameServer注册到ZooKeeper的信息
bool Utility::parseAddress(const std::string &input, std::map<uint16_t, std::pair<std::string, uint32_t> > &addresses) {
    std::vector<std::string> output;
    StringUtils::split(input, "|", output);
    
    if (output.size() != 3) return false;
    
    addresses[std::stoi(output[0])] = std::make_pair(output[1], std::stoi(output[2]));
    return true;
}

// 解析其他Server注册到ZooKeeper的信息
bool Utility::parseAddress(const std::string &input, std::vector<std::pair<std::string, uint32_t> > &addresses) {
    std::vector<std::string> output;
    StringUtils::split(input, ":", output);
    
    if (output.size() != 2) return false;
    
    addresses.push_back(std::make_pair(output[0], std::stoi(output[1])));
    return true;
}
