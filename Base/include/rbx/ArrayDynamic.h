#pragma once

#include "rbx/Debug.h"
#include "boost/utility.hpp"
#include "boost/type_traits.hpp"

#include <stdlib.h>

#ifdef __ANDROID__
#include <malloc.h>
#endif

namespace RBX
{

//
// ArrayBase
//
template< class T >
class ArrayBase
{
public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef T* reverse_iterator;
    typedef const T* const_reverse_iterator;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

    inline T& operator[]( size_t i );
    inline const T& operator[]( size_t i ) const;
    inline T& at( size_t i );
    inline const T& at( size_t i ) const;
    inline size_t size() const;
    inline T& front();
    inline const T& front() const;
    inline T& back();
    inline const T& back() const;
    inline T* begin();
    inline const T* begin() const;
    inline const T* cbegin() const;
    inline T* end();
    inline const T* end() const;
    inline const T* cend() const;
    inline T* data();
    inline const T* data() const;
    inline const T* cdata() const;
    inline bool empty() const;

protected:
    inline ArrayBase( T* _data, size_t _size );
    inline ArrayBase( const ArrayBase< T >& a );

private:
    inline ArrayBase< T >& operator=( const ArrayBase< T >& src );

protected:
    T* mData;
    size_t mSize;
};

//
// ArrayBase Implementation
//
template< class T >
ArrayBase< T >::ArrayBase( T* _data, size_t _size ): mData( _data ), mSize( _size ) { }

template< class T >
ArrayBase< T >::ArrayBase( const ArrayBase< T >& a ): mData( a.mData ), mSize( a.mSize ) { }

template< class T >
inline T& ArrayBase< T >::operator[]( size_t i )
{
    RBXASSERT_VERY_FAST( i < mSize );
    return mData[ i ];
}

template< class T >
inline const T& ArrayBase< T >::operator[]( size_t i ) const
{
    RBXASSERT_VERY_FAST( i < mSize );
    return mData[ i ];
}

template< class T >
inline T& ArrayBase< T >::at( size_t i )
{
    RBXASSERT_VERY_FAST( i < mSize );
    return mData[ i ];
}

template< class T >
inline const T& ArrayBase< T >::at( size_t i ) const
{
    RBXASSERT_VERY_FAST( i < mSize );
    return mData[ i ];
}

template< class T >
inline size_t ArrayBase< T >::size() const
{
    return mSize;
}

template< class T >
inline T& ArrayBase< T >::front()
{
    RBXASSERT_VERY_FAST( mSize > 0 );
    return mData[ 0 ];
}

template< class T >
inline const T& ArrayBase< T >::front() const
{
    RBXASSERT_VERY_FAST( mSize > 0 );
    return mData[ 0 ];
}

template< class T >
inline T& ArrayBase< T >::back()
{
    RBXASSERT_VERY_FAST( mSize > 0 );
    return mData[ mSize - 1 ];
}

template< class T >
inline const T& ArrayBase< T >::back() const
{
    RBXASSERT_VERY_FAST( mSize > 0 );
    return mData[ mSize - 1 ];
}

template< class T >
inline T* ArrayBase< T >::begin()
{
    return mData;
}

template< class T >
inline const T* ArrayBase< T >::begin() const
{
    return mData;
}

template< class T >
inline const T* ArrayBase< T >::cbegin() const
{
    return mData;
}

template< class T >
inline T* ArrayBase< T >::end()
{
    return mData + mSize;
}

template< class T >
inline const T* ArrayBase< T >::end() const
{
    return mData + mSize;
}

template< class T >
inline const T* ArrayBase< T >::cend() const
{
    return mData + mSize;
}

template< class T >
inline T* ArrayBase< T >::data()
{
    return mData;
}

template< class T >
inline const T* ArrayBase< T >::data() const
{
    return mData;
}

template< class T >
inline const T* ArrayBase< T >::cdata() const
{
    return mData;
}

template< class T >
inline bool ArrayBase< T >::empty() const
{
    return mSize == 0;
}

struct ArrayNoInit { };

//
// ArrayDynamic
//
template< class T >
class ArrayDynamic: public ArrayBase< T >
{
public:
    static const boost::uint32_t defaultAlignment = 16;
    typedef ArrayBase< T > Base;
    inline ArrayDynamic( );
    inline explicit ArrayDynamic( size_t _size, ArrayNoInit, boost::uint32_t _align = defaultAlignment );
    inline explicit ArrayDynamic( size_t _size, boost::uint32_t _align = defaultAlignment );
    inline ArrayDynamic( const ArrayDynamic< T >& a );
    inline ArrayDynamic( const ArrayBase< T >& a );
    inline ~ArrayDynamic();
    inline void clear();
    inline ArrayDynamic< T >& operator=( const ArrayDynamic< T >& _a );
    inline ArrayDynamic< T >& operator=( const ArrayBase< T >& _a );
    inline void reserve( size_t _capacity );
    inline void resize( size_t _size );
    inline size_t capacity() const;
    inline void push_back( const T& _a );
    inline void pop_back( );
    inline void insert( size_t i, const T& val );
    inline T* insert( const T* it, const T& val );
    template< class InputType >
    inline void insert_count( T* it, InputType first, size_t count );
    template< class InputType >
    inline void insert( T* it, InputType first, InputType last );
    void assign( size_t size, const T& value );

private:
    template< class InputType >
    inline void copyConstruct( void* dst, InputType src, size_t count );
    inline void copyConstruct( void* dst, const T* src, size_t count );
    inline void increase_capacity( size_t requestedCapacity );

    size_t mCapacity;
    bool mNoInit;
    boost::uint32_t mAlignment;
};

//
// ArrayDynamic implementation
//

namespace array_dynamic_details
{
    inline void* aligned_alloc(std::size_t alignment, std::size_t size) BOOST_NOEXCEPT
    {
        if (!size) {
            return 0;
        }
        if (alignment < sizeof(void*)) {
            alignment = sizeof(void*);
        }
#ifdef _WIN32
        void* p = _aligned_malloc( size, alignment );
#elif defined( __ANDROID__ )
        void* p = ::memalign( alignment, size );
#else
        void* p;
        if (::posix_memalign(&p, alignment, size) != 0) {
            p = 0;
        }
#endif
        return p;
    }

    inline void aligned_free(void* ptr)
        BOOST_NOEXCEPT
    {
#ifdef _WIN32
        _aligned_free( ptr );
#else
        ::free(ptr);
#endif
    }

    //
    // Construct
    //
    template< class T >
    static void construct( void* dst, size_t count, const boost::true_type& hasTrivialConstructor )
    {
    }

    template< class T >
    static void construct( void* dst, size_t count, const boost::false_type& hasTrivialConstructor )
    {
        for( size_t i = 0; i < count; i++ )
        {
            new( reinterpret_cast< char* >( dst ) + i * sizeof( T ) )T();
        }
    }

    template< class T >
    static void construct( void* dst, size_t count )
    {
        construct< T >( dst, count, boost::has_trivial_constructor< T >() );
    }

    //
    // Copy
    //
    template< class T >
    static void copyTrivial( void* dst, const T* src, size_t count, const boost::false_type& isFundamentalOrPointer )
    {
        memcpy( dst, src, count * sizeof( T ) );
    }

    template< class T >
    static void copyTrivial( void* dst, const T* src, size_t count, const boost::true_type& isFundamentalOrPointer )
    {
        if( count > 16 )
        {
            copyTrivial(dst, src, count, boost::false_type() );
        }
        else
        {
            for( size_t i = 0; i < count; i++ )
            {
                new( reinterpret_cast< char* >( dst ) + i * sizeof( T ) )T( src[ i ] );
            }
        }
    }

    template <bool B>
    struct bool_type : boost::integral_constant<bool, B>
    {
        static const bool value = B;
    };

    template< class T >
    static void copyTrivial( void* dst, const T* src, size_t count )
    {
        copyTrivial( dst, src, count, bool_type< boost::is_fundamental< T >::value || boost::is_pointer< T >::value >() );
    }

    template< class T >
    static void copyConstruct( void* dst, const T* src, size_t count, const boost::true_type& hasTrivialCopyConstruct )
    {
        copyTrivial( dst, src, count );
    }

    template< class T >
    static void copyConstruct( void* dst, const T* src, size_t count, const boost::false_type& hasTrivialCopyConstruct )
    {
        for( size_t i = 0; i < count; i++ )
        {
            new( reinterpret_cast< char* >( dst ) + i * sizeof( T ) )T( src[ i ] );
        }
    }

    template< class T >
    static void copyConstruct( void* dst, const T* src, size_t count )
    {
        copyConstruct( dst, src, count, boost::has_trivial_copy_constructor< T >() );
    }

    //
    // Destroy
    //
    template< class T >
    void destroy( T* src, size_t count, const boost::false_type& hasTrivialDestructor )
    {
        for( size_t i = 0; i < count; i++ )
        {
            src[ i ].~T();
        }
    }

    template< class T >
    void destroy( T* src, size_t count, const boost::true_type& hasTrivialDestructor )
    {
    }

    template< class T >
    void destroy( T* src, size_t count )
    {
        destroy( src, count, boost::has_trivial_destructor< T >() );
    }

    //
    // Shift right
    //
    template< class T >
    static void shiftRightTrivialCopy( T* src, size_t count, size_t offset, const boost::false_type& isFundamentalOrPointer )
    {
        memmove( static_cast< void* >( src + offset ), src, count * sizeof( T ) );
    }

    template< class T >
    static void shiftRightTrivialCopy( T* src, size_t count, size_t offset, const boost::true_type& isFundamentalOrPointer )
    {
        if( count < 16 )
        {
            for( size_t i = 0; i < count; i++ )
            {
                new( (T*)src + count + offset - 1 - i ) T( src[ count - 1 - i ] );
            }
        }
        else
        {
            shiftRightTrivialCopy( src, count, offset, boost::false_type() );
        }
    }

    template< class T >
    static void shiftRightTrivialCopy( T* src, size_t count, size_t offset )
    {
        shiftRightTrivialCopy( src, count, offset, boost::integral_constant< bool, boost::is_fundamental< T >::value || boost::is_pointer< T >::value >() );
    }

    template< class T >
    static void shiftRight( T* src, size_t count, size_t offset, const boost::true_type& hasTrivialCopy )
    {
        shiftRightTrivialCopy( src, count, offset, boost::integral_constant< bool, boost::is_fundamental< T >::value || boost::is_pointer< T >::value >() );
    }

    template< class T >
    static void shiftRightNonTrivialOverlapping( T* src, size_t count, size_t offset, const boost::false_type& hasTrivialDestructor )
    {
        for( size_t i = 0; i < count; i++ )
        {
            new( (T*)src + count + offset - 1 - i ) T( src[ count - 1 - i ] );
            src[ count - 1 - i ].~T();
        }
    }

    template< class T >
    static void shiftRightNonTrivialOverlapping( T* src, size_t count, size_t offset, const boost::true_type& hasTrivialDestructor )
    {
        for( size_t i = 0; i < count; i++ )
        {
            new( (T*)src + count + offset - 1 - i ) T( src[ count - 1 - i ] );
        }
    }

    template< class T >
    static void shiftRight( T* src, size_t count, size_t offset, const boost::false_type& hasTrivialCopy )
    {
        shiftRightNonTrivialOverlapping( src, count, offset, boost::has_trivial_destructor< T >() );
    }

    template< class T >
    static void shiftRight( T* src, size_t count, size_t offset )
    {
        shiftRight( src, count, offset, boost::has_trivial_copy< T >() );
    }
}

template< class T >
template< class InputType >
void ArrayDynamic<T>::copyConstruct( void* dst, InputType src, size_t count )
{
    for( size_t i = 0; i < count; i++ )
    {
        new( reinterpret_cast< char* >( dst ) + i * sizeof( T ) )T( *src );
        src++;
    }
}

template< class T >
void ArrayDynamic<T>::copyConstruct( void* dst, const T* src, size_t count )
{
    if( mNoInit )
    {
        array_dynamic_details::copyTrivial( dst, src, count );
    }
    else
    {
        array_dynamic_details::copyConstruct( dst, src, count );
    }
}

template< class T >
ArrayDynamic<T>::ArrayDynamic( ): ArrayBase< T >( NULL, 0 ), mAlignment( 16 ), mNoInit( false ), mCapacity( 0 ) { }

template< class T >
ArrayDynamic<T>::ArrayDynamic( size_t _size, boost::uint32_t _align ): ArrayBase< T >( NULL, 0 ), mAlignment( _align ), mNoInit( false ), mCapacity( 0 )
{
    reserve( _size );
    Base::mSize = _size;

    // Initialize
    array_dynamic_details::construct< T >( Base::mData, Base::mSize );
}

template< class T >
ArrayDynamic<T>::ArrayDynamic( size_t _size, ArrayNoInit, boost::uint32_t _align ): ArrayBase< T >( NULL, 0 ), mAlignment( _align ), mNoInit( true ), mCapacity( 0 )
{
    reserve( _size );
    Base::mSize = _size;
}

template< class T >
ArrayDynamic<T>::ArrayDynamic( const ArrayDynamic< T >& a ): ArrayBase< T >( NULL, 0 ), mAlignment( a.mAlignment ), mNoInit( a.mNoInit ), mCapacity( 0 )
{
    reserve( a.size() );
    copyConstruct( Base::mData, a.data(), a.size() );
    Base::mSize = a.size();
}

template< class T >
ArrayDynamic<T>::ArrayDynamic( const ArrayBase< T >& a ): ArrayBase< T >( NULL, 0 ), mAlignment( defaultAlignment ), mNoInit( false ), mCapacity( 0 )
{
    reserve( a.size() );
    copyConstruct( Base::mData, a.data(), a.size() );
    Base::mSize = a.size();
}

template< class T >
ArrayDynamic<T>::~ArrayDynamic()
{
    clear();
    if( mCapacity > 0 )
    {
        array_dynamic_details::aligned_free( Base::mData );
        mCapacity = 0;
        Base::mData = NULL;
    }
}

template< class T >
void ArrayDynamic<T>::clear()
{
    if( !mNoInit )
    {
        array_dynamic_details::destroy( Base::mData, Base::mSize );
    }
    Base::mSize = 0;
}

template< class T >
ArrayDynamic< T >& ArrayDynamic<T>::operator=( const ArrayDynamic< T >& _a )
{
    clear();
    if( mCapacity > 0 && mAlignment != _a.mAlignment )
    {
        array_dynamic_details::aligned_free( Base::mData );
        mCapacity = 0;
        Base::mData = NULL;
    }
    mNoInit = _a.mNoInit;
    mAlignment = _a.mAlignment;
    reserve( _a.size() );
    copyConstruct( Base::mData, _a.data(), _a.size() );
    Base::mSize = _a.size();
    return *this;
}

template< class T >
ArrayDynamic< T >& ArrayDynamic<T>::operator=( const ArrayBase< T >& _a )
{
    clear();
    reserve( _a.size() );
    copyConstruct( Base::mData, _a.data(), _a.size() );
    Base::mSize = _a.size();
    return *this;
}

template< class T >
void ArrayDynamic<T>::reserve( size_t _capacity )
{
    if( _capacity > mCapacity )
    {
        void* newData = array_dynamic_details::aligned_alloc( mAlignment, _capacity * sizeof( T ) );
        if( mNoInit )
        {
            array_dynamic_details::copyTrivial( newData, Base::mData, Base::mSize );
        }
        else
        {
            array_dynamic_details::copyConstruct( newData, Base::mData, Base::mSize );
            array_dynamic_details::destroy( Base::mData, Base::mSize );
        }

        if( mCapacity > 0 )
        {
            array_dynamic_details::aligned_free( Base::mData );
        }
        Base::mData = ( T* )newData;
        mCapacity = _capacity;
    }
}

template< class T >
void ArrayDynamic<T>::resize( size_t _size )
{
    if( _size <= Base::mSize )
    {
        if( !mNoInit )
        {
            array_dynamic_details::destroy( Base::mData + _size, Base::mSize - _size );
        }
        Base::mSize = _size;
        return;
    }

    if( _size > mCapacity )
    {
        reserve( _size );
    }

    if( !mNoInit )
    {
        array_dynamic_details::construct< T >( Base::mData + Base::mSize, _size - Base::mSize );
    }
    Base::mSize = _size;
}

template< class T >
inline size_t ArrayDynamic<T>::capacity() const
{
    return mCapacity;
}

template< class T >
inline void ArrayDynamic<T>::increase_capacity( size_t requestedCapacity )
{
    size_t newCapacity = mCapacity == 0 ? 2 : 2 * mCapacity;
    while (newCapacity < requestedCapacity) newCapacity *= 2;
    reserve( newCapacity );
}

template< class T >
inline void ArrayDynamic<T>::push_back( const T& _a )
{
    if( mCapacity == Base::mSize )
    {
        increase_capacity( mCapacity + 1 );
    }
    new( Base::data() + Base::mSize )T( _a );
    Base::mSize++;
}

template< class T >
inline void ArrayDynamic<T>::pop_back( )
{
    RBXASSERT_VERY_FAST( Base::mSize > 0 );
    Base::mSize--;
    if( mNoInit )
    {
        ( Base::data()+Base::mSize )->~T();
    }
}

template< class T >
void ArrayDynamic<T>::insert( size_t i, const T& val )
{
    RBXASSERT_VERY_FAST( i <= Base::mSize );
    if( i == Base::mSize )
    {
        push_back( val );
        return;
    }
    if( mCapacity == Base::mSize )
    {
        increase_capacity( mCapacity + 1 );
    }
    if( mNoInit )
    {
        array_dynamic_details::shiftRightTrivialCopy( Base::data() + i, Base::mSize - i, 1 );
    }
    else
    {
        array_dynamic_details::shiftRight( Base::data() + i, Base::mSize - i, 1 );
    }
    Base::mSize++;
    new( Base::data() + i )T( val );
}

template< class T >
inline T* ArrayDynamic<T>::insert( const T* it, const T& val )
{
    RBXASSERT_VERY_FAST( Base::begin() <= it );
    RBXASSERT_VERY_FAST( Base::end() >= it );
    size_t index = it - Base::begin();
    insert( index, val );
    return Base::begin() + index;
}

template< class T >
template< class InputType >
void ArrayDynamic<T>::insert_count( T* it, InputType first, size_t count )
{
    RBXASSERT_VERY_FAST( Base::begin() <= it );
    RBXASSERT_VERY_FAST( Base::end() >= it );
    size_t index = it - Base::begin();
    if( mCapacity < Base::mSize + count )
    {
        increase_capacity( Base::mSize + count );
    }
    it = Base::begin() + index;
    if( it < Base::end() )
    {
        if( mNoInit )
        {
            array_dynamic_details::shiftRightTrivialCopy( it, Base::end() - it, count );
        }
        else
        {
            array_dynamic_details::shiftRight( it, Base::end() - it, count );
        }
    }
    copyConstruct( it, first, count );
    Base::mSize += count;
}

template< class T >
template< class InputType >
inline void ArrayDynamic<T>::insert( T* it, InputType first, InputType last )
{
    size_t count = last - first;
    insert_count( it, first, count );
}

template< class T >
void ArrayDynamic<T>::assign( size_t size, const T& value )
{
    clear();
    reserve( size );
    T* it = Base::begin();
    for( size_t i = 0; i < size; i++ )
    {
        new( it + i )T( value );
    }
    Base::mSize = size;
}

//
// ArrayRef
//
template< class T >
class ArrayRef : public ArrayBase< T >
{
public:
    typedef ArrayBase< T > Base;

    inline ArrayRef( T* _data, size_t _size );
    inline ArrayRef( const ArrayRef< T >& a );
    inline ArrayRef( const ArrayBase< T >& a );
    inline ArrayRef< T >& operator=( const ArrayBase< T >& src );
};

//
// ArrayRef Implementation
//
template< class T >
ArrayRef< T >::ArrayRef( T* _data, size_t _size ): Base( _data, _size ) { }

template< class T >
ArrayRef< T >::ArrayRef( const ArrayRef< T >& a ): Base( a ) { }

template< class T >
ArrayRef< T >::ArrayRef( const ArrayBase< T >& a ): Base( a ) { }

template< class T >
ArrayRef< T >& ArrayRef< T >::operator=( const ArrayBase< T >& src )
{
    Base::mData = src.mData;
    Base::mSize = src.mSize;
    return *this;
}

}
