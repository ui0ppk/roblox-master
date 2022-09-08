/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/extensions/xml-rpc/idl/nsIXmlRpcClient.idl
 */

#ifndef __gen_nsIXmlRpcClient_h__
#define __gen_nsIXmlRpcClient_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsIURL_h__
#include "nsIURL.h"
#endif

#ifndef __gen_nsIXmlRpcClientListener_h__
#include "nsIXmlRpcClientListener.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIXmlRpcFault; /* forward declaration */


/* starting interface:    nsIXmlRpcClient */
#define NS_IXMLRPCCLIENT_IID_STR "4d7d15c0-3747-4f7f-b6b3-792a5ea1a9aa"

#define NS_IXMLRPCCLIENT_IID \
  {0x4d7d15c0, 0x3747, 0x4f7f, \
    { 0xb6, 0xb3, 0x79, 0x2a, 0x5e, 0xa1, 0xa9, 0xaa }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIXmlRpcClient : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXMLRPCCLIENT_IID)

  /**
 * Simple XML-RPC client interface.
 */
/**
     * Set server URL. Call this before using this object.
     *
     * @param serverURL URL of server side object on which methods should
     *                  be called.
     */
  /* void init (in string serverURL); */
  NS_SCRIPTABLE NS_IMETHOD Init(const char *serverURL) = 0;

  /**
     * Set authentication info if needed.  Both parameters must be
     * specified for authentication to be enabled.
     *
     * @param username  username to be used if asked to authenticate
     * @param password  password to be used if asked to authenticate
     */
  /* void setAuthentication (in string username, in string password); */
  NS_SCRIPTABLE NS_IMETHOD SetAuthentication(const char *username, const char *password) = 0;

  /**
     * Clear authentication info.
     */
  /* void clearAuthentication (in string username, in string password); */
  NS_SCRIPTABLE NS_IMETHOD ClearAuthentication(const char *username, const char *password) = 0;

  /**
     * Set character encoding.  The default charset if this function is
     * not called is "UTF-8".
     *
     * @param encoding  encoding charset to be used
     */
  /* void setEncoding (in string encoding); */
  NS_SCRIPTABLE NS_IMETHOD SetEncoding(const char *encoding) = 0;

  /**
     * The URL of the XML-RPC server
     */
  /* readonly attribute nsIURL serverURL; */
  NS_SCRIPTABLE NS_IMETHOD GetServerURL(nsIURL * *aServerURL) = 0;

  /**
     * Call remote method methodName asynchronously with given arguments.
     * 
     * Supported arguments are:
     * nsISupportsPRUint8, nsISupportsPRUint16,
     * nsISupportsPRInt16, nsISupportsPRInt32: <i4>
     * nsISupportsPRBool: <boolean>
     * nsISupportsChar, nsISupportsCString: <string>
     * nsISupportsFloat, nsISupportsDouble: <double>
     * nsISupportsPRTime: <dateTime.iso8601>
     * nsIInputStream: <base64>
     * nsISupportsArray: <array>
     * nsIDictionary: <struct>
     *
     * Note that both nsISupportsArray and nsIDictionary can only hold any of
     * the supported input types.
     *
     * Return value will be converted as follows:
     * <i4> or <int>: nsISupportsPRInt32
     * <boolean>: nsISupportsPRBool
     * <string>: nsISupportsCString
     * <double>: nsISupportsDouble
     * <dateTime.iso8601>: nsISupportsPRTime
     * <base64>: nsISupportsCString
     * <array>: nsISupportsArray
     * <struct>: nsIDictionary
     *
     * <fault>s (server side errors) are indicated by returning
     * NS_ERROR_FAILURE. Via nsIXPConnect::GetPendingException()->data a
     * nsIXmlRpcFault object can be retreieved with more information on the
     * fault.
     *
     * @param listener          A nsIXmlRpcClientListener that will get notified
     *                          of XML-RPC events.
     * @param context           A context to be passed on to the listener.
     * @param methodName        Remote method to call.
     * @param arguments         Array of arguments to pass to remote method.
     * @return                  Return value of remote method.
     */
  /* void asyncCall (in nsIXmlRpcClientListener listener, in nsISupports ctxt, in string methodName, [array, size_is (count)] in nsISupports arguments, in PRUint32 count); */
  NS_SCRIPTABLE NS_IMETHOD AsyncCall(nsIXmlRpcClientListener *listener, nsISupports *ctxt, const char *methodName, nsISupports **arguments, PRUint32 count) = 0;

  /**
     * Whether or not a call is in progress
     */
  /* readonly attribute boolean inProgress; */
  NS_SCRIPTABLE NS_IMETHOD GetInProgress(PRBool *aInProgress) = 0;

  /**
     * The most recent XML-RPC fault from returned from this server.
     * null if the last call didn't return an XML-RPC fault.
     */
  /* readonly attribute nsIXmlRpcFault fault; */
  NS_SCRIPTABLE NS_IMETHOD GetFault(nsIXmlRpcFault * *aFault) = 0;

  /**
     * The most recent XML-RPC call result returned from this server.
     * null if the last call didn't return a valid result
     */
  /* readonly attribute nsISupports result; */
  NS_SCRIPTABLE NS_IMETHOD GetResult(nsISupports * *aResult) = 0;

  /**
     * The most recent HTTP status code returned from this server
     * null if the server was unreachable or not yet contacted.
     */
  /* readonly attribute unsigned long responseStatus; */
  NS_SCRIPTABLE NS_IMETHOD GetResponseStatus(PRUint32 *aResponseStatus) = 0;

  /* readonly attribute unsigned long responseString; */
  NS_SCRIPTABLE NS_IMETHOD GetResponseString(PRUint32 *aResponseString) = 0;

  /**
     * Convenience: return the correct nsISupportsPrimitive for a given XML-RPC
     * type, or nsISupportsArray or nsIDictionary.  'base64' isn't supported.
     * No QueryInterface call is needed, the object has been QueryInterfaced to
     * the correct interface.
     *
     * @param type              One of the listed constants.
     * @param uuid              The uuid of the returned object. No QI call 
     *                          needed.
     * @param nsQIResult        The apropriate XPCOM object.
     */
  /* void createType (in unsigned long type, out nsIIDRef uuid, [iid_is (uuid), retval] out nsQIResult result); */
  NS_SCRIPTABLE NS_IMETHOD CreateType(PRUint32 type, nsIID & *uuid, void * *result) = 0;

  enum { INT = 1U };

  enum { BOOLEAN = 2U };

  enum { STRING = 3U };

  enum { DOUBLE = 4U };

  enum { DATETIME = 5U };

  enum { ARRAY = 6U };

  enum { STRUCT = 7U };

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIXmlRpcClient, NS_IXMLRPCCLIENT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIXMLRPCCLIENT \
  NS_SCRIPTABLE NS_IMETHOD Init(const char *serverURL); \
  NS_SCRIPTABLE NS_IMETHOD SetAuthentication(const char *username, const char *password); \
  NS_SCRIPTABLE NS_IMETHOD ClearAuthentication(const char *username, const char *password); \
  NS_SCRIPTABLE NS_IMETHOD SetEncoding(const char *encoding); \
  NS_SCRIPTABLE NS_IMETHOD GetServerURL(nsIURL * *aServerURL); \
  NS_SCRIPTABLE NS_IMETHOD AsyncCall(nsIXmlRpcClientListener *listener, nsISupports *ctxt, const char *methodName, nsISupports **arguments, PRUint32 count); \
  NS_SCRIPTABLE NS_IMETHOD GetInProgress(PRBool *aInProgress); \
  NS_SCRIPTABLE NS_IMETHOD GetFault(nsIXmlRpcFault * *aFault); \
  NS_SCRIPTABLE NS_IMETHOD GetResult(nsISupports * *aResult); \
  NS_SCRIPTABLE NS_IMETHOD GetResponseStatus(PRUint32 *aResponseStatus); \
  NS_SCRIPTABLE NS_IMETHOD GetResponseString(PRUint32 *aResponseString); \
  NS_SCRIPTABLE NS_IMETHOD CreateType(PRUint32 type, nsIID & *uuid, void * *result); \

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIXMLRPCCLIENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD Init(const char *serverURL) { return _to Init(serverURL); } \
  NS_SCRIPTABLE NS_IMETHOD SetAuthentication(const char *username, const char *password) { return _to SetAuthentication(username, password); } \
  NS_SCRIPTABLE NS_IMETHOD ClearAuthentication(const char *username, const char *password) { return _to ClearAuthentication(username, password); } \
  NS_SCRIPTABLE NS_IMETHOD SetEncoding(const char *encoding) { return _to SetEncoding(encoding); } \
  NS_SCRIPTABLE NS_IMETHOD GetServerURL(nsIURL * *aServerURL) { return _to GetServerURL(aServerURL); } \
  NS_SCRIPTABLE NS_IMETHOD AsyncCall(nsIXmlRpcClientListener *listener, nsISupports *ctxt, const char *methodName, nsISupports **arguments, PRUint32 count) { return _to AsyncCall(listener, ctxt, methodName, arguments, count); } \
  NS_SCRIPTABLE NS_IMETHOD GetInProgress(PRBool *aInProgress) { return _to GetInProgress(aInProgress); } \
  NS_SCRIPTABLE NS_IMETHOD GetFault(nsIXmlRpcFault * *aFault) { return _to GetFault(aFault); } \
  NS_SCRIPTABLE NS_IMETHOD GetResult(nsISupports * *aResult) { return _to GetResult(aResult); } \
  NS_SCRIPTABLE NS_IMETHOD GetResponseStatus(PRUint32 *aResponseStatus) { return _to GetResponseStatus(aResponseStatus); } \
  NS_SCRIPTABLE NS_IMETHOD GetResponseString(PRUint32 *aResponseString) { return _to GetResponseString(aResponseString); } \
  NS_SCRIPTABLE NS_IMETHOD CreateType(PRUint32 type, nsIID & *uuid, void * *result) { return _to CreateType(type, uuid, result); } \

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIXMLRPCCLIENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD Init(const char *serverURL) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(serverURL); } \
  NS_SCRIPTABLE NS_IMETHOD SetAuthentication(const char *username, const char *password) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAuthentication(username, password); } \
  NS_SCRIPTABLE NS_IMETHOD ClearAuthentication(const char *username, const char *password) { return !_to ? NS_ERROR_NULL_POINTER : _to->ClearAuthentication(username, password); } \
  NS_SCRIPTABLE NS_IMETHOD SetEncoding(const char *encoding) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetEncoding(encoding); } \
  NS_SCRIPTABLE NS_IMETHOD GetServerURL(nsIURL * *aServerURL) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetServerURL(aServerURL); } \
  NS_SCRIPTABLE NS_IMETHOD AsyncCall(nsIXmlRpcClientListener *listener, nsISupports *ctxt, const char *methodName, nsISupports **arguments, PRUint32 count) { return !_to ? NS_ERROR_NULL_POINTER : _to->AsyncCall(listener, ctxt, methodName, arguments, count); } \
  NS_SCRIPTABLE NS_IMETHOD GetInProgress(PRBool *aInProgress) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetInProgress(aInProgress); } \
  NS_SCRIPTABLE NS_IMETHOD GetFault(nsIXmlRpcFault * *aFault) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFault(aFault); } \
  NS_SCRIPTABLE NS_IMETHOD GetResult(nsISupports * *aResult) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetResult(aResult); } \
  NS_SCRIPTABLE NS_IMETHOD GetResponseStatus(PRUint32 *aResponseStatus) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetResponseStatus(aResponseStatus); } \
  NS_SCRIPTABLE NS_IMETHOD GetResponseString(PRUint32 *aResponseString) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetResponseString(aResponseString); } \
  NS_SCRIPTABLE NS_IMETHOD CreateType(PRUint32 type, nsIID & *uuid, void * *result) { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateType(type, uuid, result); } \

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsXmlRpcClient : public nsIXmlRpcClient
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXMLRPCCLIENT

  nsXmlRpcClient();

private:
  ~nsXmlRpcClient();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsXmlRpcClient, nsIXmlRpcClient)

nsXmlRpcClient::nsXmlRpcClient()
{
  /* member initializers and constructor code */
}

nsXmlRpcClient::~nsXmlRpcClient()
{
  /* destructor code */
}

/* void init (in string serverURL); */
NS_IMETHODIMP nsXmlRpcClient::Init(const char *serverURL)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setAuthentication (in string username, in string password); */
NS_IMETHODIMP nsXmlRpcClient::SetAuthentication(const char *username, const char *password)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void clearAuthentication (in string username, in string password); */
NS_IMETHODIMP nsXmlRpcClient::ClearAuthentication(const char *username, const char *password)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setEncoding (in string encoding); */
NS_IMETHODIMP nsXmlRpcClient::SetEncoding(const char *encoding)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIURL serverURL; */
NS_IMETHODIMP nsXmlRpcClient::GetServerURL(nsIURL * *aServerURL)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void asyncCall (in nsIXmlRpcClientListener listener, in nsISupports ctxt, in string methodName, [array, size_is (count)] in nsISupports arguments, in PRUint32 count); */
NS_IMETHODIMP nsXmlRpcClient::AsyncCall(nsIXmlRpcClientListener *listener, nsISupports *ctxt, const char *methodName, nsISupports **arguments, PRUint32 count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean inProgress; */
NS_IMETHODIMP nsXmlRpcClient::GetInProgress(PRBool *aInProgress)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIXmlRpcFault fault; */
NS_IMETHODIMP nsXmlRpcClient::GetFault(nsIXmlRpcFault * *aFault)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsISupports result; */
NS_IMETHODIMP nsXmlRpcClient::GetResult(nsISupports * *aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long responseStatus; */
NS_IMETHODIMP nsXmlRpcClient::GetResponseStatus(PRUint32 *aResponseStatus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long responseString; */
NS_IMETHODIMP nsXmlRpcClient::GetResponseString(PRUint32 *aResponseString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void createType (in unsigned long type, out nsIIDRef uuid, [iid_is (uuid), retval] out nsQIResult result); */
NS_IMETHODIMP nsXmlRpcClient::CreateType(PRUint32 type, nsIID & *uuid, void * *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIXmlRpcFault */
#define NS_IXMLRPCFAULT_IID_STR "691cb864-0a7e-448c-98ee-4a7f359cf145"

#define NS_IXMLRPCFAULT_IID \
  {0x691cb864, 0x0a7e, 0x448c, \
    { 0x98, 0xee, 0x4a, 0x7f, 0x35, 0x9c, 0xf1, 0x45 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIXmlRpcFault : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXMLRPCFAULT_IID)

  /**
 * An XML-RPC exception.
 * XML-RPC server fault codes are returned wrapped in this Access via
 * nsIXPConnect::GetPendingException()->data
 */
  /* readonly attribute PRInt32 faultCode; */
  NS_SCRIPTABLE NS_IMETHOD GetFaultCode(PRInt32 *aFaultCode) = 0;

  /* readonly attribute string faultString; */
  NS_SCRIPTABLE NS_IMETHOD GetFaultString(char * *aFaultString) = 0;

  /* void init (in PRInt32 faultCode, in string faultString); */
  NS_SCRIPTABLE NS_IMETHOD Init(PRInt32 faultCode, const char *faultString) = 0;

  /* string toString (); */
  NS_SCRIPTABLE NS_IMETHOD ToString(char **_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIXmlRpcFault, NS_IXMLRPCFAULT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIXMLRPCFAULT \
  NS_SCRIPTABLE NS_IMETHOD GetFaultCode(PRInt32 *aFaultCode); \
  NS_SCRIPTABLE NS_IMETHOD GetFaultString(char * *aFaultString); \
  NS_SCRIPTABLE NS_IMETHOD Init(PRInt32 faultCode, const char *faultString); \
  NS_SCRIPTABLE NS_IMETHOD ToString(char **_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIXMLRPCFAULT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetFaultCode(PRInt32 *aFaultCode) { return _to GetFaultCode(aFaultCode); } \
  NS_SCRIPTABLE NS_IMETHOD GetFaultString(char * *aFaultString) { return _to GetFaultString(aFaultString); } \
  NS_SCRIPTABLE NS_IMETHOD Init(PRInt32 faultCode, const char *faultString) { return _to Init(faultCode, faultString); } \
  NS_SCRIPTABLE NS_IMETHOD ToString(char **_retval) { return _to ToString(_retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIXMLRPCFAULT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetFaultCode(PRInt32 *aFaultCode) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFaultCode(aFaultCode); } \
  NS_SCRIPTABLE NS_IMETHOD GetFaultString(char * *aFaultString) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFaultString(aFaultString); } \
  NS_SCRIPTABLE NS_IMETHOD Init(PRInt32 faultCode, const char *faultString) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(faultCode, faultString); } \
  NS_SCRIPTABLE NS_IMETHOD ToString(char **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->ToString(_retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsXmlRpcFault : public nsIXmlRpcFault
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXMLRPCFAULT

  nsXmlRpcFault();

private:
  ~nsXmlRpcFault();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsXmlRpcFault, nsIXmlRpcFault)

nsXmlRpcFault::nsXmlRpcFault()
{
  /* member initializers and constructor code */
}

nsXmlRpcFault::~nsXmlRpcFault()
{
  /* destructor code */
}

/* readonly attribute PRInt32 faultCode; */
NS_IMETHODIMP nsXmlRpcFault::GetFaultCode(PRInt32 *aFaultCode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string faultString; */
NS_IMETHODIMP nsXmlRpcFault::GetFaultString(char * *aFaultString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void init (in PRInt32 faultCode, in string faultString); */
NS_IMETHODIMP nsXmlRpcFault::Init(PRInt32 faultCode, const char *faultString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* string toString (); */
NS_IMETHODIMP nsXmlRpcFault::ToString(char **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIXmlRpcClient_h__ */
