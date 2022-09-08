/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/dom/public/idl/offline/nsIDOMOfflineResourceList.idl
 */

#ifndef __gen_nsIDOMOfflineResourceList_h__
#define __gen_nsIDOMOfflineResourceList_h__


#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMOfflineResourceList */
#define NS_IDOMOFFLINERESOURCELIST_IID_STR "8449bce2-0d8c-4c74-ab79-b41b8d81f1c4"

#define NS_IDOMOFFLINERESOURCELIST_IID \
  {0x8449bce2, 0x0d8c, 0x4c74, \
    { 0xab, 0x79, 0xb4, 0x1b, 0x8d, 0x81, 0xf1, 0xc4 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMOfflineResourceList : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMOFFLINERESOURCELIST_IID)

  /**
   * Enumerate the list of dynamically-managed entries.
   */
  /* readonly attribute unsigned long length; */
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength) = 0;

  /* DOMString item (in unsigned long index); */
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsAString & _retval) = 0;

  /**
   * Add an item to the list of dynamically-managed entries.  The resource
   * will be fetched into the application cache.
   *
   * @param uri
   *        The resource to add.
   */
  /* void add (in DOMString uri); */
  NS_SCRIPTABLE NS_IMETHOD Add(const nsAString & uri) = 0;

  /**
   * Remove an item from the list of dynamically-managed entries.  If this
   * was the last reference to a URI in the application cache, the cache
   * entry will be removed.
   *
   * @param uri
   *        The resource to remove.
   */
  /* void remove (in DOMString uri); */
  NS_SCRIPTABLE NS_IMETHOD Remove(const nsAString & uri) = 0;

  /**
   * State of the application cache this object is associated with.
   */
  enum { UNCACHED = 0U };

  enum { IDLE = 1U };

  enum { CHECKING = 2U };

  enum { DOWNLOADING = 3U };

  /**
   * There is a new version of the application cache available
   *
   * Versioned application caches are not currently implemented, so this
   * value will not yet be returned
   */
  enum { UPDATEREADY = 4U };

  /* readonly attribute unsigned short status; */
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus) = 0;

  /**
   * Begin the application update process on the associated application cache.
   */
  /* void update (); */
  NS_SCRIPTABLE NS_IMETHOD Update(void) = 0;

  /**
   * Swap in the newest version of the application cache.
   *
   * Versioned application caches are not currently implemented, so this
   * method will throw an exception.
   */
  /* void swapCache (); */
  NS_SCRIPTABLE NS_IMETHOD SwapCache(void) = 0;

  /* attribute nsIDOMEventListener onchecking; */
  NS_SCRIPTABLE NS_IMETHOD GetOnchecking(nsIDOMEventListener * *aOnchecking) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetOnchecking(nsIDOMEventListener * aOnchecking) = 0;

  /* attribute nsIDOMEventListener onerror; */
  NS_SCRIPTABLE NS_IMETHOD GetOnerror(nsIDOMEventListener * *aOnerror) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetOnerror(nsIDOMEventListener * aOnerror) = 0;

  /* attribute nsIDOMEventListener onnoupdate; */
  NS_SCRIPTABLE NS_IMETHOD GetOnnoupdate(nsIDOMEventListener * *aOnnoupdate) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetOnnoupdate(nsIDOMEventListener * aOnnoupdate) = 0;

  /* attribute nsIDOMEventListener ondownloading; */
  NS_SCRIPTABLE NS_IMETHOD GetOndownloading(nsIDOMEventListener * *aOndownloading) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetOndownloading(nsIDOMEventListener * aOndownloading) = 0;

  /* attribute nsIDOMEventListener onprogress; */
  NS_SCRIPTABLE NS_IMETHOD GetOnprogress(nsIDOMEventListener * *aOnprogress) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetOnprogress(nsIDOMEventListener * aOnprogress) = 0;

  /* attribute nsIDOMEventListener onupdateready; */
  NS_SCRIPTABLE NS_IMETHOD GetOnupdateready(nsIDOMEventListener * *aOnupdateready) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetOnupdateready(nsIDOMEventListener * aOnupdateready) = 0;

  /* attribute nsIDOMEventListener oncached; */
  NS_SCRIPTABLE NS_IMETHOD GetOncached(nsIDOMEventListener * *aOncached) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetOncached(nsIDOMEventListener * aOncached) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMOfflineResourceList, NS_IDOMOFFLINERESOURCELIST_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMOFFLINERESOURCELIST \
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength); \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsAString & _retval); \
  NS_SCRIPTABLE NS_IMETHOD Add(const nsAString & uri); \
  NS_SCRIPTABLE NS_IMETHOD Remove(const nsAString & uri); \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus); \
  NS_SCRIPTABLE NS_IMETHOD Update(void); \
  NS_SCRIPTABLE NS_IMETHOD SwapCache(void); \
  NS_SCRIPTABLE NS_IMETHOD GetOnchecking(nsIDOMEventListener * *aOnchecking); \
  NS_SCRIPTABLE NS_IMETHOD SetOnchecking(nsIDOMEventListener * aOnchecking); \
  NS_SCRIPTABLE NS_IMETHOD GetOnerror(nsIDOMEventListener * *aOnerror); \
  NS_SCRIPTABLE NS_IMETHOD SetOnerror(nsIDOMEventListener * aOnerror); \
  NS_SCRIPTABLE NS_IMETHOD GetOnnoupdate(nsIDOMEventListener * *aOnnoupdate); \
  NS_SCRIPTABLE NS_IMETHOD SetOnnoupdate(nsIDOMEventListener * aOnnoupdate); \
  NS_SCRIPTABLE NS_IMETHOD GetOndownloading(nsIDOMEventListener * *aOndownloading); \
  NS_SCRIPTABLE NS_IMETHOD SetOndownloading(nsIDOMEventListener * aOndownloading); \
  NS_SCRIPTABLE NS_IMETHOD GetOnprogress(nsIDOMEventListener * *aOnprogress); \
  NS_SCRIPTABLE NS_IMETHOD SetOnprogress(nsIDOMEventListener * aOnprogress); \
  NS_SCRIPTABLE NS_IMETHOD GetOnupdateready(nsIDOMEventListener * *aOnupdateready); \
  NS_SCRIPTABLE NS_IMETHOD SetOnupdateready(nsIDOMEventListener * aOnupdateready); \
  NS_SCRIPTABLE NS_IMETHOD GetOncached(nsIDOMEventListener * *aOncached); \
  NS_SCRIPTABLE NS_IMETHOD SetOncached(nsIDOMEventListener * aOncached); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMOFFLINERESOURCELIST(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength) { return _to GetLength(aLength); } \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsAString & _retval) { return _to Item(index, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD Add(const nsAString & uri) { return _to Add(uri); } \
  NS_SCRIPTABLE NS_IMETHOD Remove(const nsAString & uri) { return _to Remove(uri); } \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus) { return _to GetStatus(aStatus); } \
  NS_SCRIPTABLE NS_IMETHOD Update(void) { return _to Update(); } \
  NS_SCRIPTABLE NS_IMETHOD SwapCache(void) { return _to SwapCache(); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnchecking(nsIDOMEventListener * *aOnchecking) { return _to GetOnchecking(aOnchecking); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnchecking(nsIDOMEventListener * aOnchecking) { return _to SetOnchecking(aOnchecking); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnerror(nsIDOMEventListener * *aOnerror) { return _to GetOnerror(aOnerror); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnerror(nsIDOMEventListener * aOnerror) { return _to SetOnerror(aOnerror); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnnoupdate(nsIDOMEventListener * *aOnnoupdate) { return _to GetOnnoupdate(aOnnoupdate); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnnoupdate(nsIDOMEventListener * aOnnoupdate) { return _to SetOnnoupdate(aOnnoupdate); } \
  NS_SCRIPTABLE NS_IMETHOD GetOndownloading(nsIDOMEventListener * *aOndownloading) { return _to GetOndownloading(aOndownloading); } \
  NS_SCRIPTABLE NS_IMETHOD SetOndownloading(nsIDOMEventListener * aOndownloading) { return _to SetOndownloading(aOndownloading); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnprogress(nsIDOMEventListener * *aOnprogress) { return _to GetOnprogress(aOnprogress); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnprogress(nsIDOMEventListener * aOnprogress) { return _to SetOnprogress(aOnprogress); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnupdateready(nsIDOMEventListener * *aOnupdateready) { return _to GetOnupdateready(aOnupdateready); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnupdateready(nsIDOMEventListener * aOnupdateready) { return _to SetOnupdateready(aOnupdateready); } \
  NS_SCRIPTABLE NS_IMETHOD GetOncached(nsIDOMEventListener * *aOncached) { return _to GetOncached(aOncached); } \
  NS_SCRIPTABLE NS_IMETHOD SetOncached(nsIDOMEventListener * aOncached) { return _to SetOncached(aOncached); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMOFFLINERESOURCELIST(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLength(aLength); } \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsAString & _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Item(index, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD Add(const nsAString & uri) { return !_to ? NS_ERROR_NULL_POINTER : _to->Add(uri); } \
  NS_SCRIPTABLE NS_IMETHOD Remove(const nsAString & uri) { return !_to ? NS_ERROR_NULL_POINTER : _to->Remove(uri); } \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetStatus(aStatus); } \
  NS_SCRIPTABLE NS_IMETHOD Update(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Update(); } \
  NS_SCRIPTABLE NS_IMETHOD SwapCache(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->SwapCache(); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnchecking(nsIDOMEventListener * *aOnchecking) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOnchecking(aOnchecking); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnchecking(nsIDOMEventListener * aOnchecking) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOnchecking(aOnchecking); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnerror(nsIDOMEventListener * *aOnerror) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOnerror(aOnerror); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnerror(nsIDOMEventListener * aOnerror) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOnerror(aOnerror); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnnoupdate(nsIDOMEventListener * *aOnnoupdate) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOnnoupdate(aOnnoupdate); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnnoupdate(nsIDOMEventListener * aOnnoupdate) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOnnoupdate(aOnnoupdate); } \
  NS_SCRIPTABLE NS_IMETHOD GetOndownloading(nsIDOMEventListener * *aOndownloading) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOndownloading(aOndownloading); } \
  NS_SCRIPTABLE NS_IMETHOD SetOndownloading(nsIDOMEventListener * aOndownloading) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOndownloading(aOndownloading); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnprogress(nsIDOMEventListener * *aOnprogress) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOnprogress(aOnprogress); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnprogress(nsIDOMEventListener * aOnprogress) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOnprogress(aOnprogress); } \
  NS_SCRIPTABLE NS_IMETHOD GetOnupdateready(nsIDOMEventListener * *aOnupdateready) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOnupdateready(aOnupdateready); } \
  NS_SCRIPTABLE NS_IMETHOD SetOnupdateready(nsIDOMEventListener * aOnupdateready) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOnupdateready(aOnupdateready); } \
  NS_SCRIPTABLE NS_IMETHOD GetOncached(nsIDOMEventListener * *aOncached) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOncached(aOncached); } \
  NS_SCRIPTABLE NS_IMETHOD SetOncached(nsIDOMEventListener * aOncached) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOncached(aOncached); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMOfflineResourceList : public nsIDOMOfflineResourceList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMOFFLINERESOURCELIST

  nsDOMOfflineResourceList();

private:
  ~nsDOMOfflineResourceList();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMOfflineResourceList, nsIDOMOfflineResourceList)

nsDOMOfflineResourceList::nsDOMOfflineResourceList()
{
  /* member initializers and constructor code */
}

nsDOMOfflineResourceList::~nsDOMOfflineResourceList()
{
  /* destructor code */
}

/* readonly attribute unsigned long length; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetLength(PRUint32 *aLength)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString item (in unsigned long index); */
NS_IMETHODIMP nsDOMOfflineResourceList::Item(PRUint32 index, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void add (in DOMString uri); */
NS_IMETHODIMP nsDOMOfflineResourceList::Add(const nsAString & uri)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void remove (in DOMString uri); */
NS_IMETHODIMP nsDOMOfflineResourceList::Remove(const nsAString & uri)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned short status; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetStatus(PRUint16 *aStatus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void update (); */
NS_IMETHODIMP nsDOMOfflineResourceList::Update()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void swapCache (); */
NS_IMETHODIMP nsDOMOfflineResourceList::SwapCache()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIDOMEventListener onchecking; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetOnchecking(nsIDOMEventListener * *aOnchecking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMOfflineResourceList::SetOnchecking(nsIDOMEventListener * aOnchecking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIDOMEventListener onerror; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetOnerror(nsIDOMEventListener * *aOnerror)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMOfflineResourceList::SetOnerror(nsIDOMEventListener * aOnerror)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIDOMEventListener onnoupdate; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetOnnoupdate(nsIDOMEventListener * *aOnnoupdate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMOfflineResourceList::SetOnnoupdate(nsIDOMEventListener * aOnnoupdate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIDOMEventListener ondownloading; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetOndownloading(nsIDOMEventListener * *aOndownloading)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMOfflineResourceList::SetOndownloading(nsIDOMEventListener * aOndownloading)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIDOMEventListener onprogress; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetOnprogress(nsIDOMEventListener * *aOnprogress)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMOfflineResourceList::SetOnprogress(nsIDOMEventListener * aOnprogress)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIDOMEventListener onupdateready; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetOnupdateready(nsIDOMEventListener * *aOnupdateready)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMOfflineResourceList::SetOnupdateready(nsIDOMEventListener * aOnupdateready)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIDOMEventListener oncached; */
NS_IMETHODIMP nsDOMOfflineResourceList::GetOncached(nsIDOMEventListener * *aOncached)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMOfflineResourceList::SetOncached(nsIDOMEventListener * aOncached)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMOfflineResourceList_h__ */
