#ifndef wukong_url_h
#define wukong_url_h

#include <string>
#include <stdint.h>

namespace wukong {
    
    class UrlUtils {
    public :
        static void encode( const std::string & src, std::string & dst );
        static void decode( const std::string & src, std::string & dst );
    };

}

#endif /* wukong_url_h */
