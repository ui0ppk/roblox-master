/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/accessible/public/nsPIAccessible.idl
 */

#ifndef __gen_nsPIAccessible_h__
#define __gen_nsPIAccessible_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIAccessible; /* forward declaration */

class nsIAccessibleEvent; /* forward declaration */

   struct nsRoleMapEntry;

/* starting interface:    nsPIAccessible */
#define NS_PIACCESSIBLE_IID_STR "893ee16d-c157-4d5f-b236-60b3b2bef6a5"

#define NS_PIACCESSIBLE_IID \
  {0x893ee16d, 0xc157, 0x4d5f, \
    { 0xb2, 0x36, 0x60, 0xb3, 0xb2, 0xbe, 0xf6, 0xa5 }}

class NS_NO_VTABLE nsPIAccessible : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIACCESSIBLE_IID)

  /**
   * Set accessible parent.
   */
  /* void setParent (in nsIAccessible aAccParent); */
  NS_IMETHOD SetParent(nsIAccessible *aAccParent) = 0;

  /**
   * Set first accessible child.
   */
  /* void setFirstChild (in nsIAccessible aAccFirstChild); */
  NS_IMETHOD SetFirstChild(nsIAccessible *aAccFirstChild) = 0;

  /**
   * Set next sibling accessible.
   */
  /* void setNextSibling (in nsIAccessible aAccNextSibling); */
  NS_IMETHOD SetNextSibling(nsIAccessible *aAccNextSibling) = 0;

  /**
   * Return parent accessible only if cached.
   */
  /* void getCachedParent (out nsIAccessible aAccParent); */
  NS_IMETHOD GetCachedParent(nsIAccessible **aAccParent) = 0;

  /**
   * Return first child accessible only if cached.
   */
  /* void getCachedFirstChild (out nsIAccessible aAccFirstChild); */
  NS_IMETHOD GetCachedFirstChild(nsIAccessible **aAccFirstChild) = 0;

  /**
   * Set the child count to -1 (unknown) and null out cached child pointers
   */
  /* void invalidateChildren (); */
  NS_IMETHOD InvalidateChildren(void) = 0;

  /**
   * Fire accessible event.
   *
   * @param aEvent - DOM event
   * @param aTarget - target of DOM event
   *
   * XXX: eventually this method will be removed (see bug 377022)
   */
  /* void fireToolkitEvent (in unsigned long aEvent, in nsIAccessible aTarget); */
  NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget) = 0;

  /**
   * Fire accessible event.
   */
  /* void fireAccessibleEvent (in nsIAccessibleEvent aAccEvent); */
  NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aAccEvent) = 0;

  /**
   * Return true if there are accessible children in anonymous content
   */
  /* readonly attribute boolean allowsAnonChildAccessibles; */
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildAccessibles) = 0;

  /**
   * Assert if child not in parent's cache.
   */
  /* void testChildCache (in nsIAccessible aCachedChild); */
  NS_IMETHOD TestChildCache(nsIAccessible *aCachedChild) = 0;

  /**
   * Returns text of accessible if accessible has text role otherwise empty
   * string.
   */
  /* void appendTextTo (out AString aString, in unsigned long aStartOffset, in unsigned long aLength); */
  NS_IMETHOD AppendTextTo(nsAString & aString, PRUint32 aStartOffset, PRUint32 aLength) = 0;

  /**
   * Set the ARIA role map entry for a new accessible.
   * For a newly created accessible, specify which role map entry should be used.
   * @param aRoleMapEntry The ARIA nsRoleMapEntry* for the accessible, or 
   *                      nsnull if none.
   */
  /* void setRoleMapEntry (in nsRoleMapEntryPtr aRoleMapEntry); */
  NS_IMETHOD SetRoleMapEntry(nsRoleMapEntry * aRoleMapEntry) = 0;

  /**
   * Maps ARIA state attributes to state of accessible. Note the given state
   * argument should hold states for accessible before you pass it into this
   * method.
   * @param  in/out where to fill the states into.
   */
  /* void getARIAState (out unsigned long aState); */
  NS_IMETHOD GetARIAState(PRUint32 *aState) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsPIAccessible, NS_PIACCESSIBLE_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSPIACCESSIBLE \
  NS_IMETHOD SetParent(nsIAccessible *aAccParent); \
  NS_IMETHOD SetFirstChild(nsIAccessible *aAccFirstChild); \
  NS_IMETHOD SetNextSibling(nsIAccessible *aAccNextSibling); \
  NS_IMETHOD GetCachedParent(nsIAccessible **aAccParent); \
  NS_IMETHOD GetCachedFirstChild(nsIAccessible **aAccFirstChild); \
  NS_IMETHOD InvalidateChildren(void); \
  NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget); \
  NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aAccEvent); \
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildAccessibles); \
  NS_IMETHOD TestChildCache(nsIAccessible *aCachedChild); \
  NS_IMETHOD AppendTextTo(nsAString & aString, PRUint32 aStartOffset, PRUint32 aLength); \
  NS_IMETHOD SetRoleMapEntry(nsRoleMapEntry * aRoleMapEntry); \
  NS_IMETHOD GetARIAState(PRUint32 *aState); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSPIACCESSIBLE(_to) \
  NS_IMETHOD SetParent(nsIAccessible *aAccParent) { return _to SetParent(aAccParent); } \
  NS_IMETHOD SetFirstChild(nsIAccessible *aAccFirstChild) { return _to SetFirstChild(aAccFirstChild); } \
  NS_IMETHOD SetNextSibling(nsIAccessible *aAccNextSibling) { return _to SetNextSibling(aAccNextSibling); } \
  NS_IMETHOD GetCachedParent(nsIAccessible **aAccParent) { return _to GetCachedParent(aAccParent); } \
  NS_IMETHOD GetCachedFirstChild(nsIAccessible **aAccFirstChild) { return _to GetCachedFirstChild(aAccFirstChild); } \
  NS_IMETHOD InvalidateChildren(void) { return _to InvalidateChildren(); } \
  NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget) { return _to FireToolkitEvent(aEvent, aTarget); } \
  NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aAccEvent) { return _to FireAccessibleEvent(aAccEvent); } \
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildAccessibles) { return _to GetAllowsAnonChildAccessibles(aAllowsAnonChildAccessibles); } \
  NS_IMETHOD TestChildCache(nsIAccessible *aCachedChild) { return _to TestChildCache(aCachedChild); } \
  NS_IMETHOD AppendTextTo(nsAString & aString, PRUint32 aStartOffset, PRUint32 aLength) { return _to AppendTextTo(aString, aStartOffset, aLength); } \
  NS_IMETHOD SetRoleMapEntry(nsRoleMapEntry * aRoleMapEntry) { return _to SetRoleMapEntry(aRoleMapEntry); } \
  NS_IMETHOD GetARIAState(PRUint32 *aState) { return _to GetARIAState(aState); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSPIACCESSIBLE(_to) \
  NS_IMETHOD SetParent(nsIAccessible *aAccParent) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetParent(aAccParent); } \
  NS_IMETHOD SetFirstChild(nsIAccessible *aAccFirstChild) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetFirstChild(aAccFirstChild); } \
  NS_IMETHOD SetNextSibling(nsIAccessible *aAccNextSibling) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetNextSibling(aAccNextSibling); } \
  NS_IMETHOD GetCachedParent(nsIAccessible **aAccParent) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCachedParent(aAccParent); } \
  NS_IMETHOD GetCachedFirstChild(nsIAccessible **aAccFirstChild) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCachedFirstChild(aAccFirstChild); } \
  NS_IMETHOD InvalidateChildren(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->InvalidateChildren(); } \
  NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget) { return !_to ? NS_ERROR_NULL_POINTER : _to->FireToolkitEvent(aEvent, aTarget); } \
  NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aAccEvent) { return !_to ? NS_ERROR_NULL_POINTER : _to->FireAccessibleEvent(aAccEvent); } \
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildAccessibles) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAllowsAnonChildAccessibles(aAllowsAnonChildAccessibles); } \
  NS_IMETHOD TestChildCache(nsIAccessible *aCachedChild) { return !_to ? NS_ERROR_NULL_POINTER : _to->TestChildCache(aCachedChild); } \
  NS_IMETHOD AppendTextTo(nsAString & aString, PRUint32 aStartOffset, PRUint32 aLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->AppendTextTo(aString, aStartOffset, aLength); } \
  NS_IMETHOD SetRoleMapEntry(nsRoleMapEntry * aRoleMapEntry) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetRoleMapEntry(aRoleMapEntry); } \
  NS_IMETHOD GetARIAState(PRUint32 *aState) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetARIAState(aState); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class _MYCLASS_ : public nsPIAccessible
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSPIACCESSIBLE

  _MYCLASS_();

private:
  ~_MYCLASS_();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(_MYCLASS_, nsPIAccessible)

_MYCLASS_::_MYCLASS_()
{
  /* member initializers and constructor code */
}

_MYCLASS_::~_MYCLASS_()
{
  /* destructor code */
}

/* void setParent (in nsIAccessible aAccParent); */
NS_IMETHODIMP _MYCLASS_::SetParent(nsIAccessible *aAccParent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setFirstChild (in nsIAccessible aAccFirstChild); */
NS_IMETHODIMP _MYCLASS_::SetFirstChild(nsIAccessible *aAccFirstChild)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setNextSibling (in nsIAccessible aAccNextSibling); */
NS_IMETHODIMP _MYCLASS_::SetNextSibling(nsIAccessible *aAccNextSibling)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getCachedParent (out nsIAccessible aAccParent); */
NS_IMETHODIMP _MYCLASS_::GetCachedParent(nsIAccessible **aAccParent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getCachedFirstChild (out nsIAccessible aAccFirstChild); */
NS_IMETHODIMP _MYCLASS_::GetCachedFirstChild(nsIAccessible **aAccFirstChild)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void invalidateChildren (); */
NS_IMETHODIMP _MYCLASS_::InvalidateChildren()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void fireToolkitEvent (in unsigned long aEvent, in nsIAccessible aTarget); */
NS_IMETHODIMP _MYCLASS_::FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void fireAccessibleEvent (in nsIAccessibleEvent aAccEvent); */
NS_IMETHODIMP _MYCLASS_::FireAccessibleEvent(nsIAccessibleEvent *aAccEvent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean allowsAnonChildAccessibles; */
NS_IMETHODIMP _MYCLASS_::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildAccessibles)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void testChildCache (in nsIAccessible aCachedChild); */
NS_IMETHODIMP _MYCLASS_::TestChildCache(nsIAccessible *aCachedChild)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void appendTextTo (out AString aString, in unsigned long aStartOffset, in unsigned long aLength); */
NS_IMETHODIMP _MYCLASS_::AppendTextTo(nsAString & aString, PRUint32 aStartOffset, PRUint32 aLength)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setRoleMapEntry (in nsRoleMapEntryPtr aRoleMapEntry); */
NS_IMETHODIMP _MYCLASS_::SetRoleMapEntry(nsRoleMapEntry * aRoleMapEntry)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getARIAState (out unsigned long aState); */
NS_IMETHODIMP _MYCLASS_::GetARIAState(PRUint32 *aState)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsPIAccessible_h__ */
