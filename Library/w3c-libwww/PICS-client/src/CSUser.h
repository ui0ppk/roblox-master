/*

  					User parser for libpics


!
  User parser
!
*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module provides the interface to CSUser.c. Labels
are parsed from strings (see CSParse.html). These
labels may then be kept in a CSUser_t structure for inspection by the application
or compared to the values in a CSLabel_t structure (see
CSLL.html).
*/

#ifndef CSUSER_H
#define CSUSER_H

/*


(
  State Change Enumeration
)

Call to the
TargetChangeCallback will
have one of the following values.
*/

typedef enum {
	CSUserTC_USER = 1, 
	CSUserTC_SERVICE,
	CSUserTC_RLIST,
	CSUserTC_RATING,
	CSUserTC_RANGE,
	CSUserTC_COUNT
} CSUserTC_t;

/*
(
  Data shell
)

All PICS user data is stored in a CSUserData_t
*/

typedef struct CSUserData_s CSUserData_t;

/*
(
  TargetChangeCallback
)

As the label is parsed, it will call the assigned TargetChangeCallback as
it passes from state to state.
*/

typedef StateRet_t UserTargetCallback_t(CSUser_t * pCSUser, 
				  CSParse_t * pCSParse, 
				  CSUserTC_t target, BOOL closed, 
				  void * pVoid);

/*
(
  ErrorHandler
)

All parsing error will be passed to the Apps UserErrorHandler for user display
or automatic dismissal.
*/

typedef StateRet_t UserErrorHandler_t(CSUser_t * pCSUser, CSParse_t * pCSParse,
 				      const char * token, char demark, 
				      StateRet_t errorCode);

/*
(
  Construction/Destruction
)

These methods allow the user to create and get access to both the user and
the state. CSUsers may be cloned so that one saves state while another continues
to iterate or parse. The states mus all be freed. User data will only be
freed after all the CSUsers that refer to it are deleted.
*/

extern CSParse_t * CSParse_newUser(void);
extern BOOL CSParse_deleteUser(CSParse_t *);
extern CSUser_t * CSParse_getUser(CSParse_t * me);
extern char * CSUser_name(CSUser_t * pCSUser);
extern BOOL CSUser_checkPassword(CSUser_t * pCSUser, char * password);
extern char * CSUser_bureau(CSUser_t * pCSUser);
extern void CSUser_free(CSUser_t * me);
extern Range_t * CSUser_getUserRatingRange(CSUser_t * pCSUser);
extern char * CSUser_getRatingStr(CSUser_t * pCSUser);

/*
(
  Iterating methods
)
(
  Callback function
)

The Iterators are passed a callback function to be called for each matching
element. For instance, when iterating through ranges, the callback function
is called once for each range, or, if a match is requested, only for the
matching range.
*/

typedef CSError_t CSUser_callback_t(CSUser_t *, 
			            State_Parms_t *, const char *, 
				    void * pVoid);
typedef CSError_t CSUser_iterator_t(CSUser_t *, 
				    CSUser_callback_t *, 
				    State_Parms_t *, const char *, 
				    void * pVoid);

extern CSUser_iterator_t CSUser_iterateServices;
extern CSUser_iterator_t CSUser_iterateServiceRatings;

extern CSError_t CSCheckLabel_checkLabelAndUser(CSLabel_t * pCSLabel, 
						CSUser_t * pCSUser);
extern CSError_t CSCheckLabel_parseAndValidateLabelStr(const char * label, 
						       CSUser_t * pCSUser);

/*


.
  CSUser_acceptLabels
.

get a malloced HTTP Protocol-Request string requesting PICS labels for all
services defined for pCSUser
*/

extern char * CSUser_acceptLabels(CSUser_t * pCSUser, 
				  CSCompleteness_t completeness);

/*


.
  CSUser_getLabels
.

get a malloced HTTP GET string requesting PICS labels for all services defined
for a user
*/

extern char * CSUser_getLabels(CSUser_t * pCSUser, char * url, 
			       CSOption_t option, 
			       CSCompleteness_t completeness);

/*


.
  CSUser_postLabels
.

get a malloced HTTP POST string requesting PICS labels for all services defined
for a user
*/

extern char * CSUser_postLabels(CSUser_t * pCSUser, char * url, 
				CSOption_t option, 
				CSCompleteness_t completeness);

/*
*/

#endif /* CSUser_H */

/*

End of Declaration
*/
