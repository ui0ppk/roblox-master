/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/xpcom/threads/nsIThreadPool.idl
 */

#ifndef __gen_nsIThreadPool_h__
#define __gen_nsIThreadPool_h__


#ifndef __gen_nsIEventTarget_h__
#include "nsIEventTarget.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIThreadPool */
#define NS_ITHREADPOOL_IID_STR "394c29f0-225f-487f-86d3-4c259da76cab"

#define NS_ITHREADPOOL_IID \
  {0x394c29f0, 0x225f, 0x487f, \
    { 0x86, 0xd3, 0x4c, 0x25, 0x9d, 0xa7, 0x6c, 0xab }}

/**
 * An interface to a thread pool.  A thread pool creates a limited number of
 * anonymous (unnamed) worker threads.  An event dispatched to the thread pool
 * will be run on the next available worker thread.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIThreadPool : public nsIEventTarget {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITHREADPOOL_IID)

  /**
   * Shutdown the thread pool.  This method may not be executed from any thread
   * in the thread pool.  Instead, it is meant to be executed from another
   * thread (usually the thread that created this thread pool).  When this
   * function returns, the thread pool and all of its threads will be shutdown,
   * and it will no longer be possible to dispatch tasks to the thread pool.
   */
  /* void shutdown (); */
  NS_SCRIPTABLE NS_IMETHOD Shutdown(void) = 0;

  /**
   * Get/set the maximum number of threads allowed at one time in this pool.
   */
  /* attribute unsigned long threadLimit; */
  NS_SCRIPTABLE NS_IMETHOD GetThreadLimit(PRUint32 *aThreadLimit) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetThreadLimit(PRUint32 aThreadLimit) = 0;

  /**
   * Get/set the maximum number of idle threads kept alive.
   */
  /* attribute unsigned long idleThreadLimit; */
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadLimit(PRUint32 *aIdleThreadLimit) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadLimit(PRUint32 aIdleThreadLimit) = 0;

  /**
   * Get/set the amount of time in milliseconds before an idle thread is
   * destroyed.
   */
  /* attribute unsigned long idleThreadTimeout; */
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadTimeout(PRUint32 *aIdleThreadTimeout) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadTimeout(PRUint32 aIdleThreadTimeout) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIThreadPool, NS_ITHREADPOOL_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSITHREADPOOL \
  NS_SCRIPTABLE NS_IMETHOD Shutdown(void); \
  NS_SCRIPTABLE NS_IMETHOD GetThreadLimit(PRUint32 *aThreadLimit); \
  NS_SCRIPTABLE NS_IMETHOD SetThreadLimit(PRUint32 aThreadLimit); \
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadLimit(PRUint32 *aIdleThreadLimit); \
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadLimit(PRUint32 aIdleThreadLimit); \
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadTimeout(PRUint32 *aIdleThreadTimeout); \
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadTimeout(PRUint32 aIdleThreadTimeout); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSITHREADPOOL(_to) \
  NS_SCRIPTABLE NS_IMETHOD Shutdown(void) { return _to Shutdown(); } \
  NS_SCRIPTABLE NS_IMETHOD GetThreadLimit(PRUint32 *aThreadLimit) { return _to GetThreadLimit(aThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD SetThreadLimit(PRUint32 aThreadLimit) { return _to SetThreadLimit(aThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadLimit(PRUint32 *aIdleThreadLimit) { return _to GetIdleThreadLimit(aIdleThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadLimit(PRUint32 aIdleThreadLimit) { return _to SetIdleThreadLimit(aIdleThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadTimeout(PRUint32 *aIdleThreadTimeout) { return _to GetIdleThreadTimeout(aIdleThreadTimeout); } \
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadTimeout(PRUint32 aIdleThreadTimeout) { return _to SetIdleThreadTimeout(aIdleThreadTimeout); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSITHREADPOOL(_to) \
  NS_SCRIPTABLE NS_IMETHOD Shutdown(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Shutdown(); } \
  NS_SCRIPTABLE NS_IMETHOD GetThreadLimit(PRUint32 *aThreadLimit) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetThreadLimit(aThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD SetThreadLimit(PRUint32 aThreadLimit) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetThreadLimit(aThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadLimit(PRUint32 *aIdleThreadLimit) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIdleThreadLimit(aIdleThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadLimit(PRUint32 aIdleThreadLimit) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetIdleThreadLimit(aIdleThreadLimit); } \
  NS_SCRIPTABLE NS_IMETHOD GetIdleThreadTimeout(PRUint32 *aIdleThreadTimeout) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIdleThreadTimeout(aIdleThreadTimeout); } \
  NS_SCRIPTABLE NS_IMETHOD SetIdleThreadTimeout(PRUint32 aIdleThreadTimeout) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetIdleThreadTimeout(aIdleThreadTimeout); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsThreadPool : public nsIThreadPool
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITHREADPOOL

  nsThreadPool();

private:
  ~nsThreadPool();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsThreadPool, nsIThreadPool)

nsThreadPool::nsThreadPool()
{
  /* member initializers and constructor code */
}

nsThreadPool::~nsThreadPool()
{
  /* destructor code */
}

/* void shutdown (); */
NS_IMETHODIMP nsThreadPool::Shutdown()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned long threadLimit; */
NS_IMETHODIMP nsThreadPool::GetThreadLimit(PRUint32 *aThreadLimit)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsThreadPool::SetThreadLimit(PRUint32 aThreadLimit)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned long idleThreadLimit; */
NS_IMETHODIMP nsThreadPool::GetIdleThreadLimit(PRUint32 *aIdleThreadLimit)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsThreadPool::SetIdleThreadLimit(PRUint32 aIdleThreadLimit)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned long idleThreadTimeout; */
NS_IMETHODIMP nsThreadPool::GetIdleThreadTimeout(PRUint32 *aIdleThreadTimeout)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsThreadPool::SetIdleThreadTimeout(PRUint32 aIdleThreadTimeout)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIThreadPool_h__ */
