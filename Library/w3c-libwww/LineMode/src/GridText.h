/*

					Style Management for Line Mode Browser




!Style Definition for Hypertext!

*/

/*
**	(c) COPYRIGHT MIT 1995.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module defines the HText functions referenced from the HTML
module in the W3C
Sample Code Library.

*/

#include "WWWLib.h"
#include "HText.h"

extern HTChildAnchor * HText_childNumber (HText * text, int n);

/* delete a hyper doc object with interfering with any anchors */
extern void hyperfree (HText * me);

/* Is there any file left? */
extern BOOL HText_canScrollUp (HText * text);
extern BOOL HText_canScrollDown (HText * text);

/* Move display within window */
extern void HText_scrollUp (HText * text);	/* One page */
extern void HText_scrollDown (HText * text);	/* One page */
extern void HText_scrollTop (HText * text);
extern void HText_scrollBottom (HText * text);

extern BOOL HText_select (HText * text);
extern BOOL HText_selectAnchor (HText * text, HTChildAnchor * anchor);

extern int HText_sourceAnchors (HText * text);
extern void HText_setStale (HText * text);
extern void HText_refresh (HText * text);

#ifdef CURSES
extern int HText_getTopOfScreen (HText * text);
extern int HText_getLines (HText * text);
#endif


/*

.
Delete All Documents in Memory
.

This is typically called at clean up time just before exiting the application.

*/

extern BOOL HText_freeAll (void);

/*

.
Callbacks
.

These functions are invoked as callbacks from the HText interface.

*/

extern HText* LMHText_new (HTRequest * req,
		HTParentAnchor * anchor,HTStream * output_stream);
extern HText* LMHText_new2 (HTRequest * req,
              HTParentAnchor * anchor,HTStream * output_stream);
extern void HText_free (HText * self);
extern BOOL LMHText_delete (HText * self);
extern void LMHText_addText (HText * text, const char * str, int length);
extern void LMHText_appendText (HText * text, const char * str);
extern void LMHText_beginAnchor (HText *text,
		int elem_num, int attr_num, HTChildAnchor* anc,
		const BOOL* present, const char** value);


/*

End of declaration


*/


