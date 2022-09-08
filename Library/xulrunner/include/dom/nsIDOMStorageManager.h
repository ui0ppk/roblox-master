/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/dom/public/idl/storage/nsIDOMStorageManager.idl
 */

#ifndef __gen_nsIDOMStorageManager_h__
#define __gen_nsIDOMStorageManager_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMStorageManager */
#define NS_IDOMSTORAGEMANAGER_IID_STR "6e4bc25e-f056-4c6c-b27e-89152ca91834"

#define NS_IDOMSTORAGEMANAGER_IID \
  {0x6e4bc25e, 0xf056, 0x4c6c, \
    { 0xb2, 0x7e, 0x89, 0x15, 0x2c, 0xa9, 0x18, 0x34 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMStorageManager : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMSTORAGEMANAGER_IID)

  /**
   * Return the amount of disk space used by a domain.  Usage is checked
   * against the domain of the page that set the key (the owner domain), not
   * the domain of the storage object.
   *
   * @param aOwnerDomain The domain to check.
   * @returns the space usage of the domain, in bytes.
   */
  /* long getUsage (in AString aOwnerDomain); */
  NS_SCRIPTABLE NS_IMETHOD GetUsage(const nsAString & aOwnerDomain, PRInt32 *_retval) = 0;

  /**
   * Clear keys owned by offline applications.  All data owned by a domain
   * with the "offline-app" permission will be removed from the database.
   */
  /* void clearOfflineApps (); */
  NS_SCRIPTABLE NS_IMETHOD ClearOfflineApps(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMStorageManager, NS_IDOMSTORAGEMANAGER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMSTORAGEMANAGER \
  NS_SCRIPTABLE NS_IMETHOD GetUsage(const nsAString & aOwnerDomain, PRInt32 *_retval); \
  NS_SCRIPTABLE NS_IMETHOD ClearOfflineApps(void); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMSTORAGEMANAGER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetUsage(const nsAString & aOwnerDomain, PRInt32 *_retval) { return _to GetUsage(aOwnerDomain, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ClearOfflineApps(void) { return _to ClearOfflineApps(); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMSTORAGEMANAGER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetUsage(const nsAString & aOwnerDomain, PRInt32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUsage(aOwnerDomain, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ClearOfflineApps(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->ClearOfflineApps(); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMStorageManager : public nsIDOMStorageManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSTORAGEMANAGER

  nsDOMStorageManager();

private:
  ~nsDOMStorageManager();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMStorageManager, nsIDOMStorageManager)

nsDOMStorageManager::nsDOMStorageManager()
{
  /* member initializers and constructor code */
}

nsDOMStorageManager::~nsDOMStorageManager()
{
  /* destructor code */
}

/* long getUsage (in AString aOwnerDomain); */
NS_IMETHODIMP nsDOMStorageManager::GetUsage(const nsAString & aOwnerDomain, PRInt32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void clearOfflineApps (); */
NS_IMETHODIMP nsDOMStorageManager::ClearOfflineApps()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMStorageManager_h__ */
