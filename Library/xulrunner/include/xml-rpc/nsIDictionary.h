/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/extensions/xml-rpc/idl/nsIDictionary.idl
 */

#ifndef __gen_nsIDictionary_h__
#define __gen_nsIDictionary_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDictionary */
#define NS_IDICTIONARY_IID_STR "1dd0cb45-aea3-4a52-8b29-01429a542863"

#define NS_IDICTIONARY_IID \
  {0x1dd0cb45, 0xaea3, 0x4a52, \
    { 0x8b, 0x29, 0x01, 0x42, 0x9a, 0x54, 0x28, 0x63 }}

/**
 * A simple mutable table of objects, accessed by key.
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIDictionary : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDICTIONARY_IID)

  /**
     * Check if a given key is present in the dictionary.
     *
     * @param key       Key to check for
     * @return          true if present, false if absent.
     */
  /* boolean hasKey (in string key); */
  NS_SCRIPTABLE NS_IMETHOD HasKey(const char *key, PRBool *_retval) = 0;

  /**
     * Retrieve all keys in the dictionary.
     *
     * @return          array of all keys, unsorted.
     */
  /* void getKeys (out PRUint32 count, [array, size_is (count), retval] out string keys); */
  NS_SCRIPTABLE NS_IMETHOD GetKeys(PRUint32 *count, char ***keys) = 0;

  /**
     * Find the value indicated by the key.
     *
     * @param key       The lookup key indicating the value.
     * @return          Value indicated by key. If the key doesn't exist,
     *                  NS_ERROR_FAILURE will be returned.
     */
  /* nsISupports getValue (in string key); */
  NS_SCRIPTABLE NS_IMETHOD GetValue(const char *key, nsISupports **_retval) = 0;

  /**
     * Add the key-value pair to the dictionary.
     * If the key is already present, replace the old value
     * with the new.
     *
     * @param key       The key by which the value can be accessed
     * @param value     The value to be stored.
     */
  /* void setValue (in string key, in nsISupports value); */
  NS_SCRIPTABLE NS_IMETHOD SetValue(const char *key, nsISupports *value) = 0;

  /**
     * Delete the indicated key-value pair.
     *
     * @param key       The key indicating the pair to be removed.
     * @return          The removed value. If the key doesn't exist,
     *                  NS_ERROR_FAILURE will be returned.
     */
  /* nsISupports deleteValue (in string key); */
  NS_SCRIPTABLE NS_IMETHOD DeleteValue(const char *key, nsISupports **_retval) = 0;

  /**
     * Delete all key-value pairs from the dictionary.
     */
  /* void clear (); */
  NS_SCRIPTABLE NS_IMETHOD Clear(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDictionary, NS_IDICTIONARY_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDICTIONARY \
  NS_SCRIPTABLE NS_IMETHOD HasKey(const char *key, PRBool *_retval); \
  NS_SCRIPTABLE NS_IMETHOD GetKeys(PRUint32 *count, char ***keys); \
  NS_SCRIPTABLE NS_IMETHOD GetValue(const char *key, nsISupports **_retval); \
  NS_SCRIPTABLE NS_IMETHOD SetValue(const char *key, nsISupports *value); \
  NS_SCRIPTABLE NS_IMETHOD DeleteValue(const char *key, nsISupports **_retval); \
  NS_SCRIPTABLE NS_IMETHOD Clear(void); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDICTIONARY(_to) \
  NS_SCRIPTABLE NS_IMETHOD HasKey(const char *key, PRBool *_retval) { return _to HasKey(key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetKeys(PRUint32 *count, char ***keys) { return _to GetKeys(count, keys); } \
  NS_SCRIPTABLE NS_IMETHOD GetValue(const char *key, nsISupports **_retval) { return _to GetValue(key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD SetValue(const char *key, nsISupports *value) { return _to SetValue(key, value); } \
  NS_SCRIPTABLE NS_IMETHOD DeleteValue(const char *key, nsISupports **_retval) { return _to DeleteValue(key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD Clear(void) { return _to Clear(); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDICTIONARY(_to) \
  NS_SCRIPTABLE NS_IMETHOD HasKey(const char *key, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->HasKey(key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetKeys(PRUint32 *count, char ***keys) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetKeys(count, keys); } \
  NS_SCRIPTABLE NS_IMETHOD GetValue(const char *key, nsISupports **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetValue(key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD SetValue(const char *key, nsISupports *value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetValue(key, value); } \
  NS_SCRIPTABLE NS_IMETHOD DeleteValue(const char *key, nsISupports **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->DeleteValue(key, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD Clear(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Clear(); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDictionary : public nsIDictionary
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDICTIONARY

  nsDictionary();

private:
  ~nsDictionary();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDictionary, nsIDictionary)

nsDictionary::nsDictionary()
{
  /* member initializers and constructor code */
}

nsDictionary::~nsDictionary()
{
  /* destructor code */
}

/* boolean hasKey (in string key); */
NS_IMETHODIMP nsDictionary::HasKey(const char *key, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getKeys (out PRUint32 count, [array, size_is (count), retval] out string keys); */
NS_IMETHODIMP nsDictionary::GetKeys(PRUint32 *count, char ***keys)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISupports getValue (in string key); */
NS_IMETHODIMP nsDictionary::GetValue(const char *key, nsISupports **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setValue (in string key, in nsISupports value); */
NS_IMETHODIMP nsDictionary::SetValue(const char *key, nsISupports *value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISupports deleteValue (in string key); */
NS_IMETHODIMP nsDictionary::DeleteValue(const char *key, nsISupports **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void clear (); */
NS_IMETHODIMP nsDictionary::Clear()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDictionary_h__ */
