/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/security/manager/ssl/public/nsIClientAuthDialogs.idl
 */

#ifndef __gen_nsIClientAuthDialogs_h__
#define __gen_nsIClientAuthDialogs_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIInterfaceRequestor; /* forward declaration */


/* starting interface:    nsIClientAuthDialogs */
#define NS_ICLIENTAUTHDIALOGS_IID_STR "fa4c7520-1433-11d5-ba24-00108303b117"

#define NS_ICLIENTAUTHDIALOGS_IID \
  {0xfa4c7520, 0x1433, 0x11d5, \
    { 0xba, 0x24, 0x00, 0x10, 0x83, 0x03, 0xb1, 0x17 }}

/**
 * nsIClientAuthDialog
 * Provides UI for SSL client-auth dialogs.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIClientAuthDialogs : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICLIENTAUTHDIALOGS_IID)

  /**
   * display
   *   UI shown when a user is asked to do SSL client auth.
   */
  /* void ChooseCertificate (in nsIInterfaceRequestor ctx, in wstring cn, in wstring organization, in wstring issuer, [array, size_is (count)] in wstring certNickList, [array, size_is (count)] in wstring certDetailsList, in unsigned long count, out long selectedIndex, out boolean canceled); */
  NS_SCRIPTABLE NS_IMETHOD ChooseCertificate(nsIInterfaceRequestor *ctx, const PRUnichar *cn, const PRUnichar *organization, const PRUnichar *issuer, const PRUnichar **certNickList, const PRUnichar **certDetailsList, PRUint32 count, PRInt32 *selectedIndex, PRBool *canceled) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIClientAuthDialogs, NS_ICLIENTAUTHDIALOGS_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSICLIENTAUTHDIALOGS \
  NS_SCRIPTABLE NS_IMETHOD ChooseCertificate(nsIInterfaceRequestor *ctx, const PRUnichar *cn, const PRUnichar *organization, const PRUnichar *issuer, const PRUnichar **certNickList, const PRUnichar **certDetailsList, PRUint32 count, PRInt32 *selectedIndex, PRBool *canceled); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSICLIENTAUTHDIALOGS(_to) \
  NS_SCRIPTABLE NS_IMETHOD ChooseCertificate(nsIInterfaceRequestor *ctx, const PRUnichar *cn, const PRUnichar *organization, const PRUnichar *issuer, const PRUnichar **certNickList, const PRUnichar **certDetailsList, PRUint32 count, PRInt32 *selectedIndex, PRBool *canceled) { return _to ChooseCertificate(ctx, cn, organization, issuer, certNickList, certDetailsList, count, selectedIndex, canceled); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSICLIENTAUTHDIALOGS(_to) \
  NS_SCRIPTABLE NS_IMETHOD ChooseCertificate(nsIInterfaceRequestor *ctx, const PRUnichar *cn, const PRUnichar *organization, const PRUnichar *issuer, const PRUnichar **certNickList, const PRUnichar **certDetailsList, PRUint32 count, PRInt32 *selectedIndex, PRBool *canceled) { return !_to ? NS_ERROR_NULL_POINTER : _to->ChooseCertificate(ctx, cn, organization, issuer, certNickList, certDetailsList, count, selectedIndex, canceled); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsClientAuthDialogs : public nsIClientAuthDialogs
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIENTAUTHDIALOGS

  nsClientAuthDialogs();

private:
  ~nsClientAuthDialogs();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsClientAuthDialogs, nsIClientAuthDialogs)

nsClientAuthDialogs::nsClientAuthDialogs()
{
  /* member initializers and constructor code */
}

nsClientAuthDialogs::~nsClientAuthDialogs()
{
  /* destructor code */
}

/* void ChooseCertificate (in nsIInterfaceRequestor ctx, in wstring cn, in wstring organization, in wstring issuer, [array, size_is (count)] in wstring certNickList, [array, size_is (count)] in wstring certDetailsList, in unsigned long count, out long selectedIndex, out boolean canceled); */
NS_IMETHODIMP nsClientAuthDialogs::ChooseCertificate(nsIInterfaceRequestor *ctx, const PRUnichar *cn, const PRUnichar *organization, const PRUnichar *issuer, const PRUnichar **certNickList, const PRUnichar **certDetailsList, PRUint32 count, PRInt32 *selectedIndex, PRBool *canceled)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif

#define NS_CLIENTAUTHDIALOGS_CONTRACTID "@mozilla.org/nsClientAuthDialogs;1"

#endif /* __gen_nsIClientAuthDialogs_h__ */
