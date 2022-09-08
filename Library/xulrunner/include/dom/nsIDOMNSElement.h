/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/dom/public/idl/core/nsIDOMNSElement.idl
 */

#ifndef __gen_nsIDOMNSElement_h__
#define __gen_nsIDOMNSElement_h__


#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIDOMNodeList; /* forward declaration */


/* starting interface:    nsIDOMNSElement */
#define NS_IDOMNSELEMENT_IID_STR "cea6f919-7fe6-4bdd-9db6-158d9283f8d3"

#define NS_IDOMNSELEMENT_IID \
  {0xcea6f919, 0x7fe6, 0x4bdd, \
    { 0x9d, 0xb6, 0x15, 0x8d, 0x92, 0x83, 0xf8, 0xd3 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMNSElement : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMNSELEMENT_IID)

  /* nsIDOMNodeList getElementsByClassName (in DOMString classes); */
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval) = 0;

  /* nsIDOMClientRectList getClientRects (); */
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval) = 0;

  /**
   * Returns the union of all rectangles in the getClientRects() list. Empty
   * rectangles are ignored, except that if all rectangles are empty,
   * we return an empty rectangle positioned at the top-left of the first
   * rectangle in getClientRects().
   */
  /* nsIDOMClientRect getBoundingClientRect (); */
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMNSElement, NS_IDOMNSELEMENT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMNSELEMENT \
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval); \
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval); \
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMNSELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval) { return _to GetElementsByClassName(classes, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval) { return _to GetClientRects(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval) { return _to GetBoundingClientRect(_retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMNSELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetElementsByClassName(classes, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientRects(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBoundingClientRect(_retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMNSElement : public nsIDOMNSElement
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNSELEMENT

  nsDOMNSElement();

private:
  ~nsDOMNSElement();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMNSElement, nsIDOMNSElement)

nsDOMNSElement::nsDOMNSElement()
{
  /* member initializers and constructor code */
}

nsDOMNSElement::~nsDOMNSElement()
{
  /* destructor code */
}

/* nsIDOMNodeList getElementsByClassName (in DOMString classes); */
NS_IMETHODIMP nsDOMNSElement::GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMClientRectList getClientRects (); */
NS_IMETHODIMP nsDOMNSElement::GetClientRects(nsIDOMClientRectList **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMClientRect getBoundingClientRect (); */
NS_IMETHODIMP nsDOMNSElement::GetBoundingClientRect(nsIDOMClientRect **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMNSElement_h__ */
