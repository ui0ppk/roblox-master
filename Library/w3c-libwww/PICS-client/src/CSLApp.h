/*

  					PICS library utilities


!
  PICS library utilities
!
*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*
*/

#ifndef CSLAPP_H
#define CSLAPP_H
#include "WWWCore.h"
#include "CSLUtils.h"

/*
(
  CSApp_error
)

Return codes for those functions which do not return a pointer.
*/

/*
typedef enum {
  CSApp_OK, CSCSApp_done, CSApp_memory, CSApp_file, CSApp_badHandle, 
  CSApp_inconsistentParms, CSApp_badPassword, CSApp_internal, CSApp_allowed, 
  CSApp_denied
} CSApp_error;
*/

/*
(
  CSDisposition_callback
)

- function to be called by the when Pics receives ratings for a requested
document. The callback is called afer the PICS library has decided whether
the user should be permitted access to the document.

CSDisposition_criteria - when to call the app

pReq - HTRequest which told libwww to load the document

disposition - CSApp_OK if user should see document, CSApp_denied otherwise

pVoid - void pointer passed to CSApp_registerApp
 or CSApp_registerReq.
*/

typedef enum {
  CSApp_neverCall = 0, CSApp_callOnBad = 1, CSApp_callOnGood = 2
} CSDisposition_criteria;
typedef CSError_t (CSDisposition_callback)(HTRequest* pReq, CSLabel_t * pCSLabel, 
				   CSUser_t * pCSUser, CSError_t disposition, 
				   void * pVoid);

/*

CSApp.c maintains a list of LoadedUsers. More than
one may be loaded at a time as different requests may be associated with
different users.
*/

extern CSUser_t * CSLoadedUser_load(char * url, char * relatedName);
extern BOOL CSLoadedUser_add(CSUser_t * pCSUser, char * url);
extern BOOL CSLoadedUser_remove(CSUser_t * pCSUser);
extern BOOL CSLoadedUser_find(char * name);
extern BOOL CSLoadedUser_deleteAll (void);

/*

CSLoadedUser_enum - used to iterate through the loaded users

pCallback - application callback to call with each user

pVoid - passed through to callback
*/

typedef CSError_t (CSLoadedUserCallback)(CSUser_t * pCSUser, 
					 int index, void * pVoid);
extern int CSLoadedUser_enum(CSLoadedUserCallback * pCallback, 
			     void * pVoid);

/*


.
  CSUserList
.

stores a list of PICS users and the URLs to their descritpion files

(
  CSUserList_load
)
*/

extern BOOL CSUserList_load(char * url, char * relatedName);

/*


(
  CSUserList_enum
)

used to iterate through the known users

pCallback - application callback to call with each user

pVoid - passed through to callback

username

url - where to find this user's profile

index
*/

typedef CSError_t (CSUserListCallback)(char * username, char * url, 
		   int index, void * pVoid);
extern int CSUserList_enum(CSUserListCallback * pCallback, void * pVoid);

/*
!
  Registering Users
!
*/

extern BOOL CSApp_registerDefaultUserByName(char * user, char * password);
extern CSUser_t * CSApp_registerUserByName(char * user, char * password);
extern BOOL CSApp_setDefaultUser(CSUser_t * pCSUser);
extern BOOL CSApp_checkUser(CSUser_t * pCSUser);
extern BOOL CSApp_unregisterDefaultUser(void);
extern BOOL CSApp_unregisterUser(CSUser_t * pCSUser);

/*
!
  application functions
!


(
  CSApp_registerApp
)

- register defaults for an application

pCallback - callback to tell app the disposition of header check

criteria - when the app wants the callback

pUserCallback - called when a new user is loaded. It returns:
	 
	   o 
	     1: load this user and set as default
	   o 
	     0: load this user
	   o 
	     -1: get rid of it
	 
	 
pVoid - passed through to callback

CSApp_unregisterApp - unregister defaults and free associated memory
*/

typedef int (CSApp_userCallback)(CSUser_t * pCSUser, void * pVoid);
extern BOOL CSApp_registerApp(CSDisposition_callback * pCallback, 
			      CSDisposition_criteria criteria, 
			      CSApp_userCallback * pUserCallback, 
			      void * pVoid);
extern BOOL CSApp_unregisterApp();

/*
(
  CSApp_registerReq
)

- override defaults for a particular request. It is advisable to use this
function, rather than relying on the defaults as it eliminates many problems
associated with multiple clients using the same dynamic library. Use
CSApp_UnregisterReq for every call to CSApp_RegisterReq.

pReq - pointer to request about to be sent.

rest - see parameters for CSApp_RegisterApp()

CSApp_UnregisterReq - free memory associated with call to CSApp_registerReq
*/

extern BOOL CSApp_registerReq(HTRequest* pReq, CSUser_t * pCSUser, 
			      CSDisposition_callback callback, 
			      CSDisposition_criteria criteria,
			      void * pVoid);

extern BOOL CSApp_unregisterReq(HTRequest* pReq);

extern HTRequest * CSApp_originalRequest(HTRequest* pReq);


/*
!
  MISC
!

CSApp_disposition - tell PICS if a request is allowd
*/

extern BOOL CSApp_label(HTRequest * pReq, CSLabel_t * pCSLabel);

/*

CSApp_libraryVersion - get current version

CSParseMachRead, CSParseUser, CSParseLabel - HTConverters for parsing to
these objects

CSLabel_output - spew canonical form of pCSLabel out to pStream
*/

char * CSApp_libraryVersion(void);

extern HTConverter CSParseMachRead;
extern HTConverter CSParseUser;
extern HTConverter CSParseLabel;

extern int CSLabel_output(CSLabel_t * pCSLabel, HTStream * pStream);

/*
*/

#endif /* CSLAPP_H */

/*

End of Declaration
*/
