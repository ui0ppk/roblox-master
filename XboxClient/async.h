#pragma once

//
// Many xbox functions return an IAsyncOperation<T> and leave you to deal with it.
// This dirty little helper can take away some of the pain.
//
// Example: suppose you want to call this monstrosity:  IAsyncOperation<ISomeResult^>^ SomeXboxClass::SomeAsyncMethod();
//
//  Use case #1: get/subscribe/wait
//
//  async( var->SomeXBoxMethod() ).complete(         // - install the 'completed' event handler
//     [<captures>]( ISomeResult^ result )
//     { 
//         <your-completed-handler-goes-here>;
//     }
//  ).except( [<captures>]( Platform::Exception^ e )             // - install the exception handler. 
//     {                                                         // if you don't do that, join() will rethrow the exception on the calling thread
//         <your-exception-handler-goes-here>;
//     }
//  ).join();                                                    // - wait until it finishes
//
//  Use case #2: several parallel operations:
//
//  auto& a1 = async( var->SomeXBoxMethod() ).complete( <same handler> ).except( .... ); // do not call join yet
//  auto& a2 = async( var2->SomeOtherMethod() ).complete( <handler> ).except( .... ); // neither yet
//  <do stuff>;
//  a1.join(); a2.join();
//
//  Use case #3: don't care to wait
//  async( var->SomeXBoxMethod() ).complete( <same handler> ).except( .... ).detach(); // will return immediately without waiting
//
//  You MUST call either .join() or .detach() on *every* async instance you spawn, otherwise this will leak the instances.
//  g_asyncLeaked contains the number of async instances still alive.
//


#include <wrl.h>
#include <functional>
#include <atomic>

namespace AsyncDetail
{

extern int g_asyncLeaked;

using Windows::Foundation::IAsyncAction;
using Windows::Foundation::IAsyncOperation;
using Windows::Foundation::AsyncStatus;


// abstracts different Async Types, 
template< class Op > struct subscriber; // not implemented

template< class Ty > struct subscriber< IAsyncOperation<Ty> >
{
    typedef Ty result_type;
    typedef IAsyncOperation<Ty> op_type;

    static void subscribe( op_type^ op, std::function< void(op_type^, AsyncStatus) > fn )
    {
        op->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<result_type>( fn );
    }

    template< class Fn >
    static void call_handler( op_type^ op, Fn& fn ) { fn( op->GetResults() ); }
};

template<> struct subscriber<IAsyncAction>
{
    typedef IAsyncAction op_type;

    static void subscribe( op_type^ op, std::function< void(op_type^, AsyncStatus) > fn )
    {
        op->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler( fn );
    }

    template< class Fn >
    static void call_handler( op_type^ op, Fn& fn ) { op->GetResults(); fn(); }
};


template< class Ty, class Op >
class async_caller
{
    typedef async_caller<Ty, Op> myt;
    typedef std::function<void(Ty)>  func_success;
    typedef std::function<void()> func_error;
    typedef Op opera;
    typedef Windows::Foundation::IAsyncAction action;
    typedef Platform::Exception exception;
    typedef std::function<void(exception^ e)> excfunc;

    func_success m_fn_success;
    func_error   m_fn_error;
    func_error   m_fn_cancelled;
    opera^ m_op;
    exception ^m_xcpt;
    excfunc m_excfn;
    std::atomic<int> m_done;

public:
    static myt& create( opera^ op ) { return *new myt(op); }

    myt& complete( func_success fn ) 
    { 
        m_fn_success = fn;
        return *this;
    }

    myt& error( func_error fn )
    {
        m_fn_error = fn;
        return * this;
    }

    myt& cancelled( func_error fn )
    {
        m_fn_cancelled = fn;
        return *this;
    }

    myt& except(excfunc fn)
    {
        m_excfn = fn;
        return *this;
    }

    void join()
    {
        install_handlers();

        while ( m_op->Status == Windows::Foundation::AsyncStatus::Started || !m_done.load() )
        {
            ::Sleep(1);
        }
        if( m_xcpt )
        {
            if( m_excfn ) 
                m_excfn(m_xcpt); 
            else 
            {
                delete this; 
                throw m_xcpt; // exception translation: if there was an exception on the async thread, this will make sure the exception propagates to the calling thread.
            }
        }
        delete this;
    }

    void detach()
    {
        install_handlers();

        if( m_excfn ) // nope!
            RBXASSERT( !"Do not install exception handlers if you're calling detach(). There is no meaningful exception translation possible in this case." );

        int expect = 0;
        if( !m_done.compare_exchange_strong( expect, 1 ) ) // are we done yet? 
        {
            delete this; // kill self, otherwise the handler will do that from its own context
        }
    }

private:
    async_caller( opera^ op ): m_op(op), m_done(0) { g_asyncLeaked++; }
    ~async_caller() { g_asyncLeaked--; }
    async_caller(); // = delete;  // use  auto& a  instead.
    async_caller( const myt& ); // = delete;
    void operator=(const myt&); // = delete;

    void install_handlers()
    {
        subscriber<Op>::subscribe( m_op,  
            [this](opera^ operation, AsyncStatus status) -> void
            {
                try
                {
                    switch (status)
                    {
                    case Windows::Foundation::AsyncStatus::Canceled:
                        if( m_fn_cancelled ) 
                            m_fn_cancelled();
                        break;
                    case Windows::Foundation::AsyncStatus::Completed:
                        if( m_fn_success ) 
                            subscriber<Op>::call_handler( m_op, m_fn_success );
                        break;
                    case Windows::Foundation::AsyncStatus::Error:
                        if( m_fn_error ) 
                            m_fn_error();
                        break;
                    default:
                        RBXASSERT(!"should not happen");
                        break;
                    }
                }
                catch( exception^ e )
                {
                    m_xcpt = e;
                }

                int expect = 0;
                if( !m_done.compare_exchange_strong( expect, 1 ) )
                {
                    delete this; // detach() has been already called, delete self and exit
                }
            }   
        );
    }
};

} // ns AsyncDetail

template <class Ty>
inline AsyncDetail::async_caller<Ty, Windows::Foundation::IAsyncOperation<Ty> >& async( Windows::Foundation::IAsyncOperation<Ty>^ op )
{
    return AsyncDetail::async_caller<Ty, Windows::Foundation::IAsyncOperation<Ty> >::create(op);
}

inline AsyncDetail::async_caller<void, Windows::Foundation::IAsyncAction>& async( Windows::Foundation::IAsyncAction^ op )
{
    return AsyncDetail::async_caller<void, Windows::Foundation::IAsyncAction>::create(op);
}
