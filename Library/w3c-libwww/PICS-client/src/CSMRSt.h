/*

					Machine-readable data internals






!Machine-readable data internals!

*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module defines the Machine-readable data structures read by 
CSParser.c. Applications will include this if they
want direct access to the data (as opposed to using iterator methods).

The following data structures relate to the data encapsulated in a PICS 
description. Each data type correlates to a time in the BNF for the 
machine-readable description.
See PICS Labels spec
for more details.

*/

#ifndef CSMRST_H
#define CSMRST_H

/*

*/

typedef struct {
    SVal_t name;
    SVal_t description;
	FVal_t value;
    SVal_t icon;
    } MachRead_enum_t;

typedef struct MachRead_category_s MachRead_category_t;
struct MachRead_category_s {
    SVal_t transmit;
    SVal_t icon;
    SVal_t name;
    SVal_t description;
    FVal_t min;
    FVal_t max;
    BVal_t multi;
    BVal_t unord;
    BVal_t integer;
    BVal_t labeled;
    HTList * machRead_enums;
    HTList * machRead_categories;
    MachRead_category_t * pParent;
    };

struct CSMachReadData_s {
    FVal_t version;
    SVal_t system;
    SVal_t service;
    SVal_t icon;
    SVal_t name;
    SVal_t description;
	FVal_t min;
	FVal_t max;
	BVal_t multi;
	BVal_t unord;
	BVal_t integer;
	BVal_t labeled;
    HTList * machRead_categories;
    };
 
/*

--------------these belong in CSMR.html--------------

*/

extern CSMachReadData_t * CSMachRead_getCSMachReadData(
					 CSMachRead_t * pCSMachRead);
extern MachRead_category_t * CSMachRead_getMachReadCategory(
					 CSMachRead_t * pCSMachRead);
extern MachRead_enum_t * CSMachRead_getMachReadEnum(
					 CSMachRead_t * pCSMachRead);

/*

*/

#endif /* CSMRST_H */

/*

End of Declaration

*/
