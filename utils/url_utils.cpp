#include <cctype>
#include <cstdlib>

#include "url_utils.h"

using namespace wukong;

inline uint8_t TOHEX( uint8_t x )
{
    return ( x > 9 ? x + 55 : x + 48 );
}

inline uint8_t FROMHEX( uint8_t x )
{
    return ( x > 64 ? x - 55 : x - 48 );
}

void UrlUtils::encode( const std::string & src, std::string & dst )
{
    dst.clear();
    dst.reserve( src.size() * 3 + 1 );

    for ( size_t i = 0; i < src.size(); ++i )
    {
        char s = src[i];

        if ( isalnum( (uint8_t)( s ) )
                || s == '-' || s == '_' || s == '.' || s == '~' )
        {
            dst.push_back( s );
        }
        else
        {
            dst.push_back( '%' );
            dst.push_back( TOHEX( (uint8_t)s >> 4 ) );
            dst.push_back( TOHEX( (uint8_t)s % 16 ) );
        }
    }
}

void UrlUtils::decode( const std::string & src, std::string & dst )
{
    dst.clear();
    dst.reserve( src.size()+1 );

    for ( size_t i = 0; i < src.size(); ++i )
    {
        char c = 0;

        if ( src[i] == '%' )
        {
            c = FROMHEX( src[++i] );
            c = c << 4;
            c += FROMHEX( src[++i] );
        }
        else if ( src[i] == '+' )
        {
            c = ' ';
        }
        else
        {
            c = src[i];
        }

        dst.push_back( c );
    }
}
