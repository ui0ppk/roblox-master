/*

					User data internals






!User data internals!

*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module defines the User data structures read by 
CSParser.c. Applications will include this if they
want direct access to the data (as opposed to using iterator methods).

The following data structures relate to the data encapsulated in a PICS User.
Each data type correlates to a time in the BNF for the user description.

*/

#ifndef CSUSERST_H
#define CSUSERST_H

/*

*/

typedef struct {
    SVal_t identifier;
    BVal_t missing_scale;
    BVal_t observe_dates;
    HTList * ranges;
    } UserServiceRating_t;

typedef struct {
    FVal_t version;
    SVal_t rating_system;
    SVal_t rating_service;
    BVal_t missing_service;
    BVal_t missing_scale;
    BVal_t observe_dates;
    HTList * userServiceRatings;
    } UserService_t;

struct CSUserData_s {
    FVal_t version;
    SVal_t user_name;
    SVal_t password;
    BVal_t super_user;
    FVal_t minimum_services;
    BVal_t missing_service;
    BVal_t missing_scale;
    BVal_t observe_dates;
    SVal_t bureau;
    HTList * proxies;
    HTList * userServices;
    };

/*

--------------these provide access to the above data types--------------

*/

extern CSUserData_t * CSUser_getCSUserData(CSUser_t * me);
extern UserService_t * CSUser_getUserService(CSUser_t * pCSUser);
extern UserServiceRating_t * CSUser_getUserServiceRating(CSUser_t * pCSUser);

/*

*/

#endif /* CSUSERST_H */

/*

End of Declaration

*/
