/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM e:/xr19rel/WINNT_5.2_Depend/mozilla/editor/txtsvc/public/nsIInlineSpellChecker.idl
 */

#ifndef __gen_nsIInlineSpellChecker_h__
#define __gen_nsIInlineSpellChecker_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsISelection; /* forward declaration */

class nsIEditor; /* forward declaration */

class nsIEditorSpellCheck; /* forward declaration */


/* starting interface:    nsIInlineSpellChecker */
#define NS_IINLINESPELLCHECKER_IID_STR "f5d1ec9e-4d30-11d8-8053-da0cc7df1f20"

#define NS_IINLINESPELLCHECKER_IID \
  {0xf5d1ec9e, 0x4d30, 0x11d8, \
    { 0x80, 0x53, 0xda, 0x0c, 0xc7, 0xdf, 0x1f, 0x20 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIInlineSpellChecker : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IINLINESPELLCHECKER_IID)

  /* readonly attribute nsIEditorSpellCheck spellChecker; */
  NS_SCRIPTABLE NS_IMETHOD GetSpellChecker(nsIEditorSpellCheck * *aSpellChecker) = 0;

  /* [noscript] void init (in nsIEditor aEditor); */
  NS_IMETHOD Init(nsIEditor *aEditor) = 0;

  /* [noscript] void cleanup (); */
  NS_IMETHOD Cleanup(void) = 0;

  /* attribute boolean enableRealTimeSpell; */
  NS_SCRIPTABLE NS_IMETHOD GetEnableRealTimeSpell(PRBool *aEnableRealTimeSpell) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetEnableRealTimeSpell(PRBool aEnableRealTimeSpell) = 0;

  /* void spellCheckAfterEditorChange (in long aAction, in nsISelection aSelection, in nsIDOMNode aPreviousSelectedNode, in long aPreviousSelectedOffset, in nsIDOMNode aStartNode, in long aStartOffset, in nsIDOMNode aEndNode, in long aEndOffset); */
  NS_SCRIPTABLE NS_IMETHOD SpellCheckAfterEditorChange(PRInt32 aAction, nsISelection *aSelection, nsIDOMNode *aPreviousSelectedNode, PRInt32 aPreviousSelectedOffset, nsIDOMNode *aStartNode, PRInt32 aStartOffset, nsIDOMNode *aEndNode, PRInt32 aEndOffset) = 0;

  /* void spellCheckRange (in nsIDOMRange aSelection); */
  NS_SCRIPTABLE NS_IMETHOD SpellCheckRange(nsIDOMRange *aSelection) = 0;

  /* nsIDOMRange getMispelledWord (in nsIDOMNode aNode, in long aOffset); */
  NS_SCRIPTABLE NS_IMETHOD GetMispelledWord(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMRange **_retval) = 0;

  /* void replaceWord (in nsIDOMNode aNode, in long aOffset, in AString aNewword); */
  NS_SCRIPTABLE NS_IMETHOD ReplaceWord(nsIDOMNode *aNode, PRInt32 aOffset, const nsAString & aNewword) = 0;

  /* void addWordToDictionary (in AString aWord); */
  NS_SCRIPTABLE NS_IMETHOD AddWordToDictionary(const nsAString & aWord) = 0;

  /* void ignoreWord (in AString aWord); */
  NS_SCRIPTABLE NS_IMETHOD IgnoreWord(const nsAString & aWord) = 0;

  /* void ignoreWords ([array, size_is (aCount)] in wstring aWordsToIgnore, in unsigned long aCount); */
  NS_SCRIPTABLE NS_IMETHOD IgnoreWords(const PRUnichar **aWordsToIgnore, PRUint32 aCount) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIInlineSpellChecker, NS_IINLINESPELLCHECKER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIINLINESPELLCHECKER \
  NS_SCRIPTABLE NS_IMETHOD GetSpellChecker(nsIEditorSpellCheck * *aSpellChecker); \
  NS_IMETHOD Init(nsIEditor *aEditor); \
  NS_IMETHOD Cleanup(void); \
  NS_SCRIPTABLE NS_IMETHOD GetEnableRealTimeSpell(PRBool *aEnableRealTimeSpell); \
  NS_SCRIPTABLE NS_IMETHOD SetEnableRealTimeSpell(PRBool aEnableRealTimeSpell); \
  NS_SCRIPTABLE NS_IMETHOD SpellCheckAfterEditorChange(PRInt32 aAction, nsISelection *aSelection, nsIDOMNode *aPreviousSelectedNode, PRInt32 aPreviousSelectedOffset, nsIDOMNode *aStartNode, PRInt32 aStartOffset, nsIDOMNode *aEndNode, PRInt32 aEndOffset); \
  NS_SCRIPTABLE NS_IMETHOD SpellCheckRange(nsIDOMRange *aSelection); \
  NS_SCRIPTABLE NS_IMETHOD GetMispelledWord(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMRange **_retval); \
  NS_SCRIPTABLE NS_IMETHOD ReplaceWord(nsIDOMNode *aNode, PRInt32 aOffset, const nsAString & aNewword); \
  NS_SCRIPTABLE NS_IMETHOD AddWordToDictionary(const nsAString & aWord); \
  NS_SCRIPTABLE NS_IMETHOD IgnoreWord(const nsAString & aWord); \
  NS_SCRIPTABLE NS_IMETHOD IgnoreWords(const PRUnichar **aWordsToIgnore, PRUint32 aCount); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIINLINESPELLCHECKER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetSpellChecker(nsIEditorSpellCheck * *aSpellChecker) { return _to GetSpellChecker(aSpellChecker); } \
  NS_IMETHOD Init(nsIEditor *aEditor) { return _to Init(aEditor); } \
  NS_IMETHOD Cleanup(void) { return _to Cleanup(); } \
  NS_SCRIPTABLE NS_IMETHOD GetEnableRealTimeSpell(PRBool *aEnableRealTimeSpell) { return _to GetEnableRealTimeSpell(aEnableRealTimeSpell); } \
  NS_SCRIPTABLE NS_IMETHOD SetEnableRealTimeSpell(PRBool aEnableRealTimeSpell) { return _to SetEnableRealTimeSpell(aEnableRealTimeSpell); } \
  NS_SCRIPTABLE NS_IMETHOD SpellCheckAfterEditorChange(PRInt32 aAction, nsISelection *aSelection, nsIDOMNode *aPreviousSelectedNode, PRInt32 aPreviousSelectedOffset, nsIDOMNode *aStartNode, PRInt32 aStartOffset, nsIDOMNode *aEndNode, PRInt32 aEndOffset) { return _to SpellCheckAfterEditorChange(aAction, aSelection, aPreviousSelectedNode, aPreviousSelectedOffset, aStartNode, aStartOffset, aEndNode, aEndOffset); } \
  NS_SCRIPTABLE NS_IMETHOD SpellCheckRange(nsIDOMRange *aSelection) { return _to SpellCheckRange(aSelection); } \
  NS_SCRIPTABLE NS_IMETHOD GetMispelledWord(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMRange **_retval) { return _to GetMispelledWord(aNode, aOffset, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ReplaceWord(nsIDOMNode *aNode, PRInt32 aOffset, const nsAString & aNewword) { return _to ReplaceWord(aNode, aOffset, aNewword); } \
  NS_SCRIPTABLE NS_IMETHOD AddWordToDictionary(const nsAString & aWord) { return _to AddWordToDictionary(aWord); } \
  NS_SCRIPTABLE NS_IMETHOD IgnoreWord(const nsAString & aWord) { return _to IgnoreWord(aWord); } \
  NS_SCRIPTABLE NS_IMETHOD IgnoreWords(const PRUnichar **aWordsToIgnore, PRUint32 aCount) { return _to IgnoreWords(aWordsToIgnore, aCount); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIINLINESPELLCHECKER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetSpellChecker(nsIEditorSpellCheck * *aSpellChecker) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSpellChecker(aSpellChecker); } \
  NS_IMETHOD Init(nsIEditor *aEditor) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(aEditor); } \
  NS_IMETHOD Cleanup(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Cleanup(); } \
  NS_SCRIPTABLE NS_IMETHOD GetEnableRealTimeSpell(PRBool *aEnableRealTimeSpell) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetEnableRealTimeSpell(aEnableRealTimeSpell); } \
  NS_SCRIPTABLE NS_IMETHOD SetEnableRealTimeSpell(PRBool aEnableRealTimeSpell) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetEnableRealTimeSpell(aEnableRealTimeSpell); } \
  NS_SCRIPTABLE NS_IMETHOD SpellCheckAfterEditorChange(PRInt32 aAction, nsISelection *aSelection, nsIDOMNode *aPreviousSelectedNode, PRInt32 aPreviousSelectedOffset, nsIDOMNode *aStartNode, PRInt32 aStartOffset, nsIDOMNode *aEndNode, PRInt32 aEndOffset) { return !_to ? NS_ERROR_NULL_POINTER : _to->SpellCheckAfterEditorChange(aAction, aSelection, aPreviousSelectedNode, aPreviousSelectedOffset, aStartNode, aStartOffset, aEndNode, aEndOffset); } \
  NS_SCRIPTABLE NS_IMETHOD SpellCheckRange(nsIDOMRange *aSelection) { return !_to ? NS_ERROR_NULL_POINTER : _to->SpellCheckRange(aSelection); } \
  NS_SCRIPTABLE NS_IMETHOD GetMispelledWord(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMRange **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetMispelledWord(aNode, aOffset, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD ReplaceWord(nsIDOMNode *aNode, PRInt32 aOffset, const nsAString & aNewword) { return !_to ? NS_ERROR_NULL_POINTER : _to->ReplaceWord(aNode, aOffset, aNewword); } \
  NS_SCRIPTABLE NS_IMETHOD AddWordToDictionary(const nsAString & aWord) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddWordToDictionary(aWord); } \
  NS_SCRIPTABLE NS_IMETHOD IgnoreWord(const nsAString & aWord) { return !_to ? NS_ERROR_NULL_POINTER : _to->IgnoreWord(aWord); } \
  NS_SCRIPTABLE NS_IMETHOD IgnoreWords(const PRUnichar **aWordsToIgnore, PRUint32 aCount) { return !_to ? NS_ERROR_NULL_POINTER : _to->IgnoreWords(aWordsToIgnore, aCount); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsInlineSpellChecker : public nsIInlineSpellChecker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINLINESPELLCHECKER

  nsInlineSpellChecker();

private:
  ~nsInlineSpellChecker();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsInlineSpellChecker, nsIInlineSpellChecker)

nsInlineSpellChecker::nsInlineSpellChecker()
{
  /* member initializers and constructor code */
}

nsInlineSpellChecker::~nsInlineSpellChecker()
{
  /* destructor code */
}

/* readonly attribute nsIEditorSpellCheck spellChecker; */
NS_IMETHODIMP nsInlineSpellChecker::GetSpellChecker(nsIEditorSpellCheck * *aSpellChecker)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void init (in nsIEditor aEditor); */
NS_IMETHODIMP nsInlineSpellChecker::Init(nsIEditor *aEditor)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void cleanup (); */
NS_IMETHODIMP nsInlineSpellChecker::Cleanup()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean enableRealTimeSpell; */
NS_IMETHODIMP nsInlineSpellChecker::GetEnableRealTimeSpell(PRBool *aEnableRealTimeSpell)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsInlineSpellChecker::SetEnableRealTimeSpell(PRBool aEnableRealTimeSpell)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void spellCheckAfterEditorChange (in long aAction, in nsISelection aSelection, in nsIDOMNode aPreviousSelectedNode, in long aPreviousSelectedOffset, in nsIDOMNode aStartNode, in long aStartOffset, in nsIDOMNode aEndNode, in long aEndOffset); */
NS_IMETHODIMP nsInlineSpellChecker::SpellCheckAfterEditorChange(PRInt32 aAction, nsISelection *aSelection, nsIDOMNode *aPreviousSelectedNode, PRInt32 aPreviousSelectedOffset, nsIDOMNode *aStartNode, PRInt32 aStartOffset, nsIDOMNode *aEndNode, PRInt32 aEndOffset)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void spellCheckRange (in nsIDOMRange aSelection); */
NS_IMETHODIMP nsInlineSpellChecker::SpellCheckRange(nsIDOMRange *aSelection)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMRange getMispelledWord (in nsIDOMNode aNode, in long aOffset); */
NS_IMETHODIMP nsInlineSpellChecker::GetMispelledWord(nsIDOMNode *aNode, PRInt32 aOffset, nsIDOMRange **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void replaceWord (in nsIDOMNode aNode, in long aOffset, in AString aNewword); */
NS_IMETHODIMP nsInlineSpellChecker::ReplaceWord(nsIDOMNode *aNode, PRInt32 aOffset, const nsAString & aNewword)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addWordToDictionary (in AString aWord); */
NS_IMETHODIMP nsInlineSpellChecker::AddWordToDictionary(const nsAString & aWord)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void ignoreWord (in AString aWord); */
NS_IMETHODIMP nsInlineSpellChecker::IgnoreWord(const nsAString & aWord)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void ignoreWords ([array, size_is (aCount)] in wstring aWordsToIgnore, in unsigned long aCount); */
NS_IMETHODIMP nsInlineSpellChecker::IgnoreWords(const PRUnichar **aWordsToIgnore, PRUint32 aCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif

#define MOZ_INLINESPELLCHECKER_CONTRACTID "@mozilla.org/spellchecker-inline;1"

#endif /* __gen_nsIInlineSpellChecker_h__ */
