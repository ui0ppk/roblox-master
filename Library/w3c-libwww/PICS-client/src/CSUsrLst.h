/*

					UserList for libpics





!UserList!

*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module provides the interface to CSUsrLst.c. 
UserLists are a list of users and the URLs where their profiles are kept.
UserLists are read from a stream.

*/

#ifndef CSUSRLST_H
#define CSUSRLST_H

/*

(UserList element)

*/

typedef struct {
	char * user;
	char * URL;
} UserListEl_t;

/*

(Methods)

Getting at the UserList

*/

extern HTList * CSUserList_get(void);
extern BOOL CSUserList_destroy(void);
extern char * CSUserList_findURL(char * username);

/*

Stream to read the UserList

*/

extern HTStream * CSUserLists (HTRequest *	request,
               	               void *		param,
               	               HTFormat		input_format,
               	               HTFormat		output_format,
               	               HTStream *	output_stream);

/*

*/

#endif /* CSUSRLST_H */

/*

End of Declaration

*/
