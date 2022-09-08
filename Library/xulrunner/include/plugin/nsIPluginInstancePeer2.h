/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/modules/plugin/base/public/nsIPluginInstancePeer2.idl
 */

#ifndef __gen_nsIPluginInstancePeer2_h__
#define __gen_nsIPluginInstancePeer2_h__


#ifndef __gen_nsIPluginInstancePeer_h__
#include "nsIPluginInstancePeer.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
struct JSObject;
struct JSContext;

/* starting interface:    nsIPluginInstancePeer2 */
#define NS_IPLUGININSTANCEPEER2_IID_STR "e7d48c00-e1f1-11d2-8360-fbc8abc4ae7c"

#define NS_IPLUGININSTANCEPEER2_IID \
  {0xe7d48c00, 0xe1f1, 0x11d2, \
    { 0x83, 0x60, 0xfb, 0xc8, 0xab, 0xc4, 0xae, 0x7c }}

/**
 * The nsIPluginInstancePeer2 interface extends the nsIPluginInstancePeer
 * interface, providing access to functionality provided by newer browsers.
 * All functionality in nsIPluginInstancePeer can be mapped to the 4.X
 * plugin API.
 */
class NS_NO_VTABLE nsIPluginInstancePeer2 : public nsIPluginInstancePeer {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGININSTANCEPEER2_IID)

  /**
     * Get the JavaScript window object corresponding to this plugin instance.
     *
   * @param aJSWindow - the resulting JavaScript window object
   * @result          - NS_OK if this operation was successful
     */
  /* readonly attribute JSObjectPtr JSWindow; */
  NS_IMETHOD GetJSWindow(JSObject * *aJSWindow) = 0;

  /**
	 * Get the JavaScript execution thread corresponding to this plugin instance.
	 *
   * @param aJSThread - the resulting JavaScript thread id
   * @result            - NS_OK if this operation was successful
	 */
  /* readonly attribute unsigned long JSThread; */
  NS_IMETHOD GetJSThread(PRUint32 *aJSThread) = 0;

  /**
	 * Get the JavaScript context to this plugin instance.
	 *
   * @param aJSContext - the resulting JavaScript context
   * @result           - NS_OK if this operation was successful
	 */
  /* readonly attribute JSContextPtr JSContext; */
  NS_IMETHOD GetJSContext(JSContext * *aJSContext) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginInstancePeer2, NS_IPLUGININSTANCEPEER2_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPLUGININSTANCEPEER2 \
  NS_IMETHOD GetJSWindow(JSObject * *aJSWindow); \
  NS_IMETHOD GetJSThread(PRUint32 *aJSThread); \
  NS_IMETHOD GetJSContext(JSContext * *aJSContext); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPLUGININSTANCEPEER2(_to) \
  NS_IMETHOD GetJSWindow(JSObject * *aJSWindow) { return _to GetJSWindow(aJSWindow); } \
  NS_IMETHOD GetJSThread(PRUint32 *aJSThread) { return _to GetJSThread(aJSThread); } \
  NS_IMETHOD GetJSContext(JSContext * *aJSContext) { return _to GetJSContext(aJSContext); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPLUGININSTANCEPEER2(_to) \
  NS_IMETHOD GetJSWindow(JSObject * *aJSWindow) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetJSWindow(aJSWindow); } \
  NS_IMETHOD GetJSThread(PRUint32 *aJSThread) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetJSThread(aJSThread); } \
  NS_IMETHOD GetJSContext(JSContext * *aJSContext) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetJSContext(aJSContext); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsPluginInstancePeer2 : public nsIPluginInstancePeer2
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCEPEER2

  nsPluginInstancePeer2();

private:
  ~nsPluginInstancePeer2();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPluginInstancePeer2, nsIPluginInstancePeer2)

nsPluginInstancePeer2::nsPluginInstancePeer2()
{
  /* member initializers and constructor code */
}

nsPluginInstancePeer2::~nsPluginInstancePeer2()
{
  /* destructor code */
}

/* readonly attribute JSObjectPtr JSWindow; */
NS_IMETHODIMP nsPluginInstancePeer2::GetJSWindow(JSObject * *aJSWindow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long JSThread; */
NS_IMETHODIMP nsPluginInstancePeer2::GetJSThread(PRUint32 *aJSThread)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute JSContextPtr JSContext; */
NS_IMETHODIMP nsPluginInstancePeer2::GetJSContext(JSContext * *aJSContext)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIPluginInstancePeer2_h__ */
