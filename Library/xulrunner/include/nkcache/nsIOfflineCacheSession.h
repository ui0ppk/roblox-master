/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/netwerk/cache/public/nsIOfflineCacheSession.idl
 */

#ifndef __gen_nsIOfflineCacheSession_h__
#define __gen_nsIOfflineCacheSession_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsICache_h__
#include "nsICache.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIOfflineCacheSession */
#define NS_IOFFLINECACHESESSION_IID_STR "3a33e268-4175-4440-a933-89d461c86c5f"

#define NS_IOFFLINECACHESESSION_IID \
  {0x3a33e268, 0x4175, 0x4440, \
    { 0xa9, 0x33, 0x89, 0xd4, 0x61, 0xc8, 0x6c, 0x5f }}

/**
 * The offline cache is meant to reliably store resources for
 * offline use.  The expected semantics are:
 *
 * a) Once populated, the cache will not evict an application resource
 *    unless explicitly asked.
 *
 * b) Resources no longer in use by the application should be evicted.
 *
 * c) If the cache fills up, new entries should be rejected rather
 *    than throwing out old ones.
 *
 * The offline cache uses domains to concretely represent an
 * application.  It maintains a list of resources to be pinned for
 * each domain.  This list is separate from actual cache
 * population - the caller is still responsible for placing items
 * in the cache, and ownership can be declared without a
 * corresponding entry.
 *
 * A key can optionally be associated with a specific URI within
 * the domain.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIOfflineCacheSession : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOFFLINECACHESESSION_IID)

  /**
     * Gets the list of owner domains in the cache.
     *
     * @param count
     *        The number of domains returned
     * @param uris
     *        The domains that own resources in the cache
     */
  /* void getOwnerDomains (out unsigned long count, [array, size_is (count)] out string domains); */
  NS_SCRIPTABLE NS_IMETHOD GetOwnerDomains(PRUint32 *count, char ***domains) = 0;

  /**
     * Gets the list of owner URIs associated with a domain.
     *
     * @param ownerAsciiDomain
     *        The domain to query
     *        !! IMPORTANT !! : This must be ascii encoded host - nsIURI.asciiHost
     * @param count
     *        The number of uris returned
     * @param uris
     *        The uris in this domain that own resources
     */
  /* void getOwnerURIs (in ACString ownerAsciiDomain, out unsigned long count, [array, size_is (count)] out string uris); */
  NS_SCRIPTABLE NS_IMETHOD GetOwnerURIs(const nsACString & ownerAsciiDomain, PRUint32 *count, char ***uris) = 0;

  /**
     * Sets the resources owned by a given domain/URI pair.
     *
     * Setting a list will remove any resources previously owned by this
     * domain/URI pair.
     *
     * A key can be added while there is no associated entry.  When
     * an entry is created with this key, it will be owned by the
     * domain/URI pair.
     *
     * @param ownerAsciiDomain
     *        The domain that owns the resources
     *        !! IMPORTANT !! : This must be ascii encoded host - nsIURI.asciiHost
     * @param ownerAsciiKey
     *        The specific key that owns the resources.  You may use 
     *        ascii encoded URI spec of the owner - nsIURI.asciiSpec.
     *        This can be empty if none specifically owns the resources.
     * @param count
     *        The number of keys in keys.
     * @param keys
     *        The keys that the domain/URI pair own.  This can be empty to
     *        clear ownership for the domain/URI pair.
     */
  /* void setOwnedKeys (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in unsigned long count, [array, size_is (count)] in string keys); */
  NS_SCRIPTABLE NS_IMETHOD SetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 count, const char **keys) = 0;

  /**
     * Gets the list of resources owned by a given domain/URI pair.
     *
     * @param ownerAsciiDomain
     *        The domain that owns the resources
     *        !! IMPORTANT !! : This must be ascii encoded host - nsIURI.asciiHost
     * @param ownerAsciiKey
     *        The specific key that owns the resources.  You may use 
     *        ascii encoded URI spec of the owner - nsIURI.asciiSpec.
     *        This can be empty if none specifically owns the resources.
     * @param count
     *        The number of keys in keys.
     * @param keys
     *        The keys that the domain/URI pair own.
     */
  /* void getOwnedKeys (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, out unsigned long count, [array, size_is (count)] out string keys); */
  NS_SCRIPTABLE NS_IMETHOD GetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 *count, char ***keys) = 0;

  /**
     * Adds an owned key to a domain/URI pair.
     *
     * A key can be added while there is no associated entry.  When
     * an entry is created with this key, it will be owned by the
     * domain/URI pair.
     *
     * @param ownerAsciiDomain
     *        The domain that owns the resources
     *        !! IMPORTANT !! : This must be ascii encoded host - nsIURI.asciiHost
     * @param ownerAsciiKey
     *        The specific key that owns the resources.  You may use 
     *        ascii encoded URI spec of the owner - nsIURI.asciiSpec.
     *        This can be empty if none specifically owns the resources.
     * @param key
     *        The key to add.
     */
  /* void addOwnedKey (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in ACString key); */
  NS_SCRIPTABLE NS_IMETHOD AddOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key) = 0;

  /**
     * Removes an owned key from a domain/URI pair.
     *
     * If the key does not exist, an NS_ERROR_NOT_AVAILABLE exception
     * will be thrown.
     *
     * @param ownerAsciiDomain
     *        The domain that owns the resources
     *        !! IMPORTANT !! : This must be ascii encoded host - nsIURI.asciiHost
     * @param ownerAsciiKey
     *        The specific key that owns the resources.  You may use 
     *        ascii encoded URI spec of the owner - nsIURI.asciiSpec.
     *        This can be empty if none specifically owns the resources.
     * @param key The key to remove.
     */
  /* void removeOwnedKey (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in ACString key); */
  NS_SCRIPTABLE NS_IMETHOD RemoveOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key) = 0;

  /**
     * Checks whether a key is owned by a given domain/URI pair.
     *
     * @param ownerAsciiDomain
     *        The domain that owns the resources
     *        !! IMPORTANT !! : This must be ascii encoded host - nsIURI.asciiHost
     * @param ownerAsciiKey
     *        The specific key that owns the resources.  You may use 
     *        ascii encoded URI spec of the owner - nsIURI.asciiSpec.
     *        This can be empty if none specifically owns the resources.
     * @param key The key to check
     */
  /* boolean keyIsOwned (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in ACString key); */
  NS_SCRIPTABLE NS_IMETHOD KeyIsOwned(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key, PRBool *_retval) = 0;

  /**
     * Remove all keys owned by a domain, including keys owned by
     * a specific URI.
     *
     * @param ownerAsciiDomain
     *        The domain for which keys should be removed
     *        !! IMPORTANT !! : This must be ascii encoded host - nsIURI.asciiHost
     */
  /* void clearKeysOwnedByDomain (in ACString ownerAsciiDomain); */
  NS_SCRIPTABLE NS_IMETHOD ClearKeysOwnedByDomain(const nsACString & ownerAsciiDomain) = 0;

  /**
     * Get the number of bytes used in the cache by a domain.
     *
     * @param domain The domain to check.
     */
  /* unsigned long getDomainUsage (in ACString ownerDomain); */
  NS_SCRIPTABLE NS_IMETHOD GetDomainUsage(const nsACString & ownerDomain, PRUint32 *_retval) = 0;

  /**
     * Evict all entries that are not owned by a domain.
     */
  /* void evictUnownedEntries (); */
  NS_SCRIPTABLE NS_IMETHOD EvictUnownedEntries(void) = 0;

  /**
     * Merge the items from a temporary clientID in to this client.  This lets
     * offline cache updates accumulate in a temporary client and be moved
     * in all at once.
     *
     * Entries in the temporary client will replace any entries in this client
     * with the same cache key.
     *
     * Ownership lists for a given domain/URI pair from the temporary client
     * will replace ownership lists for the same domain/URI pair.
     */
  /* void mergeTemporaryClientID (in ACString temporaryClientID); */
  NS_SCRIPTABLE NS_IMETHOD MergeTemporaryClientID(const nsACString & temporaryClientID) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIOfflineCacheSession, NS_IOFFLINECACHESESSION_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIOFFLINECACHESESSION \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerDomains(PRUint32 *count, char ***domains); \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerURIs(const nsACString & ownerAsciiDomain, PRUint32 *count, char ***uris); \
  NS_SCRIPTABLE NS_IMETHOD SetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 count, const char **keys); \
  NS_SCRIPTABLE NS_IMETHOD GetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 *count, char ***keys); \
  NS_SCRIPTABLE NS_IMETHOD AddOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key); \
  NS_SCRIPTABLE NS_IMETHOD RemoveOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key); \
  NS_SCRIPTABLE NS_IMETHOD KeyIsOwned(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key, PRBool *_retval); \
  NS_SCRIPTABLE NS_IMETHOD ClearKeysOwnedByDomain(const nsACString & ownerAsciiDomain); \
  NS_SCRIPTABLE NS_IMETHOD GetDomainUsage(const nsACString & ownerDomain, PRUint32 *_retval); \
  NS_SCRIPTABLE NS_IMETHOD EvictUnownedEntries(void); \
  NS_SCRIPTABLE NS_IMETHOD MergeTemporaryClientID(const nsACString & temporaryClientID); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIOFFLINECACHESESSION(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerDomains(PRUint32 *count, char ***domains) { return _to GetOwnerDomains(count, domains); } \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerURIs(const nsACString & ownerAsciiDomain, PRUint32 *count, char ***uris) { return _to GetOwnerURIs(ownerAsciiDomain, count, uris); } \
  NS_SCRIPTABLE NS_IMETHOD SetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 count, const char **keys) { return _to SetOwnedKeys(ownerAsciiDomain, ownerAsciiKey, count, keys); } \
  NS_SCRIPTABLE NS_IMETHOD GetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 *count, char ***keys) { return _to GetOwnedKeys(ownerAsciiDomain, ownerAsciiKey, count, keys); } \
  NS_SCRIPTABLE NS_IMETHOD AddOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key) { return _to AddOwnedKey(ownerAsciiDomain, ownerAsciiKey, key); } \
  NS_SCRIPTABLE NS_IMETHOD RemoveOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key) { return _to RemoveOwnedKey(ownerAsciiDomain, ownerAsciiKey, key); } \
  NS_SCRIPTABLE NS_IMETHOD KeyIsOwned(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key, PRBool *_retval) { return _to KeyIsOwned(ownerAsciiDomain, ownerAsciiKey, key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ClearKeysOwnedByDomain(const nsACString & ownerAsciiDomain) { return _to ClearKeysOwnedByDomain(ownerAsciiDomain); } \
  NS_SCRIPTABLE NS_IMETHOD GetDomainUsage(const nsACString & ownerDomain, PRUint32 *_retval) { return _to GetDomainUsage(ownerDomain, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD EvictUnownedEntries(void) { return _to EvictUnownedEntries(); } \
  NS_SCRIPTABLE NS_IMETHOD MergeTemporaryClientID(const nsACString & temporaryClientID) { return _to MergeTemporaryClientID(temporaryClientID); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIOFFLINECACHESESSION(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerDomains(PRUint32 *count, char ***domains) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOwnerDomains(count, domains); } \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerURIs(const nsACString & ownerAsciiDomain, PRUint32 *count, char ***uris) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOwnerURIs(ownerAsciiDomain, count, uris); } \
  NS_SCRIPTABLE NS_IMETHOD SetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 count, const char **keys) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOwnedKeys(ownerAsciiDomain, ownerAsciiKey, count, keys); } \
  NS_SCRIPTABLE NS_IMETHOD GetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 *count, char ***keys) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOwnedKeys(ownerAsciiDomain, ownerAsciiKey, count, keys); } \
  NS_SCRIPTABLE NS_IMETHOD AddOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddOwnedKey(ownerAsciiDomain, ownerAsciiKey, key); } \
  NS_SCRIPTABLE NS_IMETHOD RemoveOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveOwnedKey(ownerAsciiDomain, ownerAsciiKey, key); } \
  NS_SCRIPTABLE NS_IMETHOD KeyIsOwned(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->KeyIsOwned(ownerAsciiDomain, ownerAsciiKey, key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ClearKeysOwnedByDomain(const nsACString & ownerAsciiDomain) { return !_to ? NS_ERROR_NULL_POINTER : _to->ClearKeysOwnedByDomain(ownerAsciiDomain); } \
  NS_SCRIPTABLE NS_IMETHOD GetDomainUsage(const nsACString & ownerDomain, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDomainUsage(ownerDomain, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD EvictUnownedEntries(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->EvictUnownedEntries(); } \
  NS_SCRIPTABLE NS_IMETHOD MergeTemporaryClientID(const nsACString & temporaryClientID) { return !_to ? NS_ERROR_NULL_POINTER : _to->MergeTemporaryClientID(temporaryClientID); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsOfflineCacheSession : public nsIOfflineCacheSession
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOFFLINECACHESESSION

  nsOfflineCacheSession();

private:
  ~nsOfflineCacheSession();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsOfflineCacheSession, nsIOfflineCacheSession)

nsOfflineCacheSession::nsOfflineCacheSession()
{
  /* member initializers and constructor code */
}

nsOfflineCacheSession::~nsOfflineCacheSession()
{
  /* destructor code */
}

/* void getOwnerDomains (out unsigned long count, [array, size_is (count)] out string domains); */
NS_IMETHODIMP nsOfflineCacheSession::GetOwnerDomains(PRUint32 *count, char ***domains)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getOwnerURIs (in ACString ownerAsciiDomain, out unsigned long count, [array, size_is (count)] out string uris); */
NS_IMETHODIMP nsOfflineCacheSession::GetOwnerURIs(const nsACString & ownerAsciiDomain, PRUint32 *count, char ***uris)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setOwnedKeys (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in unsigned long count, [array, size_is (count)] in string keys); */
NS_IMETHODIMP nsOfflineCacheSession::SetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 count, const char **keys)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getOwnedKeys (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, out unsigned long count, [array, size_is (count)] out string keys); */
NS_IMETHODIMP nsOfflineCacheSession::GetOwnedKeys(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, PRUint32 *count, char ***keys)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addOwnedKey (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in ACString key); */
NS_IMETHODIMP nsOfflineCacheSession::AddOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void removeOwnedKey (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in ACString key); */
NS_IMETHODIMP nsOfflineCacheSession::RemoveOwnedKey(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean keyIsOwned (in ACString ownerAsciiDomain, in ACString ownerAsciiKey, in ACString key); */
NS_IMETHODIMP nsOfflineCacheSession::KeyIsOwned(const nsACString & ownerAsciiDomain, const nsACString & ownerAsciiKey, const nsACString & key, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void clearKeysOwnedByDomain (in ACString ownerAsciiDomain); */
NS_IMETHODIMP nsOfflineCacheSession::ClearKeysOwnedByDomain(const nsACString & ownerAsciiDomain)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long getDomainUsage (in ACString ownerDomain); */
NS_IMETHODIMP nsOfflineCacheSession::GetDomainUsage(const nsACString & ownerDomain, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void evictUnownedEntries (); */
NS_IMETHODIMP nsOfflineCacheSession::EvictUnownedEntries()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void mergeTemporaryClientID (in ACString temporaryClientID); */
NS_IMETHODIMP nsOfflineCacheSession::MergeTemporaryClientID(const nsACString & temporaryClientID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIOfflineCacheSession_h__ */
