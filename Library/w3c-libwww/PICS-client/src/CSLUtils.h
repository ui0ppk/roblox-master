/*

					PICS library utilities






!PICS library utilities!

*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module defines the PICS library interface. 
*/

#ifndef CSLUTILS_H
#define CSLUTILS_H

/*

*/

#include "HTUtils.h"
#include "HTList.h"

/*

!Primitave Data Structures!


BVal_t, FVal_t, SVal_t, DVal_t - hold a boolean, float (not double),
string, or date value (respectively).  These data structures are designed so
that they may be initialized to all 0s (and hence included directly within  
larger structures, rather than allocated and initialized individually).  You
must, however, call their clear method to deallocate any additional memory  
used to store the actual value once they have been initialized.  The
following methods are defined on all four data types ("X" should be either
"B" "F" "S" or "D", XType is "BOOL" "float" "char *" or "char *", 
respectively):
	 
	 o BOOL XVal_readVal(XVal_t, char *), etc. - convert the string to a value
	 of the specified type.  Returns TRUE on success, FALSE on failure.  If
	 successful, may allocate additional storage.
	 o BOOL XVal_initialized(XVal_t) - Returns TRUE if the value has been  
	 initialized (hence contains a legitimate value and may have additional
	 storage allocated internally), FALSE otherwise.
	 o XType XVal_value(XVal_t) -- Returns the value stored in the object.
	 o void XVal_clear(XVal_t) -- Mark the object as uninitialized and release 
	 any memory associated with the value currently stored in the object.
	 
	 
.BVal.
- Boolean value. 
(definition)

*/

typedef struct {
    enum {BVal_UNINITIALIZED = 0,BVal_YES = 1, BVal_INITIALIZED = 2} state;
    } BVal_t;

extern BOOL BVal_readVal(BVal_t * pBVal, const char * valueStr);
extern BOOL BVal_initialized(const BVal_t * pBVal);
extern BOOL BVal_value(const BVal_t * pBVal);
extern void BVal_clear(BVal_t * pBVal);

/*
(additional methods)
	 
	 o void set - assign value
	 
	 */

extern void BVal_set(BVal_t * pBVal, BOOL value);

/*
.FVal.
- Float value with negative and positive infinity values
(definition)

*/

typedef struct {
    float value;
    enum {FVal_UNINITIALIZED = 0, FVal_VALUE = 1, FVal_NEGATIVE_INF = 2, 
	  FVal_POSITIVE_INF = 3} stat;
    } FVal_t;

extern BOOL FVal_readVal(FVal_t * pFVal, const char * valueStr);
extern BOOL FVal_initialized(const FVal_t * pFVal);
extern float FVal_value(const FVal_t * pFVal);
extern void FVal_clear(FVal_t * pFVal);

/*
(additional methods)
	 
	 o void set - assign a float value
	 o void setInfinite - set to negative or positive infinity
	 o BOOL isZero - see if value is zero
	 o int isInfinite - -1 or 1 for negative or positive infinity
	 o BOOL nearerZero - see if check is nearer zero than check
	 o FVal_t FVal_minus - subtract small from big
	 o char * FVal_toStr - convert to allocated CString, caller must free
	 
	 */

extern void FVal_set(FVal_t * pFVal, float value);
extern void FVal_setInfinite(FVal_t * pFVal, BOOL negative);
extern BOOL FVal_isZero(const FVal_t * pFVal);
extern int FVal_isInfinite(const FVal_t * pFVal);
extern BOOL FVal_nearerZero(const FVal_t * pRef, const FVal_t * pCheck);
extern FVal_t FVal_minus(const FVal_t * pBig, const FVal_t * pSmall);
extern char * FVal_toStr(FVal_t * pFVal);

/*
(initializers)
FVal intializers may be used when creating an FVal
eg. FVal_t localFVal = FVal_NEGATIVE_INF;
*/

#define FVal_NEW_UNINITIALIZED {(float) 0.0, FVal_UNINITIALIZED}
#define FVal_NEW_NEGATIVE_INF {(float) 0.0, FVal_NEGATIVE_INF}
#define FVal_NEW_POSITIVE_INF {(float) 0.0, FVal_POSITIVE_INF}
#define FVal_NEW_ZERO {(float) 0.0, FVal_VALUE}


/*
.SVal.
- String value. 
(definition)

*/

typedef struct {
    char * value;
    BOOL initialized;
    } SVal_t;

extern BOOL SVal_readVal(SVal_t * pSVal, const char * valueStr);
extern BOOL SVal_initialized(const SVal_t * pSVal);
extern char * SVal_value(const SVal_t * pSVal);
extern void SVal_clear(SVal_t * pSVal);

/*
.DVal.
- Date value. 
(definition)

*/

typedef struct {
    char * value; /* keep the string around for debugging and output */
    BOOL initialized;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int timeZoneHours;
    int timeZoneMinutes;
    } DVal_t;

extern BOOL DVal_readVal(DVal_t * pDVal, const char * valueStr);
extern BOOL DVal_initialized(const DVal_t * pDVal);
extern char * DVal_value(const DVal_t * pDVal);
extern void DVal_clear(DVal_t * pDVal);

/*
(additional methods)
	 
	 o int compare - -1 or 1 for a before or after b, 0 for equivilence
	 
	 */

extern int DVal_compare(const DVal_t * a, const DVal_t * b);

/*
.Range.
- Range of FVals. 
(definition)

*/

typedef struct {
    FVal_t min;
    FVal_t max;
    } Range_t;

/*
(methods)
	 
	 o rangeToStr - print range to malloced string. This string must be freed 
	 by caller
	 o gap - find the difference between a and b
	 
	 
	 */

extern char * Range_toStr(Range_t * pRange);
extern FVal_t Range_gap(Range_t * a, Range_t * b);

/*
(initializers)

*/

#define Range_NEW_UNINITIALIZED {FVal_NEW_UNINITIALIZED, \
				 FVal_NEW_UNINITIALIZED}


/*
!Parser!

.CSParse_parseChunk.
CSParse_t - ephemeral parser data, the 
CSParse structure is defined in
CSParse.html.
CSDoMore_t - tells caller whether parseChunk expects more or encountered an 
error

*/

typedef struct CSParse_s CSParse_t;
typedef enum {CSDoMore_more, CSDoMore_done, CSDoMore_error} CSDoMore_t;
extern CSDoMore_t CSParse_parseChunk (CSParse_t * pCSParse, const char * ptr, 
				      int len, void * pVoid);

/*
!Parse callbacks!
During parsing, the parser makes callbacks to tell the caller that an error 
has been encountered or that the parser is reading into a new data structure.

.CSParseTC.
The TC, or TargetChange, type is a way of itemizing the different targets in
a parsable object. It is used in the 
TargetChangeCallback

*/

typedef unsigned int CSParseTC_t;

/*
.StateRet.
*/

typedef enum {StateRet_OK = 0, StateRet_DONE = 1, StateRet_WARN = 0x10, 
	      StateRet_WARN_NO_MATCH = 0x11, StateRet_WARN_BAD_PUNCT = 0x12, 
	      StateRet_ERROR = 0x100, StateRet_ERROR_BAD_CHAR = 0x101
} StateRet_t;


/*

.TargetChangeCallback.
These callbacks keep the caller abreast of what type of object the parser is 
currently reading. TargetChangeCallbacks are made whenever the parser starts 
or finishes reading one of these objects. The actual values of targetChange, 
and what objects they correlate to, can be found in the modules for the object 
being parsed.
	 
	 o CSLL.html for PICS labels.
o CSMR.html for machine-readable 
service descriptions.
o CSUser.html for PICS user 
profiles.

	 
	 
	 Example: When reading a CSLabel, the callback will be called 
	 with pTargetObject = CSLLTC_SERVICE when reading a service, CSLLTC_LABEL when
	 reading a label, etc.
	 */

typedef struct TargetObject_s TargetObject_t;
typedef StateRet_t TargetChangeCallback_t(CSParse_t * pCSParse, 
					 TargetObject_t * pTargetObject, 
					 CSParseTC_t targetChange, BOOL closed,
					 void * pVoid);

/*
.ParseErrorHandler.

*/

typedef StateRet_t ParseErrorHandler_t(CSParse_t * pCSParse, 
				       const char * token, 
				       char demark, StateRet_t errorCode);


/*
.CSList_acceptLabels.
get a malloced HTTP Protocol-Request string requesting PICS labels for all 
services in pServiceList
*/

typedef enum {CSCompleteness_minimal, CSCompleteness_short, 
	      CSCompleteness_full, CSCompleteness_signed} CSCompleteness_t;
extern char * CSList_acceptLabels(HTList * pServiceList, 
				  CSCompleteness_t completeness);


/*
.CSList_getLabels.
get a malloced HTTP GET string requesting PICS labels for all services 
in pServiceList
*/

typedef enum {CSOption_generic, CSOption_normal, CSOption_tree, 
	      CSOption_genericTree} CSOption_t;
extern char * CSList_getLabels(HTList * pServiceList, CSOption_t option, 
			       CSCompleteness_t completeness);

/*
.CSList_postLabels.
get a malloced HTTP GET string requesting PICS labels for all services 
in pServiceList
*/

extern char * CSList_postLabels(HTList * pServiceList, char * url, 
				CSOption_t option, 
				CSCompleteness_t completeness);

/*
.individual parsers.
.CSLabel.
PICS label list
*/

typedef struct CSLabel_s CSLabel_t;

/*

.CSUser.
PICS user profile
*/

typedef struct CSUser_s CSUser_t;

/*

.CSMachRead.
PICS machine readable system description
*/

typedef struct CSMachRead_s CSMachRead_t;

/*

for reading label error codes
*/


typedef enum {
    labelError_NA = 0, 
    labelError_NO_RATINGS, 
    labelError_UNAVAILABLE, 
    labelError_DENIED, 
    labelError_NOT_LABELED,
    labelError_UNKNOWN
    } LabelErrorCode_t;


/*
State_Parms - obsolete parameter exchange for iterators
*/

typedef struct State_Parms_s State_Parms_t;

typedef enum {
    CSError_OK = 0, 
    CSError_YES = 0, 
    CSError_NO = 1, 
    CSError_BUREAU_NONE, 
    CSError_RATING_VALUE, 
    CSError_RATING_RANGE, 
    CSError_RATING_MISSING, 
    CSError_SINGLELABEL_MISSING, 
    CSError_LABEL_MISSING, 
    CSError_SERVICE_MISSING, 
    CSError_CATEGORY_MISSING, 
    CSError_ENUM_MISSING, 
    CSError_BAD_PARAM, 
    CSError_BAD_DATE, 
    CSError_SERVICE_NONE, 
    CSError_RATING_NONE, 
    CSError_APP
    } CSError_t;

/*

*/

#endif /* CSLUTILS_H */

/*

End of Declaration

*/
