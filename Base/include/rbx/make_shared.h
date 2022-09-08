#pragma once
#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <cstddef>
#include <new>

// This is a copy of make_shared from boost 1.42.1
// When we upgrade we can use boost directly
// NOTE: 1.38.1 has an undocumented make_shared.hpp, but I don't know if it is 
//       safe or not to use.
namespace rbx
{
namespace detail
{
template< std::size_t N, std::size_t A > struct sp_aligned_storage
{
    union type
    {
        char data_[ N ];
        typename boost::type_with_alignment< A >::type align_;
    };
};

template< class T > class sp_ms_deleter
{
private:

    typedef typename sp_aligned_storage< sizeof( T ), ::boost::alignment_of< T >::value >::type storage_type;

    bool initialized_;
    storage_type storage_;

private:

    void destroy()
    {
        if( initialized_ )
        {
            reinterpret_cast< T* >( storage_.data_ )->~T();
            initialized_ = false;
        }
    }

public:

    sp_ms_deleter(): initialized_( false )
    {
    }

    // optimization: do not copy storage_
    sp_ms_deleter( sp_ms_deleter const & ): initialized_( false )
    {
    }

    ~sp_ms_deleter()
    {
        destroy();
    }

    void operator()( T * )
    {
        destroy();
    }

    void * address()
    {
        return storage_.data_;
    }

    void set_initialized()
    {
        initialized_ = true;
    }
};

#if defined( BOOST_HAS_RVALUE_REFS )
template< class T > T&& sp_forward( T & t )
{
    return static_cast< T&& >( t );
}
#endif
} // namespace detail

// TODO: This implementation may not support shared_from_this properly. Upgrade to new boost, which implements this for us
template< class T > boost::shared_ptr< T > make_shared()
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), rbx::detail::sp_ms_deleter< T >() );

    rbx::detail::sp_ms_deleter< T > * pd = boost::get_deleter< rbx::detail::sp_ms_deleter< T > >( pt );

    void * pv = pd->address();

    ::new( pv ) T();
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T> boost::shared_ptr< T > make_shared(std::allocator<T> a)
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), rbx::detail::sp_ms_deleter< T >(), a );

    rbx::detail::sp_ms_deleter< T > * pd = boost::get_deleter< rbx::detail::sp_ms_deleter< T > >( pt );

    void * pv = pd->address();

    ::new( pv ) T();
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    return boost::shared_ptr< T >( pt, pt2 );
}


template< class T, class A1 >
boost::shared_ptr< T > make_shared( A1 const & a1)
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), rbx::detail::sp_ms_deleter< T >() );

    rbx::detail::sp_ms_deleter< T > * pd = boost::get_deleter< rbx::detail::sp_ms_deleter< T > >( pt );

    void * pv = pd->address();

    ::new( pv ) T( a1 );
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2 >
boost::shared_ptr< T > make_shared( A1 const & a1, A2 const & a2 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), rbx::detail::sp_ms_deleter< T >() );

    rbx::detail::sp_ms_deleter< T > * pd = boost::get_deleter< rbx::detail::sp_ms_deleter< T > >( pt );

    void * pv = pd->address();

    ::new( pv ) T( a1, a2 );
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    return boost::shared_ptr< T >( pt, pt2 );
}

template< class T, class A1, class A2, class A3 >
boost::shared_ptr< T > make_shared( A1 const & a1, A2 const & a2, A3 const & a3 )
{
    boost::shared_ptr< T > pt( static_cast< T* >( 0 ), rbx::detail::sp_ms_deleter< T >() );

    rbx::detail::sp_ms_deleter< T > * pd = boost::get_deleter< rbx::detail::sp_ms_deleter< T > >( pt );

    void * pv = pd->address();

    ::new( pv ) T( a1, a2, a3 );
    pd->set_initialized();

    T * pt2 = static_cast< T* >( pv );

    return boost::shared_ptr< T >( pt, pt2 );
}

}
