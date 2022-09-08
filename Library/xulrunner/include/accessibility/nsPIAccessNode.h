/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/accessible/public/nsPIAccessNode.idl
 */

#ifndef __gen_nsPIAccessNode_h__
#define __gen_nsPIAccessNode_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIFrame; /* forward declaration */


/* starting interface:    nsPIAccessNode */
#define NS_PIACCESSNODE_IID_STR "b3507946-4a44-4e40-b66c-f23e320997c5"

#define NS_PIACCESSNODE_IID \
  {0xb3507946, 0x4a44, 0x4e40, \
    { 0xb6, 0x6c, 0xf2, 0x3e, 0x32, 0x09, 0x97, 0xc5 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsPIAccessNode : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIACCESSNODE_IID)

  /* [noscript] void init (); */
  NS_IMETHOD Init(void) = 0;

  /* [noscript] void shutdown (); */
  NS_IMETHOD Shutdown(void) = 0;

  /* [notxpcom] nsIFrame GetFrame (); */
  NS_IMETHOD_(nsIFrame *) GetFrame(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsPIAccessNode, NS_PIACCESSNODE_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSPIACCESSNODE \
  NS_IMETHOD Init(void); \
  NS_IMETHOD Shutdown(void); \
  NS_IMETHOD_(nsIFrame *) GetFrame(void); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSPIACCESSNODE(_to) \
  NS_IMETHOD Init(void) { return _to Init(); } \
  NS_IMETHOD Shutdown(void) { return _to Shutdown(); } \
  NS_IMETHOD_(nsIFrame *) GetFrame(void) { return _to GetFrame(); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSPIACCESSNODE(_to) \
  NS_IMETHOD Init(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(); } \
  NS_IMETHOD Shutdown(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Shutdown(); } \
  NS_IMETHOD_(nsIFrame *) GetFrame(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFrame(); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class _MYCLASS_ : public nsPIAccessNode
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSPIACCESSNODE

  _MYCLASS_();

private:
  ~_MYCLASS_();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(_MYCLASS_, nsPIAccessNode)

_MYCLASS_::_MYCLASS_()
{
  /* member initializers and constructor code */
}

_MYCLASS_::~_MYCLASS_()
{
  /* destructor code */
}

/* [noscript] void init (); */
NS_IMETHODIMP _MYCLASS_::Init()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void shutdown (); */
NS_IMETHODIMP _MYCLASS_::Shutdown()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [notxpcom] nsIFrame GetFrame (); */
NS_IMETHODIMP_(nsIFrame *) _MYCLASS_::GetFrame()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsPIAccessNode_h__ */
