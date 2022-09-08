/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/modules/plugin/base/public/nsIPluginDocument.idl
 */

#ifndef __gen_nsIPluginDocument_h__
#define __gen_nsIPluginDocument_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsIStreamListener_h__
#include "nsIStreamListener.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIPluginDocument */
#define NS_IPLUGINDOCUMENT_IID_STR "0d8129f1-5a55-4eaa-851f-15e43ce3d183"

#define NS_IPLUGINDOCUMENT_IID \
  {0x0d8129f1, 0x5a55, 0x4eaa, \
    { 0x85, 0x1f, 0x15, 0xe4, 0x3c, 0xe3, 0xd1, 0x83 }}

class NS_NO_VTABLE nsIPluginDocument : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGINDOCUMENT_IID)

  /**
  * Sets the stream listener for this plugin document 
  */
  /* void setStreamListener (in nsIStreamListener aStreamListener); */
  NS_IMETHOD SetStreamListener(nsIStreamListener *aStreamListener) = 0;

  /**
  * Causes the plugin to print in full-page mode
  */
  /* void print (); */
  NS_IMETHOD Print(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginDocument, NS_IPLUGINDOCUMENT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPLUGINDOCUMENT \
  NS_IMETHOD SetStreamListener(nsIStreamListener *aStreamListener); \
  NS_IMETHOD Print(void); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPLUGINDOCUMENT(_to) \
  NS_IMETHOD SetStreamListener(nsIStreamListener *aStreamListener) { return _to SetStreamListener(aStreamListener); } \
  NS_IMETHOD Print(void) { return _to Print(); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPLUGINDOCUMENT(_to) \
  NS_IMETHOD SetStreamListener(nsIStreamListener *aStreamListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetStreamListener(aStreamListener); } \
  NS_IMETHOD Print(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Print(); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsPluginDocument : public nsIPluginDocument
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINDOCUMENT

  nsPluginDocument();

private:
  ~nsPluginDocument();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPluginDocument, nsIPluginDocument)

nsPluginDocument::nsPluginDocument()
{
  /* member initializers and constructor code */
}

nsPluginDocument::~nsPluginDocument()
{
  /* destructor code */
}

/* void setStreamListener (in nsIStreamListener aStreamListener); */
NS_IMETHODIMP nsPluginDocument::SetStreamListener(nsIStreamListener *aStreamListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void print (); */
NS_IMETHODIMP nsPluginDocument::Print()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIPluginDocument_h__ */
