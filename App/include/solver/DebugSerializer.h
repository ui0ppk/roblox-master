#pragma once

#include "util/G3DCore.h"
#include "util/Quaternion.h"

#include "boost/type_traits.hpp"
#include "boost/utility.hpp"
#include "boost/cstdint.hpp"
#include "boost/container/map.hpp"

#include "simd/simd.h"
#include "rbx/ArrayDynamic.h"

#include <vector>
#include <map>
#include <string>

namespace RBX
{

class DebugSerializer;

// Compile time 'Has X Method' implementation using 'Substitution Failure is not an Error'
template< typename T >
struct HasSerializeMethod
{
private:
    template<typename U, void (U::*)( DebugSerializer& ) const> struct SFINAE {};
    template<typename U> static char Test(SFINAE<U, &U::serialize>*);
    template<typename U> static int Test(...);
public:
    static const bool value = sizeof(Test<T>(0)) == sizeof(char);
};

class DebugSerializerScope
{
public:
    DebugSerializerScope( DebugSerializer& s );
    ~DebugSerializerScope();

private:
    DebugSerializer& serializer;
    size_t reservedBufferIndex;
    size_t currentSize;
};

class DebugSerializer
{
public:
    void clear()
    {
        data.clear();
    }

    template< class T >
    typename boost::enable_if< boost::is_arithmetic< T >, DebugSerializer >::type& storeAt( const T& t, size_t index )
    {
        union
        {
            T t;
            char bytes[ sizeof( T ) ];
        } u;
        u.t = t;
        for( size_t i = 0; i < sizeof( T ); i++ )
        {
            data[ index + i ] = u.bytes[ i ];
        }
        return *this;
    }

    template< class T >
    typename boost::enable_if< boost::is_arithmetic< T >, DebugSerializer >::type& operator&( const T& t )
    {
        size_t index = data.size();
        data.resize( data.size() + sizeof( T ) );
        storeAt( t, index );
        return *this;
    }

    template< class T >
    typename boost::enable_if< boost::is_enum< T >, DebugSerializer >::type& operator&( const T& t )
    {
        union
        {
            T t;
            boost::uint8_t bytes[ sizeof( T ) ];
        } u;
        u.t = t;
        for( size_t i = 0; i < sizeof( T ); i++ )
        {
            data.push_back( u.bytes[ i ] );
        }
        return *this;
    }

    DebugSerializer& operator&( const Vector3& v )
    {
        *this & v.x & v.y & v.z;
        return *this;
    }

    DebugSerializer& operator&( const Quaternion& q )
    {
        *this & q.x & q.y & q.z & q.w;
        return *this;
    }

    DebugSerializer& operator&( const Matrix3& m )
    {
        *this & m.row( 0 ) & m.row( 1 ) & m.row( 2 );
        return *this;
    }

    DebugSerializer& operator&( const simd::v4f& v )
    {
        *this & simd::extractSlow( v, 0 ) & simd::extractSlow( v, 1 ) & simd::extractSlow( v, 2 ) & simd::extractSlow( v, 3 );
        return *this;
    }

    template< class T, class U >
    DebugSerializer& operator&( const std::pair< T, U >& p )
    {
        *this & p.first & p.second;
        return *this;
    }

    template< class T >
    DebugSerializer& operator&( const ArrayBase<T>& v )
    {
        *this & boost::uint32_t( v.size() );
        for( const auto& e : v )
        {
            *this & e;
        }
        return *this;
    }

    template< class T >
    DebugSerializer& operator&( const std::vector<T>& v )
    {
        *this & boost::uint32_t( v.size() );
        for( const auto& e : v )
        {
            *this & e;
        }
        return *this;
    }

    template< class T >
    typename boost::enable_if_c< HasSerializeMethod< T >::value && !boost::is_pointer< T >::value, DebugSerializer >::type& operator&( const T& t )
    {
        t.serialize( *this );
        return *this;
    }

    template< class T >
    typename boost::enable_if_c< HasSerializeMethod< T >::value, DebugSerializer >::type& operator&( const T* const t )
    {
        t->serialize( *this );
        return *this;
    }

    DebugSerializer& tag( const char* name )
    {
        boost::uint8_t length = strlen(name);
        *this & length;
        for( boost::uint8_t i = 0; i < length; i++ )
        {
            *this & name[ i ];
        }
        return *this;
    }

    std::vector< char > data;
};

inline DebugSerializerScope::DebugSerializerScope( DebugSerializer& s ): serializer( s )
{
    reservedBufferIndex = s.data.size();
    size_t size = 0;
    s & size;
//     size_t checkSum = 0;
//     s & checkSum;
    currentSize = s.data.size();
}

inline DebugSerializerScope::~DebugSerializerScope()
{
    size_t size = serializer.data.size() - currentSize;
    serializer.storeAt( size, reservedBufferIndex );
}

}
