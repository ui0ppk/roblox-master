/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/dom/public/idl/html/nsIDOMNSHTMLElement.idl
 */

#ifndef __gen_nsIDOMNSHTMLElement_h__
#define __gen_nsIDOMNSHTMLElement_h__


#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMNSHTMLElement */
#define NS_IDOMNSHTMLELEMENT_IID_STR "eac0a4ee-2e4f-403c-9b77-5cf32cfb42f7"

#define NS_IDOMNSHTMLELEMENT_IID \
  {0xeac0a4ee, 0x2e4f, 0x403c, \
    { 0x9b, 0x77, 0x5c, 0xf3, 0x2c, 0xfb, 0x42, 0xf7 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMNSHTMLElement : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMNSHTMLELEMENT_IID)

  /* readonly attribute long offsetTop; */
  NS_SCRIPTABLE NS_IMETHOD GetOffsetTop(PRInt32 *aOffsetTop) = 0;

  /* readonly attribute long offsetLeft; */
  NS_SCRIPTABLE NS_IMETHOD GetOffsetLeft(PRInt32 *aOffsetLeft) = 0;

  /* readonly attribute long offsetWidth; */
  NS_SCRIPTABLE NS_IMETHOD GetOffsetWidth(PRInt32 *aOffsetWidth) = 0;

  /* readonly attribute long offsetHeight; */
  NS_SCRIPTABLE NS_IMETHOD GetOffsetHeight(PRInt32 *aOffsetHeight) = 0;

  /* readonly attribute nsIDOMElement offsetParent; */
  NS_SCRIPTABLE NS_IMETHOD GetOffsetParent(nsIDOMElement * *aOffsetParent) = 0;

  /* attribute DOMString innerHTML; */
  NS_SCRIPTABLE NS_IMETHOD GetInnerHTML(nsAString & aInnerHTML) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetInnerHTML(const nsAString & aInnerHTML) = 0;

  /* attribute long scrollTop; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop) = 0;

  /* attribute long scrollLeft; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft) = 0;

  /* readonly attribute long scrollHeight; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight) = 0;

  /* readonly attribute long scrollWidth; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth) = 0;

  /* readonly attribute long clientTop; */
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop) = 0;

  /* readonly attribute long clientLeft; */
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft) = 0;

  /* readonly attribute long clientHeight; */
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight) = 0;

  /* readonly attribute long clientWidth; */
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth) = 0;

  /* attribute long tabIndex; */
  NS_SCRIPTABLE NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetTabIndex(PRInt32 aTabIndex) = 0;

  /* attribute DOMString contentEditable; */
  NS_SCRIPTABLE NS_IMETHOD GetContentEditable(nsAString & aContentEditable) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetContentEditable(const nsAString & aContentEditable) = 0;

  /* void blur (); */
  NS_SCRIPTABLE NS_IMETHOD Blur(void) = 0;

  /* void focus (); */
  NS_SCRIPTABLE NS_IMETHOD Focus(void) = 0;

  /* void scrollIntoView (in boolean top); */
  NS_SCRIPTABLE NS_IMETHOD ScrollIntoView(PRBool top) = 0;

  /* attribute boolean spellcheck; */
  NS_SCRIPTABLE NS_IMETHOD GetSpellcheck(PRBool *aSpellcheck) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetSpellcheck(PRBool aSpellcheck) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMNSHTMLElement, NS_IDOMNSHTMLELEMENT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMNSHTMLELEMENT \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetTop(PRInt32 *aOffsetTop); \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetLeft(PRInt32 *aOffsetLeft); \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetWidth(PRInt32 *aOffsetWidth); \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetHeight(PRInt32 *aOffsetHeight); \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetParent(nsIDOMElement * *aOffsetParent); \
  NS_SCRIPTABLE NS_IMETHOD GetInnerHTML(nsAString & aInnerHTML); \
  NS_SCRIPTABLE NS_IMETHOD SetInnerHTML(const nsAString & aInnerHTML); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop); \
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft); \
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth); \
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop); \
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft); \
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight); \
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth); \
  NS_SCRIPTABLE NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex); \
  NS_SCRIPTABLE NS_IMETHOD SetTabIndex(PRInt32 aTabIndex); \
  NS_SCRIPTABLE NS_IMETHOD GetContentEditable(nsAString & aContentEditable); \
  NS_SCRIPTABLE NS_IMETHOD SetContentEditable(const nsAString & aContentEditable); \
  NS_SCRIPTABLE NS_IMETHOD Blur(void); \
  NS_SCRIPTABLE NS_IMETHOD Focus(void); \
  NS_SCRIPTABLE NS_IMETHOD ScrollIntoView(PRBool top); \
  NS_SCRIPTABLE NS_IMETHOD GetSpellcheck(PRBool *aSpellcheck); \
  NS_SCRIPTABLE NS_IMETHOD SetSpellcheck(PRBool aSpellcheck); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMNSHTMLELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetTop(PRInt32 *aOffsetTop) { return _to GetOffsetTop(aOffsetTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetLeft(PRInt32 *aOffsetLeft) { return _to GetOffsetLeft(aOffsetLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetWidth(PRInt32 *aOffsetWidth) { return _to GetOffsetWidth(aOffsetWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetHeight(PRInt32 *aOffsetHeight) { return _to GetOffsetHeight(aOffsetHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetParent(nsIDOMElement * *aOffsetParent) { return _to GetOffsetParent(aOffsetParent); } \
  NS_SCRIPTABLE NS_IMETHOD GetInnerHTML(nsAString & aInnerHTML) { return _to GetInnerHTML(aInnerHTML); } \
  NS_SCRIPTABLE NS_IMETHOD SetInnerHTML(const nsAString & aInnerHTML) { return _to SetInnerHTML(aInnerHTML); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop) { return _to GetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop) { return _to SetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft) { return _to GetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft) { return _to SetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight) { return _to GetScrollHeight(aScrollHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth) { return _to GetScrollWidth(aScrollWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop) { return _to GetClientTop(aClientTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft) { return _to GetClientLeft(aClientLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight) { return _to GetClientHeight(aClientHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth) { return _to GetClientWidth(aClientWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex) { return _to GetTabIndex(aTabIndex); } \
  NS_SCRIPTABLE NS_IMETHOD SetTabIndex(PRInt32 aTabIndex) { return _to SetTabIndex(aTabIndex); } \
  NS_SCRIPTABLE NS_IMETHOD GetContentEditable(nsAString & aContentEditable) { return _to GetContentEditable(aContentEditable); } \
  NS_SCRIPTABLE NS_IMETHOD SetContentEditable(const nsAString & aContentEditable) { return _to SetContentEditable(aContentEditable); } \
  NS_SCRIPTABLE NS_IMETHOD Blur(void) { return _to Blur(); } \
  NS_SCRIPTABLE NS_IMETHOD Focus(void) { return _to Focus(); } \
  NS_SCRIPTABLE NS_IMETHOD ScrollIntoView(PRBool top) { return _to ScrollIntoView(top); } \
  NS_SCRIPTABLE NS_IMETHOD GetSpellcheck(PRBool *aSpellcheck) { return _to GetSpellcheck(aSpellcheck); } \
  NS_SCRIPTABLE NS_IMETHOD SetSpellcheck(PRBool aSpellcheck) { return _to SetSpellcheck(aSpellcheck); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMNSHTMLELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetTop(PRInt32 *aOffsetTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOffsetTop(aOffsetTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetLeft(PRInt32 *aOffsetLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOffsetLeft(aOffsetLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetWidth(PRInt32 *aOffsetWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOffsetWidth(aOffsetWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetHeight(PRInt32 *aOffsetHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOffsetHeight(aOffsetHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetParent(nsIDOMElement * *aOffsetParent) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOffsetParent(aOffsetParent); } \
  NS_SCRIPTABLE NS_IMETHOD GetInnerHTML(nsAString & aInnerHTML) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetInnerHTML(aInnerHTML); } \
  NS_SCRIPTABLE NS_IMETHOD SetInnerHTML(const nsAString & aInnerHTML) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetInnerHTML(aInnerHTML); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollHeight(aScrollHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollWidth(aScrollWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientTop(aClientTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientLeft(aClientLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientHeight(aClientHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientWidth(aClientWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetTabIndex(aTabIndex); } \
  NS_SCRIPTABLE NS_IMETHOD SetTabIndex(PRInt32 aTabIndex) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetTabIndex(aTabIndex); } \
  NS_SCRIPTABLE NS_IMETHOD GetContentEditable(nsAString & aContentEditable) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentEditable(aContentEditable); } \
  NS_SCRIPTABLE NS_IMETHOD SetContentEditable(const nsAString & aContentEditable) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentEditable(aContentEditable); } \
  NS_SCRIPTABLE NS_IMETHOD Blur(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Blur(); } \
  NS_SCRIPTABLE NS_IMETHOD Focus(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Focus(); } \
  NS_SCRIPTABLE NS_IMETHOD ScrollIntoView(PRBool top) { return !_to ? NS_ERROR_NULL_POINTER : _to->ScrollIntoView(top); } \
  NS_SCRIPTABLE NS_IMETHOD GetSpellcheck(PRBool *aSpellcheck) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSpellcheck(aSpellcheck); } \
  NS_SCRIPTABLE NS_IMETHOD SetSpellcheck(PRBool aSpellcheck) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetSpellcheck(aSpellcheck); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMNSHTMLElement : public nsIDOMNSHTMLElement
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNSHTMLELEMENT

  nsDOMNSHTMLElement();

private:
  ~nsDOMNSHTMLElement();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMNSHTMLElement, nsIDOMNSHTMLElement)

nsDOMNSHTMLElement::nsDOMNSHTMLElement()
{
  /* member initializers and constructor code */
}

nsDOMNSHTMLElement::~nsDOMNSHTMLElement()
{
  /* destructor code */
}

/* readonly attribute long offsetTop; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetOffsetTop(PRInt32 *aOffsetTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long offsetLeft; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetOffsetLeft(PRInt32 *aOffsetLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long offsetWidth; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetOffsetWidth(PRInt32 *aOffsetWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long offsetHeight; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetOffsetHeight(PRInt32 *aOffsetHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMElement offsetParent; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetOffsetParent(nsIDOMElement * *aOffsetParent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString innerHTML; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetInnerHTML(nsAString & aInnerHTML)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSHTMLElement::SetInnerHTML(const nsAString & aInnerHTML)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long scrollTop; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetScrollTop(PRInt32 *aScrollTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSHTMLElement::SetScrollTop(PRInt32 aScrollTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long scrollLeft; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetScrollLeft(PRInt32 *aScrollLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSHTMLElement::SetScrollLeft(PRInt32 aScrollLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long scrollHeight; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetScrollHeight(PRInt32 *aScrollHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long scrollWidth; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetScrollWidth(PRInt32 *aScrollWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientTop; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetClientTop(PRInt32 *aClientTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientLeft; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetClientLeft(PRInt32 *aClientLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientHeight; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetClientHeight(PRInt32 *aClientHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientWidth; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetClientWidth(PRInt32 *aClientWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long tabIndex; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetTabIndex(PRInt32 *aTabIndex)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSHTMLElement::SetTabIndex(PRInt32 aTabIndex)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString contentEditable; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetContentEditable(nsAString & aContentEditable)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSHTMLElement::SetContentEditable(const nsAString & aContentEditable)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void blur (); */
NS_IMETHODIMP nsDOMNSHTMLElement::Blur()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void focus (); */
NS_IMETHODIMP nsDOMNSHTMLElement::Focus()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void scrollIntoView (in boolean top); */
NS_IMETHODIMP nsDOMNSHTMLElement::ScrollIntoView(PRBool top)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean spellcheck; */
NS_IMETHODIMP nsDOMNSHTMLElement::GetSpellcheck(PRBool *aSpellcheck)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSHTMLElement::SetSpellcheck(PRBool aSpellcheck)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMNSHTMLElement_h__ */
