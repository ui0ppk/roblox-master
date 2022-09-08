/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/dom/public/idl/offline/nsIDOMLoadStatusList.idl
 */

#ifndef __gen_nsIDOMLoadStatusList_h__
#define __gen_nsIDOMLoadStatusList_h__


#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIDOMLoadStatus; /* forward declaration */


/* starting interface:    nsIDOMLoadStatusList */
#define NS_IDOMLOADSTATUSLIST_IID_STR "d58bc0cf-e35c-4d22-9e21-9ada8fc4203a"

#define NS_IDOMLOADSTATUSLIST_IID \
  {0xd58bc0cf, 0xe35c, 0x4d22, \
    { 0x9e, 0x21, 0x9a, 0xda, 0x8f, 0xc4, 0x20, 0x3a }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMLoadStatusList : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMLOADSTATUSLIST_IID)

  /* readonly attribute unsigned long length; */
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength) = 0;

  /* nsIDOMLoadStatus item (in unsigned long index); */
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMLoadStatusList, NS_IDOMLOADSTATUSLIST_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMLOADSTATUSLIST \
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength); \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMLOADSTATUSLIST(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength) { return _to GetLength(aLength); } \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval) { return _to Item(index, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMLOADSTATUSLIST(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLength(PRUint32 *aLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLength(aLength); } \
  NS_SCRIPTABLE NS_IMETHOD Item(PRUint32 index, nsIDOMLoadStatus **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Item(index, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMLoadStatusList : public nsIDOMLoadStatusList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMLOADSTATUSLIST

  nsDOMLoadStatusList();

private:
  ~nsDOMLoadStatusList();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMLoadStatusList, nsIDOMLoadStatusList)

nsDOMLoadStatusList::nsDOMLoadStatusList()
{
  /* member initializers and constructor code */
}

nsDOMLoadStatusList::~nsDOMLoadStatusList()
{
  /* destructor code */
}

/* readonly attribute unsigned long length; */
NS_IMETHODIMP nsDOMLoadStatusList::GetLength(PRUint32 *aLength)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMLoadStatus item (in unsigned long index); */
NS_IMETHODIMP nsDOMLoadStatusList::Item(PRUint32 index, nsIDOMLoadStatus **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMLoadStatusList_h__ */
