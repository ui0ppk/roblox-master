/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/toolkit/mozapps/extensions/public/nsIBlocklistService.idl
 */

#ifndef __gen_nsIBlocklistService_h__
#define __gen_nsIBlocklistService_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIBlocklistService */
#define NS_IBLOCKLISTSERVICE_IID_STR "0c3fe697-d50d-4f42-b747-0c5855cfc60e"

#define NS_IBLOCKLISTSERVICE_IID \
  {0x0c3fe697, 0xd50d, 0x4f42, \
    { 0xb7, 0x47, 0x0c, 0x58, 0x55, 0xcf, 0xc6, 0x0e }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIBlocklistService : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBLOCKLISTSERVICE_IID)

  /**
   * Determine if an item is blocklisted
   * @param   id
   *          The GUID of the item.
   * @param   version
   *          The item's version.
   * @param   appVersion
   *          The version of the application we are checking in the blocklist.
   *          If this parameter is null, the version of the running application
   *          is used.
   * @param   toolkitVersion
   *          The version of the toolkit we are checking in the blocklist.
   *          If this parameter is null, the version of the running toolkit
   *          is used.
   * @returns true if the item is compatible with this version of the
   *          application or this version of the toolkit, false, otherwise.
   */
  /* boolean isAddonBlocklisted (in AString id, in AString version, in AString appVersion, in AString toolkitVersion); */
  NS_SCRIPTABLE NS_IMETHOD IsAddonBlocklisted(const nsAString & id, const nsAString & version, const nsAString & appVersion, const nsAString & toolkitVersion, PRBool *_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIBlocklistService, NS_IBLOCKLISTSERVICE_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIBLOCKLISTSERVICE \
  NS_SCRIPTABLE NS_IMETHOD IsAddonBlocklisted(const nsAString & id, const nsAString & version, const nsAString & appVersion, const nsAString & toolkitVersion, PRBool *_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIBLOCKLISTSERVICE(_to) \
  NS_SCRIPTABLE NS_IMETHOD IsAddonBlocklisted(const nsAString & id, const nsAString & version, const nsAString & appVersion, const nsAString & toolkitVersion, PRBool *_retval) { return _to IsAddonBlocklisted(id, version, appVersion, toolkitVersion, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIBLOCKLISTSERVICE(_to) \
  NS_SCRIPTABLE NS_IMETHOD IsAddonBlocklisted(const nsAString & id, const nsAString & version, const nsAString & appVersion, const nsAString & toolkitVersion, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->IsAddonBlocklisted(id, version, appVersion, toolkitVersion, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsBlocklistService : public nsIBlocklistService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBLOCKLISTSERVICE

  nsBlocklistService();

private:
  ~nsBlocklistService();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsBlocklistService, nsIBlocklistService)

nsBlocklistService::nsBlocklistService()
{
  /* member initializers and constructor code */
}

nsBlocklistService::~nsBlocklistService()
{
  /* destructor code */
}

/* boolean isAddonBlocklisted (in AString id, in AString version, in AString appVersion, in AString toolkitVersion); */
NS_IMETHODIMP nsBlocklistService::IsAddonBlocklisted(const nsAString & id, const nsAString & version, const nsAString & appVersion, const nsAString & toolkitVersion, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIBlocklistService_h__ */
