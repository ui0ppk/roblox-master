/*

  
  					The Queue Class


!
  The Queue Class
!
*/

/*
**      (c) COPYRIGHT MIT 1995.
**      Please first read the full copyright statement in the file COPYRIGH.
*/

/*
*/

#ifndef HTQUEUE_H
#define HTQUEUE_H

#include "WWWLib.h"

/*
.
  Methods
.
*/

PUBLIC HTList * HTQueue_new(void);
PUBLIC BOOL HTQueue_delete(HTList *me);
PUBLIC BOOL HTQueue_enqueue(HTList *me,void *newObject);
PUBLIC BOOL HTQueue_append(HTList *me,void *newObject);
PUBLIC BOOL HTQueue_dequeue(HTList *me);
PUBLIC BOOL HTQueue_isEmpty(HTList *me);
PUBLIC void * HTQueue_headOfQueue(HTList *me);
PUBLIC int HTQueue_count(HTList *me);

/*
*/

#endif /* HTQUEUE_H */

/*

  

  @(#) $Id$

*/
