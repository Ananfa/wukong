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
        JsonWriter(): _writer(_buffer) {}
        virtual ~JsonWriter() {}
        
        template<class T> void put(T const & value);
        
        template<class T>
        void put(const std::vector<T> &values) {
            _writer.StartArray();

            for (const T &value : values){
                put(value);
            }

            _writer.EndArray();
        }
        
        template<class T>
        void put(const std::string &key, const T &value) {
            _writer.Key(key.c_str());
            put(value);
        }
        
        // 重载char[]
        void put(const char *value) { _writer.String(value); }

        void putRawObject(const std::string &key, const std::string &value) {
            _writer.Key(key.c_str());
            _writer.RawValue(value.c_str(), value.size(), rapidjson::kObjectType);
        }
        
        void putRawArray(const std::string &key, const std::string &value) {
            _writer.Key(key.c_str());
            _writer.RawValue(value.c_str(), value.size(), rapidjson::kArrayType);
        }
        
        void start() { _writer.StartObject(); }
        void end() { _writer.EndObject(); }
        
        size_t length() const { return _buffer.GetSize(); }
        const char* content() const { return _buffer.GetString(); }
        
    private:
        rapidjson::StringBuffer _buffer;
        rapidjson::Writer<rapidjson::StringBuffer> _writer;
    };

    // 特化常用数值类型
    template<> void JsonWriter::put(const int8_t &value) { _writer.Int(value); }
    template<> void JsonWriter::put(const uint8_t &value) { _writer.Uint(value); }
    template<> void JsonWriter::put(const int16_t &value) { _writer.Int(value); }
    template<> void JsonWriter::put(const uint16_t &value) { _writer.Uint(value); }
    template<> void JsonWriter::put(const int32_t &value) { _writer.Int(value); }
    template<> void JsonWriter::put(const uint32_t &value) { _writer.Uint(value); }
    template<> void JsonWriter::put(const int64_t &value) { _writer.Int64(value); }
    template<> void JsonWriter::put(const uint64_t &value) { _writer.Uint64(value); }
    template<> void JsonWriter::put(const double &value) { _writer.Double(value); }

    #ifdef __APPLE__
    template<> void JsonWriter::put(const long &value) { _writer.Uint64(value); }
    #endif

    // 特化STL类型
    template<> void JsonWriter::put(const bool &value) { _writer.Bool(value); }
    template<> void JsonWriter::put(const std::string &value) { _writer.String(value.c_str()); }

}

#endif /* wukong_json_utils_h */
