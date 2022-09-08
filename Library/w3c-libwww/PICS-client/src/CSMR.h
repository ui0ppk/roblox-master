/*

					Machine-readable parser for libpics






!Machine-readable parser!

*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module provides the interface to CSMacRed.c. 
Labels are parsed from strings (see CSParse.html).
These labels may then be kept in a CSMachRead_t structure for inspection by the
application.

*/

#ifndef CSMR_H
#define CSMR_H

/*

(State Change Enumeration)

Call to the TargetChangeCallback
will have one of the following values.

*/

typedef enum {
	CSMRTC_MACHREAD = 1, 
	CSMRTC_VERSION, 
	CSMRTC_SYSTEM, 
	CSMRTC_SERVICE, 
	CSMRTC_ICON, 
	CSMSRC_NAME, 
	CSMSRC_VALUE, 
	CSMSRC_DESC, 
	CSMRTC_DEF, 
	CSMRTC_MIN, 
	CSMRTC_MAX, 
	CSMRTC_MULTI, 
	CSMRTC_UNORD, 
	CSMRTC_INT, 
	CSMRTC_LABL, 
	CSMRTC_CAT, 
	CSMRTC_TRANS, 
	CSMRTC_ENUM, 
	CSMRTC_COUNT
} CSMRTC_t;

/*

(Data shell)

All PICS Machine-readable data is stored in a CSMRData_t

*/

typedef struct CSMachReadData_s CSMachReadData_t;

/*

(TargetChangeCallback)

As the label is parsed, it will call the assigned TargetChangeCallback as it
passes from state to state.

*/

typedef StateRet_t MachReadTargetCallback_t(CSMachRead_t * pCSMachRead, 
					    CSParse_t * pCSParse, 
					    CSMRTC_t target, BOOL closed, 
					    void * pVoid);

/*

(ErrorHandler)

All parsing error will be passed to the Apps MRErrorHandler for user display 
or automatic dismissal.

*/

typedef StateRet_t MRErrorHandler_t(CSMachRead_t * pCSMachRead, 
				    CSParse_t * pCSParse, const char * token, 
				    char demark, StateRet_t errorCode);

/*

(Construction/Destruction)

These methods allow the user to create and get access to both the description 
and the state. CSMachReads may be cloned so that one saves state while another 
continues to iterate or parse. The states must all be freed. description data
will only be freed after all the CSMachReads that refer to it are deleted.

*/

extern CSParse_t * CSParse_newMachRead(
			MachReadTargetCallback_t * pMachReadTargetCallback, 
			MRErrorHandler_t * pMRErrorHandler);
extern BOOL CSParse_deleteMachRead(CSParse_t *);
extern CSMachRead_t * CSParse_getMachRead(CSParse_t * me);
extern char * CSMachRead_getSystem(CSMachRead_t * pCSMachRead);
extern char * CSMachRead_getService(CSMachRead_t * pCSMachRead);



/*

(Iterating methods)

(Callback function)

The Iterators are passed a callback function to be called for each matching 
element. For instance, when iterating through ranges, the callback function is
called once for each range, or, if a match is requested, only for the matching
range.

*/



typedef CSError_t CSMachRead_callback_t(CSMachRead_t *, 
						   State_Parms_t *, 
						   const char *, void * pVoid);
typedef CSError_t CSMachRead_iterator_t(CSMachRead_t *, 
						CSMachRead_callback_t *, 
						State_Parms_t *, const char *, 
						void * pVoid);

extern CSMachRead_iterator_t CSMachRead_iterateCategories;
extern CSMachRead_iterator_t CSMachRead_iterateLabels;

/*

*/

#endif /* CSMR_H */

/*

End of Declaration

*/
