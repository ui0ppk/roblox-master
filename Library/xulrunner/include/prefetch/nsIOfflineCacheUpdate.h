/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/uriloader/prefetch/nsIOfflineCacheUpdate.idl
 */

#ifndef __gen_nsIOfflineCacheUpdate_h__
#define __gen_nsIOfflineCacheUpdate_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIURI; /* forward declaration */

class nsIDOMNode; /* forward declaration */

class nsIDOMDocument; /* forward declaration */

class nsIDOMLoadStatus; /* forward declaration */

class nsIOfflineCacheUpdate; /* forward declaration */


/* starting interface:    nsIOfflineCacheUpdateObserver */
#define NS_IOFFLINECACHEUPDATEOBSERVER_IID_STR "0aa38757-999c-44d6-bdb4-7dd32634fa83"

#define NS_IOFFLINECACHEUPDATEOBSERVER_IID \
  {0x0aa38757, 0x999c, 0x44d6, \
    { 0xbd, 0xb4, 0x7d, 0xd3, 0x26, 0x34, 0xfa, 0x83 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIOfflineCacheUpdateObserver : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOFFLINECACHEUPDATEOBSERVER_IID)

  /**
   * There was an error updating the cache.
   *
   * @param aUpdate
   *        The nsIOfflineCacheUpdate being processed.
   */
  /* void error (in nsIOfflineCacheUpdate aUpdate); */
  NS_SCRIPTABLE NS_IMETHOD Error(nsIOfflineCacheUpdate *aUpdate) = 0;

  /**
   * The manifest is being checked for updates
   *
   * @param aUpdate
   *        The nsIOfflineCacheUpdate being processed.
   */
  /* void checking (in nsIOfflineCacheUpdate aUpdate); */
  NS_SCRIPTABLE NS_IMETHOD Checking(nsIOfflineCacheUpdate *aUpdate) = 0;

  /**
   * No update was necessary.
   *
   * @param aUpdate
   *        The nsIOfflineCacheUpdate being processed.
   */
  /* void noUpdate (in nsIOfflineCacheUpdate aUpdate); */
  NS_SCRIPTABLE NS_IMETHOD NoUpdate(nsIOfflineCacheUpdate *aUpdate) = 0;

  /**
   * Starting to download resources
   *
   * @param aUpdate
   *        The nsIOfflineCacheUpdate being processed.
   */
  /* void downloading (in nsIOfflineCacheUpdate aUpdate); */
  NS_SCRIPTABLE NS_IMETHOD Downloading(nsIOfflineCacheUpdate *aUpdate) = 0;

  /**
   * An item has started downloading.
   *
   * @param aUpdate
   *        The nsIOfflineCacheUpdate being processed.
   * @param aItem
   *        load status for the item that is being downloaded.
   */
  /* void itemStarted (in nsIOfflineCacheUpdate aUpdate, in nsIDOMLoadStatus aItem); */
  NS_SCRIPTABLE NS_IMETHOD ItemStarted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem) = 0;

  /**
   * An item has finished loading.
   *
   * @param aUpdate
   *        The nsIOfflineCacheUpdate being processed.
   * @param aItem
   *         load status for the item that completed.
   */
  /* void itemCompleted (in nsIOfflineCacheUpdate aUpdate, in nsIDOMLoadStatus aItem); */
  NS_SCRIPTABLE NS_IMETHOD ItemCompleted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIOfflineCacheUpdateObserver, NS_IOFFLINECACHEUPDATEOBSERVER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIOFFLINECACHEUPDATEOBSERVER \
  NS_SCRIPTABLE NS_IMETHOD Error(nsIOfflineCacheUpdate *aUpdate); \
  NS_SCRIPTABLE NS_IMETHOD Checking(nsIOfflineCacheUpdate *aUpdate); \
  NS_SCRIPTABLE NS_IMETHOD NoUpdate(nsIOfflineCacheUpdate *aUpdate); \
  NS_SCRIPTABLE NS_IMETHOD Downloading(nsIOfflineCacheUpdate *aUpdate); \
  NS_SCRIPTABLE NS_IMETHOD ItemStarted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem); \
  NS_SCRIPTABLE NS_IMETHOD ItemCompleted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIOFFLINECACHEUPDATEOBSERVER(_to) \
  NS_SCRIPTABLE NS_IMETHOD Error(nsIOfflineCacheUpdate *aUpdate) { return _to Error(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD Checking(nsIOfflineCacheUpdate *aUpdate) { return _to Checking(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD NoUpdate(nsIOfflineCacheUpdate *aUpdate) { return _to NoUpdate(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD Downloading(nsIOfflineCacheUpdate *aUpdate) { return _to Downloading(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD ItemStarted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem) { return _to ItemStarted(aUpdate, aItem); } \
  NS_SCRIPTABLE NS_IMETHOD ItemCompleted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem) { return _to ItemCompleted(aUpdate, aItem); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIOFFLINECACHEUPDATEOBSERVER(_to) \
  NS_SCRIPTABLE NS_IMETHOD Error(nsIOfflineCacheUpdate *aUpdate) { return !_to ? NS_ERROR_NULL_POINTER : _to->Error(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD Checking(nsIOfflineCacheUpdate *aUpdate) { return !_to ? NS_ERROR_NULL_POINTER : _to->Checking(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD NoUpdate(nsIOfflineCacheUpdate *aUpdate) { return !_to ? NS_ERROR_NULL_POINTER : _to->NoUpdate(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD Downloading(nsIOfflineCacheUpdate *aUpdate) { return !_to ? NS_ERROR_NULL_POINTER : _to->Downloading(aUpdate); } \
  NS_SCRIPTABLE NS_IMETHOD ItemStarted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->ItemStarted(aUpdate, aItem); } \
  NS_SCRIPTABLE NS_IMETHOD ItemCompleted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem) { return !_to ? NS_ERROR_NULL_POINTER : _to->ItemCompleted(aUpdate, aItem); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsOfflineCacheUpdateObserver : public nsIOfflineCacheUpdateObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOFFLINECACHEUPDATEOBSERVER

  nsOfflineCacheUpdateObserver();

private:
  ~nsOfflineCacheUpdateObserver();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsOfflineCacheUpdateObserver, nsIOfflineCacheUpdateObserver)

nsOfflineCacheUpdateObserver::nsOfflineCacheUpdateObserver()
{
  /* member initializers and constructor code */
}

nsOfflineCacheUpdateObserver::~nsOfflineCacheUpdateObserver()
{
  /* destructor code */
}

/* void error (in nsIOfflineCacheUpdate aUpdate); */
NS_IMETHODIMP nsOfflineCacheUpdateObserver::Error(nsIOfflineCacheUpdate *aUpdate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void checking (in nsIOfflineCacheUpdate aUpdate); */
NS_IMETHODIMP nsOfflineCacheUpdateObserver::Checking(nsIOfflineCacheUpdate *aUpdate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void noUpdate (in nsIOfflineCacheUpdate aUpdate); */
NS_IMETHODIMP nsOfflineCacheUpdateObserver::NoUpdate(nsIOfflineCacheUpdate *aUpdate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void downloading (in nsIOfflineCacheUpdate aUpdate); */
NS_IMETHODIMP nsOfflineCacheUpdateObserver::Downloading(nsIOfflineCacheUpdate *aUpdate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void itemStarted (in nsIOfflineCacheUpdate aUpdate, in nsIDOMLoadStatus aItem); */
NS_IMETHODIMP nsOfflineCacheUpdateObserver::ItemStarted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void itemCompleted (in nsIOfflineCacheUpdate aUpdate, in nsIDOMLoadStatus aItem); */
NS_IMETHODIMP nsOfflineCacheUpdateObserver::ItemCompleted(nsIOfflineCacheUpdate *aUpdate, nsIDOMLoadStatus *aItem)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIOfflineCacheUpdate */
#define NS_IOFFLINECACHEUPDATE_IID_STR "7dc06ede-1098-4384-b95e-65525ab254c9"

#define NS_IOFFLINECACHEUPDATE_IID \
  {0x7dc06ede, 0x1098, 0x4384, \
    { 0xb9, 0x5e, 0x65, 0x52, 0x5a, 0xb2, 0x54, 0xc9 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIOfflineCacheUpdate : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOFFLINECACHEUPDATE_IID)

  /**
 * An nsIOfflineCacheUpdate is used to update an application's offline
 * resources.
 *
 * It can be used to perform partial or complete updates.
 *
 * Each update object maintains a list of nsIDOMLoadStatus items for the
 * resources it is updating.  The list of these items will be available
 * after the object is scheduled.
 *
 * One update object will be updating at a time.  The active object will
 * load its items one by one, sending itemCompleted() to any registered
 * observers.
 */
/**
   * Fetch the status of the running update.  This will return a value
   * defined in nsIDOMOfflineResourceList.
   */
  /* readonly attribute unsigned short status; */
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus) = 0;

  /**
   * TRUE if the update is being used to add specific resources.
   * FALSE if the complete cache update process is happening.
   */
  /* readonly attribute boolean partial; */
  NS_SCRIPTABLE NS_IMETHOD GetPartial(PRBool *aPartial) = 0;

  /**
   * The domain being updated, and the domain that will own any URIs added
   * with this update.
   */
  /* readonly attribute ACString updateDomain; */
  NS_SCRIPTABLE NS_IMETHOD GetUpdateDomain(nsACString & aUpdateDomain) = 0;

  /**
   * The manifest for the offline application being updated.
   */
  /* readonly attribute nsIURI manifestURI; */
  NS_SCRIPTABLE NS_IMETHOD GetManifestURI(nsIURI * *aManifestURI) = 0;

  /**
   * TRUE if the cache update completed successfully.
   */
  /* readonly attribute boolean succeeded; */
  NS_SCRIPTABLE NS_IMETHOD GetSucceeded(PRBool *aSucceeded) = 0;

  /**
   * Initialize the update.
   *
   * @param aPartialUpdate
   *        TRUE if the update should just update the URIs given to it,
   *        FALSE if all URLs for the owner domain should be added.
   * @param aManifestURI
   *        The manifest URI to be checked, or for partial updates the
   *        manifest that should own resources that are added.
   * @param aDocumentURI
   *        The page that is requesting the update.
   */
  /* void init (in boolean aPartialUpdate, in nsIURI aManifestURI, in nsIURI aDocumentURI); */
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool aPartialUpdate, nsIURI *aManifestURI, nsIURI *aDocumentURI) = 0;

  /**
   * Add a URI to the offline cache as part of the update.
   *
   * @param aURI
   *        The URI to add.
   */
  /* void addDynamicURI (in nsIURI aURI); */
  NS_SCRIPTABLE NS_IMETHOD AddDynamicURI(nsIURI *aURI) = 0;

  /**
   * Add the update to the offline update queue.  An offline-cache-update-added
   * event will be sent to the observer service.
   */
  /* void schedule (); */
  NS_SCRIPTABLE NS_IMETHOD Schedule(void) = 0;

  /**
   * Access to the list of items in the update.
   */
  /* readonly attribute unsigned long count; */
  NS_SCRIPTABLE NS_IMETHOD GetCount(PRUint32 *aCount) = 0;

  /* nsIDOMLoadStatus item (in unsigned long index); */
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval) = 0;

  /**
   * Observe loads that are added to the update.
   *
   * @param aObserver
   *        object that notifications will be sent to.
   * @param aHoldWeak
   *        TRUE if you want the update to hold a weak reference to the
   *        observer, FALSE for a strong reference.
   */
  /* void addObserver (in nsIOfflineCacheUpdateObserver aObserver, in boolean aHoldWeak); */
  NS_SCRIPTABLE NS_IMETHOD AddObserver(nsIOfflineCacheUpdateObserver *aObserver, PRBool aHoldWeak) = 0;

  /**
   * Remove an observer from the update.
   *
   * @param aObserver
   *        the observer to remove.
   */
  /* void removeObserver (in nsIOfflineCacheUpdateObserver aObserver); */
  NS_SCRIPTABLE NS_IMETHOD RemoveObserver(nsIOfflineCacheUpdateObserver *aObserver) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIOfflineCacheUpdate, NS_IOFFLINECACHEUPDATE_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIOFFLINECACHEUPDATE \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus); \
  NS_SCRIPTABLE NS_IMETHOD GetPartial(PRBool *aPartial); \
  NS_SCRIPTABLE NS_IMETHOD GetUpdateDomain(nsACString & aUpdateDomain); \
  NS_SCRIPTABLE NS_IMETHOD GetManifestURI(nsIURI * *aManifestURI); \
  NS_SCRIPTABLE NS_IMETHOD GetSucceeded(PRBool *aSucceeded); \
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool aPartialUpdate, nsIURI *aManifestURI, nsIURI *aDocumentURI); \
  NS_SCRIPTABLE NS_IMETHOD AddDynamicURI(nsIURI *aURI); \
  NS_SCRIPTABLE NS_IMETHOD Schedule(void); \
  NS_SCRIPTABLE NS_IMETHOD GetCount(PRUint32 *aCount); \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval); \
  NS_SCRIPTABLE NS_IMETHOD AddObserver(nsIOfflineCacheUpdateObserver *aObserver, PRBool aHoldWeak); \
  NS_SCRIPTABLE NS_IMETHOD RemoveObserver(nsIOfflineCacheUpdateObserver *aObserver); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIOFFLINECACHEUPDATE(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus) { return _to GetStatus(aStatus); } \
  NS_SCRIPTABLE NS_IMETHOD GetPartial(PRBool *aPartial) { return _to GetPartial(aPartial); } \
  NS_SCRIPTABLE NS_IMETHOD GetUpdateDomain(nsACString & aUpdateDomain) { return _to GetUpdateDomain(aUpdateDomain); } \
  NS_SCRIPTABLE NS_IMETHOD GetManifestURI(nsIURI * *aManifestURI) { return _to GetManifestURI(aManifestURI); } \
  NS_SCRIPTABLE NS_IMETHOD GetSucceeded(PRBool *aSucceeded) { return _to GetSucceeded(aSucceeded); } \
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool aPartialUpdate, nsIURI *aManifestURI, nsIURI *aDocumentURI) { return _to Init(aPartialUpdate, aManifestURI, aDocumentURI); } \
  NS_SCRIPTABLE NS_IMETHOD AddDynamicURI(nsIURI *aURI) { return _to AddDynamicURI(aURI); } \
  NS_SCRIPTABLE NS_IMETHOD Schedule(void) { return _to Schedule(); } \
  NS_SCRIPTABLE NS_IMETHOD GetCount(PRUint32 *aCount) { return _to GetCount(aCount); } \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval) { return _to Item(index, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD AddObserver(nsIOfflineCacheUpdateObserver *aObserver, PRBool aHoldWeak) { return _to AddObserver(aObserver, aHoldWeak); } \
  NS_SCRIPTABLE NS_IMETHOD RemoveObserver(nsIOfflineCacheUpdateObserver *aObserver) { return _to RemoveObserver(aObserver); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIOFFLINECACHEUPDATE(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(PRUint16 *aStatus) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetStatus(aStatus); } \
  NS_SCRIPTABLE NS_IMETHOD GetPartial(PRBool *aPartial) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPartial(aPartial); } \
  NS_SCRIPTABLE NS_IMETHOD GetUpdateDomain(nsACString & aUpdateDomain) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUpdateDomain(aUpdateDomain); } \
  NS_SCRIPTABLE NS_IMETHOD GetManifestURI(nsIURI * *aManifestURI) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetManifestURI(aManifestURI); } \
  NS_SCRIPTABLE NS_IMETHOD GetSucceeded(PRBool *aSucceeded) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSucceeded(aSucceeded); } \
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool aPartialUpdate, nsIURI *aManifestURI, nsIURI *aDocumentURI) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(aPartialUpdate, aManifestURI, aDocumentURI); } \
  NS_SCRIPTABLE NS_IMETHOD AddDynamicURI(nsIURI *aURI) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddDynamicURI(aURI); } \
  NS_SCRIPTABLE NS_IMETHOD Schedule(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Schedule(); } \
  NS_SCRIPTABLE NS_IMETHOD GetCount(PRUint32 *aCount) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCount(aCount); } \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Item(index, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD AddObserver(nsIOfflineCacheUpdateObserver *aObserver, PRBool aHoldWeak) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddObserver(aObserver, aHoldWeak); } \
  NS_SCRIPTABLE NS_IMETHOD RemoveObserver(nsIOfflineCacheUpdateObserver *aObserver) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveObserver(aObserver); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsOfflineCacheUpdate : public nsIOfflineCacheUpdate
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOFFLINECACHEUPDATE

  nsOfflineCacheUpdate();

private:
  ~nsOfflineCacheUpdate();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsOfflineCacheUpdate, nsIOfflineCacheUpdate)

nsOfflineCacheUpdate::nsOfflineCacheUpdate()
{
  /* member initializers and constructor code */
}

nsOfflineCacheUpdate::~nsOfflineCacheUpdate()
{
  /* destructor code */
}

/* readonly attribute unsigned short status; */
NS_IMETHODIMP nsOfflineCacheUpdate::GetStatus(PRUint16 *aStatus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean partial; */
NS_IMETHODIMP nsOfflineCacheUpdate::GetPartial(PRBool *aPartial)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute ACString updateDomain; */
NS_IMETHODIMP nsOfflineCacheUpdate::GetUpdateDomain(nsACString & aUpdateDomain)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIURI manifestURI; */
NS_IMETHODIMP nsOfflineCacheUpdate::GetManifestURI(nsIURI * *aManifestURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean succeeded; */
NS_IMETHODIMP nsOfflineCacheUpdate::GetSucceeded(PRBool *aSucceeded)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void init (in boolean aPartialUpdate, in nsIURI aManifestURI, in nsIURI aDocumentURI); */
NS_IMETHODIMP nsOfflineCacheUpdate::Init(PRBool aPartialUpdate, nsIURI *aManifestURI, nsIURI *aDocumentURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addDynamicURI (in nsIURI aURI); */
NS_IMETHODIMP nsOfflineCacheUpdate::AddDynamicURI(nsIURI *aURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void schedule (); */
NS_IMETHODIMP nsOfflineCacheUpdate::Schedule()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long count; */
NS_IMETHODIMP nsOfflineCacheUpdate::GetCount(PRUint32 *aCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMLoadStatus item (in unsigned long index); */
NS_IMETHODIMP nsOfflineCacheUpdate::Item(PRUint32 index, nsIDOMLoadStatus **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addObserver (in nsIOfflineCacheUpdateObserver aObserver, in boolean aHoldWeak); */
NS_IMETHODIMP nsOfflineCacheUpdate::AddObserver(nsIOfflineCacheUpdateObserver *aObserver, PRBool aHoldWeak)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void removeObserver (in nsIOfflineCacheUpdateObserver aObserver); */
NS_IMETHODIMP nsOfflineCacheUpdate::RemoveObserver(nsIOfflineCacheUpdateObserver *aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIOfflineCacheUpdateService */
#define NS_IOFFLINECACHEUPDATESERVICE_IID_STR "3abee04b-5bbb-4405-b659-35f780e38da0"

#define NS_IOFFLINECACHEUPDATESERVICE_IID \
  {0x3abee04b, 0x5bbb, 0x4405, \
    { 0xb6, 0x59, 0x35, 0xf7, 0x80, 0xe3, 0x8d, 0xa0 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIOfflineCacheUpdateService : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOFFLINECACHEUPDATESERVICE_IID)

  /**
     * Constants for the offline-app permission.
     *
     * XXX: This isn't a great place for this, but it's really the only
     * private offline-app-related interface
     */
/**
     * Allow the domain to use offline APIs, and don't warn about excessive
     * usage.
     */
  enum { ALLOW_NO_WARN = 3U };

  /**
     * Access to the list of cache updates that have been scheduled.
     */
  /* readonly attribute unsigned long numUpdates; */
  NS_SCRIPTABLE NS_IMETHOD GetNumUpdates(PRUint32 *aNumUpdates) = 0;

  /* nsIOfflineCacheUpdate getUpdate (in unsigned long index); */
  NS_SCRIPTABLE NS_IMETHOD GetUpdate(PRUint32 index, nsIOfflineCacheUpdate **_retval) = 0;

  /**
     * Schedule a cache update for a given offline manifest.  If an
     * existing update is scheduled or running, that update will be returned.
     * Otherwise a new update will be scheduled.
     */
  /* nsIOfflineCacheUpdate scheduleUpdate (in nsIURI aManifestURI, in nsIURI aDocumentURI); */
  NS_SCRIPTABLE NS_IMETHOD ScheduleUpdate(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIOfflineCacheUpdate **_retval) = 0;

  /**
     * Schedule a cache update for a manifest when the document finishes
     * loading.
     */
  /* void scheduleOnDocumentStop (in nsIURI aManifestURI, in nsIURI aDocumentURI, in nsIDOMDocument aDocument); */
  NS_SCRIPTABLE NS_IMETHOD ScheduleOnDocumentStop(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIDOMDocument *aDocument) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIOfflineCacheUpdateService, NS_IOFFLINECACHEUPDATESERVICE_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIOFFLINECACHEUPDATESERVICE \
  NS_SCRIPTABLE NS_IMETHOD GetNumUpdates(PRUint32 *aNumUpdates); \
  NS_SCRIPTABLE NS_IMETHOD GetUpdate(PRUint32 index, nsIOfflineCacheUpdate **_retval); \
  NS_SCRIPTABLE NS_IMETHOD ScheduleUpdate(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIOfflineCacheUpdate **_retval); \
  NS_SCRIPTABLE NS_IMETHOD ScheduleOnDocumentStop(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIDOMDocument *aDocument); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIOFFLINECACHEUPDATESERVICE(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetNumUpdates(PRUint32 *aNumUpdates) { return _to GetNumUpdates(aNumUpdates); } \
  NS_SCRIPTABLE NS_IMETHOD GetUpdate(PRUint32 index, nsIOfflineCacheUpdate **_retval) { return _to GetUpdate(index, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ScheduleUpdate(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIOfflineCacheUpdate **_retval) { return _to ScheduleUpdate(aManifestURI, aDocumentURI, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ScheduleOnDocumentStop(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIDOMDocument *aDocument) { return _to ScheduleOnDocumentStop(aManifestURI, aDocumentURI, aDocument); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIOFFLINECACHEUPDATESERVICE(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetNumUpdates(PRUint32 *aNumUpdates) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetNumUpdates(aNumUpdates); } \
  NS_SCRIPTABLE NS_IMETHOD GetUpdate(PRUint32 index, nsIOfflineCacheUpdate **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUpdate(index, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ScheduleUpdate(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIOfflineCacheUpdate **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->ScheduleUpdate(aManifestURI, aDocumentURI, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ScheduleOnDocumentStop(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIDOMDocument *aDocument) { return !_to ? NS_ERROR_NULL_POINTER : _to->ScheduleOnDocumentStop(aManifestURI, aDocumentURI, aDocument); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsOfflineCacheUpdateService : public nsIOfflineCacheUpdateService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOFFLINECACHEUPDATESERVICE

  nsOfflineCacheUpdateService();

private:
  ~nsOfflineCacheUpdateService();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsOfflineCacheUpdateService, nsIOfflineCacheUpdateService)

nsOfflineCacheUpdateService::nsOfflineCacheUpdateService()
{
  /* member initializers and constructor code */
}

nsOfflineCacheUpdateService::~nsOfflineCacheUpdateService()
{
  /* destructor code */
}

/* readonly attribute unsigned long numUpdates; */
NS_IMETHODIMP nsOfflineCacheUpdateService::GetNumUpdates(PRUint32 *aNumUpdates)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIOfflineCacheUpdate getUpdate (in unsigned long index); */
NS_IMETHODIMP nsOfflineCacheUpdateService::GetUpdate(PRUint32 index, nsIOfflineCacheUpdate **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIOfflineCacheUpdate scheduleUpdate (in nsIURI aManifestURI, in nsIURI aDocumentURI); */
NS_IMETHODIMP nsOfflineCacheUpdateService::ScheduleUpdate(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIOfflineCacheUpdate **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void scheduleOnDocumentStop (in nsIURI aManifestURI, in nsIURI aDocumentURI, in nsIDOMDocument aDocument); */
NS_IMETHODIMP nsOfflineCacheUpdateService::ScheduleOnDocumentStop(nsIURI *aManifestURI, nsIURI *aDocumentURI, nsIDOMDocument *aDocument)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIOfflineCacheUpdate_h__ */
