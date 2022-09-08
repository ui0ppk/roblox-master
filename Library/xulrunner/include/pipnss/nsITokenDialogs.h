/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/security/manager/ssl/public/nsITokenDialogs.idl
 */

#ifndef __gen_nsITokenDialogs_h__
#define __gen_nsITokenDialogs_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIInterfaceRequestor; /* forward declaration */

class nsIProtectedAuthThread; /* forward declaration */


/* starting interface:    nsITokenDialogs */
#define NS_ITOKENDIALOGS_IID_STR "bb4bae9c-39c5-11d5-ba26-00108303b117"

#define NS_ITOKENDIALOGS_IID \
  {0xbb4bae9c, 0x39c5, 0x11d5, \
    { 0xba, 0x26, 0x00, 0x10, 0x83, 0x03, 0xb1, 0x17 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsITokenDialogs : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITOKENDIALOGS_IID)

  /* void ChooseToken (in nsIInterfaceRequestor ctx, [array, size_is (count)] in wstring tokenNameList, in unsigned long count, out wstring tokenName, out boolean canceled); */
  NS_SCRIPTABLE NS_IMETHOD ChooseToken(nsIInterfaceRequestor *ctx, const PRUnichar **tokenNameList, PRUint32 count, PRUnichar **tokenName, PRBool *canceled) = 0;

  /**
    * displayProtectedAuth - displays notification dialog to the user 
    * that he is expected to authenticate to the token using its
    * "protected authentication path" feature
    */
  /* void displayProtectedAuth (in nsIInterfaceRequestor ctx, in nsIProtectedAuthThread runnable); */
  NS_SCRIPTABLE NS_IMETHOD DisplayProtectedAuth(nsIInterfaceRequestor *ctx, nsIProtectedAuthThread *runnable) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsITokenDialogs, NS_ITOKENDIALOGS_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSITOKENDIALOGS \
  NS_SCRIPTABLE NS_IMETHOD ChooseToken(nsIInterfaceRequestor *ctx, const PRUnichar **tokenNameList, PRUint32 count, PRUnichar **tokenName, PRBool *canceled); \
  NS_SCRIPTABLE NS_IMETHOD DisplayProtectedAuth(nsIInterfaceRequestor *ctx, nsIProtectedAuthThread *runnable); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSITOKENDIALOGS(_to) \
  NS_SCRIPTABLE NS_IMETHOD ChooseToken(nsIInterfaceRequestor *ctx, const PRUnichar **tokenNameList, PRUint32 count, PRUnichar **tokenName, PRBool *canceled) { return _to ChooseToken(ctx, tokenNameList, count, tokenName, canceled); } \
  NS_SCRIPTABLE NS_IMETHOD DisplayProtectedAuth(nsIInterfaceRequestor *ctx, nsIProtectedAuthThread *runnable) { return _to DisplayProtectedAuth(ctx, runnable); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSITOKENDIALOGS(_to) \
  NS_SCRIPTABLE NS_IMETHOD ChooseToken(nsIInterfaceRequestor *ctx, const PRUnichar **tokenNameList, PRUint32 count, PRUnichar **tokenName, PRBool *canceled) { return !_to ? NS_ERROR_NULL_POINTER : _to->ChooseToken(ctx, tokenNameList, count, tokenName, canceled); } \
  NS_SCRIPTABLE NS_IMETHOD DisplayProtectedAuth(nsIInterfaceRequestor *ctx, nsIProtectedAuthThread *runnable) { return !_to ? NS_ERROR_NULL_POINTER : _to->DisplayProtectedAuth(ctx, runnable); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsTokenDialogs : public nsITokenDialogs
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITOKENDIALOGS

  nsTokenDialogs();

private:
  ~nsTokenDialogs();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsTokenDialogs, nsITokenDialogs)

nsTokenDialogs::nsTokenDialogs()
{
  /* member initializers and constructor code */
}

nsTokenDialogs::~nsTokenDialogs()
{
  /* destructor code */
}

/* void ChooseToken (in nsIInterfaceRequestor ctx, [array, size_is (count)] in wstring tokenNameList, in unsigned long count, out wstring tokenName, out boolean canceled); */
NS_IMETHODIMP nsTokenDialogs::ChooseToken(nsIInterfaceRequestor *ctx, const PRUnichar **tokenNameList, PRUint32 count, PRUnichar **tokenName, PRBool *canceled)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void displayProtectedAuth (in nsIInterfaceRequestor ctx, in nsIProtectedAuthThread runnable); */
NS_IMETHODIMP nsTokenDialogs::DisplayProtectedAuth(nsIInterfaceRequestor *ctx, nsIProtectedAuthThread *runnable)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif

#define NS_TOKENDIALOGS_CONTRACTID "@mozilla.org/nsTokenDialogs;1"

#endif /* __gen_nsITokenDialogs_h__ */
