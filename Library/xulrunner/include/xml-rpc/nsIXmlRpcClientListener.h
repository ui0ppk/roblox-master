/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/extensions/xml-rpc/idl/nsIXmlRpcClientListener.idl
 */

#ifndef __gen_nsIXmlRpcClientListener_h__
#define __gen_nsIXmlRpcClientListener_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIXmlRpcClient; /* forward declaration */

class nsIXmlRpcFault; /* forward declaration */


/* starting interface:    nsIXmlRpcClientListener */
#define NS_IXMLRPCCLIENTLISTENER_IID_STR "27e60cd8-6d63-4d87-b7d1-82c09e0c7363"

#define NS_IXMLRPCCLIENTLISTENER_IID \
  {0x27e60cd8, 0x6d63, 0x4d87, \
    { 0xb7, 0xd1, 0x82, 0xc0, 0x9e, 0x0c, 0x73, 0x63 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIXmlRpcClientListener : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXMLRPCCLIENTLISTENER_IID)

  /**
     * Called when XML-RPC call has succeeded.
     *
     * @param client    The originating XML-RPC client.
     * @param context   The context passed in to the asyncCall method.
     * @param result    The result of the XML-RPC call.
     */
  /* void onResult (in nsIXmlRpcClient client, in nsISupports ctxt, in nsISupports result); */
  NS_SCRIPTABLE NS_IMETHOD OnResult(nsIXmlRpcClient *client, nsISupports *ctxt, nsISupports *result) = 0;

  /**
     * Called when the XML-RPC server returned a Fault
     *
     * @param client    The originating XML-RPC client.
     * @param context   The context passed in to the asyncCall method.
     * @param fault     The XML-RPC fault as returned by the server.
     */
  /* void onFault (in nsIXmlRpcClient client, in nsISupports ctxt, in nsIXmlRpcFault fault); */
  NS_SCRIPTABLE NS_IMETHOD OnFault(nsIXmlRpcClient *client, nsISupports *ctxt, nsIXmlRpcFault *fault) = 0;

  /**
     * Called when a transport or other error occurs.
     *
     * @param client    The originating XML-RPC client.
     * @param context   The context passed in to the asyncCall method.
     * @param status    The status code of the error.
     * @param errorMsg  A human readable error message.
     */
  /* void onError (in nsIXmlRpcClient client, in nsISupports ctxt, in nsresult status, in wstring errorMsg); */
  NS_SCRIPTABLE NS_IMETHOD OnError(nsIXmlRpcClient *client, nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIXmlRpcClientListener, NS_IXMLRPCCLIENTLISTENER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIXMLRPCCLIENTLISTENER \
  NS_SCRIPTABLE NS_IMETHOD OnResult(nsIXmlRpcClient *client, nsISupports *ctxt, nsISupports *result); \
  NS_SCRIPTABLE NS_IMETHOD OnFault(nsIXmlRpcClient *client, nsISupports *ctxt, nsIXmlRpcFault *fault); \
  NS_SCRIPTABLE NS_IMETHOD OnError(nsIXmlRpcClient *client, nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIXMLRPCCLIENTLISTENER(_to) \
  NS_SCRIPTABLE NS_IMETHOD OnResult(nsIXmlRpcClient *client, nsISupports *ctxt, nsISupports *result) { return _to OnResult(client, ctxt, result); } \
  NS_SCRIPTABLE NS_IMETHOD OnFault(nsIXmlRpcClient *client, nsISupports *ctxt, nsIXmlRpcFault *fault) { return _to OnFault(client, ctxt, fault); } \
  NS_SCRIPTABLE NS_IMETHOD OnError(nsIXmlRpcClient *client, nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg) { return _to OnError(client, ctxt, status, errorMsg); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIXMLRPCCLIENTLISTENER(_to) \
  NS_SCRIPTABLE NS_IMETHOD OnResult(nsIXmlRpcClient *client, nsISupports *ctxt, nsISupports *result) { return !_to ? NS_ERROR_NULL_POINTER : _to->OnResult(client, ctxt, result); } \
  NS_SCRIPTABLE NS_IMETHOD OnFault(nsIXmlRpcClient *client, nsISupports *ctxt, nsIXmlRpcFault *fault) { return !_to ? NS_ERROR_NULL_POINTER : _to->OnFault(client, ctxt, fault); } \
  NS_SCRIPTABLE NS_IMETHOD OnError(nsIXmlRpcClient *client, nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg) { return !_to ? NS_ERROR_NULL_POINTER : _to->OnError(client, ctxt, status, errorMsg); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsXmlRpcClientListener : public nsIXmlRpcClientListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXMLRPCCLIENTLISTENER

  nsXmlRpcClientListener();

private:
  ~nsXmlRpcClientListener();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsXmlRpcClientListener, nsIXmlRpcClientListener)

nsXmlRpcClientListener::nsXmlRpcClientListener()
{
  /* member initializers and constructor code */
}

nsXmlRpcClientListener::~nsXmlRpcClientListener()
{
  /* destructor code */
}

/* void onResult (in nsIXmlRpcClient client, in nsISupports ctxt, in nsISupports result); */
NS_IMETHODIMP nsXmlRpcClientListener::OnResult(nsIXmlRpcClient *client, nsISupports *ctxt, nsISupports *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onFault (in nsIXmlRpcClient client, in nsISupports ctxt, in nsIXmlRpcFault fault); */
NS_IMETHODIMP nsXmlRpcClientListener::OnFault(nsIXmlRpcClient *client, nsISupports *ctxt, nsIXmlRpcFault *fault)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onError (in nsIXmlRpcClient client, in nsISupports ctxt, in nsresult status, in wstring errorMsg); */
NS_IMETHODIMP nsXmlRpcClientListener::OnError(nsIXmlRpcClient *client, nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIXmlRpcClientListener_h__ */
