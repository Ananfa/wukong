#ifndef wukong_json_utils_h
#define wukong_json_utils_h

#include <cstdio>
#include <vector>
#include <string>

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/rapidjson.h"

namespace wukong {
    
    class JsonWriter {
    public:
        JsonWriter(): writer_(buffer_) {}
        virtual ~JsonWriter() {}
        
        template<class T> void put(T const & value);
        
        template<class T>
        void put(const std::vector<T> &values) {
            writer_.StartArray();

            for (const T &value : values){
                put(value);
            }

            writer_.EndArray();
        }
        
        template<class T>
        void put(const std::string &key, const T &value) {
            writer_.Key(key.c_str());
            put(value);
        }
        
        // 重载char[]
        void put(const char *value) { writer_.String(value); }

        void putRawObject(const std::string &key, const std::string &value) {
            writer_.Key(key.c_str());
            writer_.RawValue(value.c_str(), value.size(), rapidjson::kObjectType);
        }
        
        void putRawArray(const std::string &key, const std::string &value) {
            writer_.Key(key.c_str());
            writer_.RawValue(value.c_str(), value.size(), rapidjson::kArrayType);
        }
        
        void start() { writer_.StartObject(); }
        void end() { writer_.EndObject(); }
        
        size_t length() const { return buffer_.GetSize(); }
        const char* content() const { return buffer_.GetString(); }
        
    private:
        rapidjson::StringBuffer buffer_;
        rapidjson::Writer<rapidjson::StringBuffer> writer_;
    };

    // 特化常用数值类型
    template<> void JsonWriter::put(const int8_t &value) { writer_.Int(value); }
    template<> void JsonWriter::put(const uint8_t &value) { writer_.Uint(value); }
    template<> void JsonWriter::put(const int16_t &value) { writer_.Int(value); }
    template<> void JsonWriter::put(const uint16_t &value) { writer_.Uint(value); }
    template<> void JsonWriter::put(const int32_t &value) { writer_.Int(value); }
    template<> void JsonWriter::put(const uint32_t &value) { writer_.Uint(value); }
    template<> void JsonWriter::put(const int64_t &value) { writer_.Int64(value); }
    template<> void JsonWriter::put(const uint64_t &value) { writer_.Uint64(value); }
    template<> void JsonWriter::put(const double &value) { writer_.Double(value); }

    #ifdef __APPLE__
    template<> void JsonWriter::put(const long &value) { writer_.Uint64(value); }
    #endif

    // 特化STL类型
    template<> void JsonWriter::put(const bool &value) { writer_.Bool(value); }
    template<> void JsonWriter::put(const std::string &value) { writer_.String(value.c_str()); }

}

#endif /* wukong_json_utils_h */
