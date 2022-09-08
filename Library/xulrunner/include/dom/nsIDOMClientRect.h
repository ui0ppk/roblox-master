/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/dom/public/idl/base/nsIDOMClientRect.idl
 */

#ifndef __gen_nsIDOMClientRect_h__
#define __gen_nsIDOMClientRect_h__


#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMClientRect */
#define NS_IDOMCLIENTRECT_IID_STR "f8583bbc-c6de-4646-b39f-df7e766442e9"

#define NS_IDOMCLIENTRECT_IID \
  {0xf8583bbc, 0xc6de, 0x4646, \
    { 0xb3, 0x9f, 0xdf, 0x7e, 0x76, 0x64, 0x42, 0xe9 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMClientRect : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMCLIENTRECT_IID)

  /* readonly attribute float left; */
  NS_SCRIPTABLE NS_IMETHOD GetLeft(float *aLeft) = 0;

  /* readonly attribute float top; */
  NS_SCRIPTABLE NS_IMETHOD GetTop(float *aTop) = 0;

  /* readonly attribute float right; */
  NS_SCRIPTABLE NS_IMETHOD GetRight(float *aRight) = 0;

  /* readonly attribute float bottom; */
  NS_SCRIPTABLE NS_IMETHOD GetBottom(float *aBottom) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMClientRect, NS_IDOMCLIENTRECT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMCLIENTRECT \
  NS_SCRIPTABLE NS_IMETHOD GetLeft(float *aLeft); \
  NS_SCRIPTABLE NS_IMETHOD GetTop(float *aTop); \
  NS_SCRIPTABLE NS_IMETHOD GetRight(float *aRight); \
  NS_SCRIPTABLE NS_IMETHOD GetBottom(float *aBottom); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMCLIENTRECT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLeft(float *aLeft) { return _to GetLeft(aLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetTop(float *aTop) { return _to GetTop(aTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetRight(float *aRight) { return _to GetRight(aRight); } \
  NS_SCRIPTABLE NS_IMETHOD GetBottom(float *aBottom) { return _to GetBottom(aBottom); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMCLIENTRECT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetLeft(float *aLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLeft(aLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetTop(float *aTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetTop(aTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetRight(float *aRight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetRight(aRight); } \
  NS_SCRIPTABLE NS_IMETHOD GetBottom(float *aBottom) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBottom(aBottom); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMClientRect : public nsIDOMClientRect
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMCLIENTRECT

  nsDOMClientRect();

private:
  ~nsDOMClientRect();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMClientRect, nsIDOMClientRect)

nsDOMClientRect::nsDOMClientRect()
{
  /* member initializers and constructor code */
}

nsDOMClientRect::~nsDOMClientRect()
{
  /* destructor code */
}

/* readonly attribute float left; */
NS_IMETHODIMP nsDOMClientRect::GetLeft(float *aLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute float top; */
NS_IMETHODIMP nsDOMClientRect::GetTop(float *aTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute float right; */
NS_IMETHODIMP nsDOMClientRect::GetRight(float *aRight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute float bottom; */
NS_IMETHODIMP nsDOMClientRect::GetBottom(float *aBottom)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMClientRect_h__ */
