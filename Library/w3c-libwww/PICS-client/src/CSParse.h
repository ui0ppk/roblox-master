/*

					Parser for libpics





!Parser for libpics!

*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module provides the interface to CSParse.c. 
The parser is used to parse labels, 
machine-readable descriptions, and 
users. The application creates one of these and iteratevely calls CSParse_parseChunk until it returns a done or an error.


*/

#ifndef CSPARSE_H
#define CSPARSE_H
#include "CSLUtils.h"
#include "HTChunk.h"

/*

.NowIn.
tells CSParse where it is in the task of tokenizing
*/

typedef enum {
    NowIn_INVALID = 0,
    NowIn_NEEDOPEN,
    NowIn_ENGINE,
    NowIn_NEEDCLOSE,
    NowIn_END,
    NowIn_MATCHCLOSE,
    NowIn_ERROR,
    NowIn_CHAIN
    } NowIn_t;

/*


(Construction/Destruction)

The parse objects are never created by the application, but instead by one of
the objects that it is used to parse.

*/

extern CSParse_t * CSParse_new(void);
extern void CSParse_delete(CSParse_t * me);

/*

(some handy definitions)

*/

#define LPAREN '('
#define RPAREN ')'
#define LCURLY '{'
#define RCURLY '}'
#define LBRACKET '['
#define RBRACKET ']'
#define SQUOTE 0x27 /* avoid confusing parens checking editors */
#define DQUOTE 0x22
#define LPARENSTR "("
#define RPARENSTR ")"
#define raysize(A) (sizeof(A)/sizeof(A[0]))

/*

!subparser data!
.Punct.
valid punctuation

*/

typedef enum {Punct_ZERO = 1, Punct_WHITE = 2, Punct_LPAREN = 4, 
	      Punct_RPAREN = 8, Punct_ALL = 0xf} Punct_t;

/*

.SubState.
Enumerated bits that are used to mark a parsing state. Because they are bits, 
as opposed to sequential numbers, a StateToken may
or more than one together and serve more than one state. They must have 
identical outcomes if this is to be exploited.

By convention, the following SubState names are used:
o X - has no state
	 o N - is a newly created object
	 o A-H - substate definitions. Because they are non-conflicting bits, a 
	 subparser may have options that sit in more than state. For instance, the 
	 string "error" may be matched in states A and C with:
{"error test", SubState_A|SubState_C, Punct_LPAREN, 0, "error"}

*probs* I meant to keep these 16 bit caompatible, but ran up short at the end 
of one StateToken list. This can be fixed if anyone needs a 16 bit enum.

*/

typedef enum {SubState_X = -1, SubState_N = 0x4000, SubState_A = 1, 
	      SubState_B = 2, SubState_C = 4, SubState_D = 8, 
	      SubState_E = 0x10, SubState_F = 0x20, SubState_G = 0x40, 
	      SubState_H = 0x80, SubState_I = 0x100} SubState_t;

/*

forward declaration for StateToken_t

*/

typedef struct StateToken_s StateToken_t;

/*

.Engine.
called by CSParse to process tokens and punctuation
*/

typedef NowIn_t Engine_t(CSParse_t * pCSParse, char demark, void * pVoid);

/*

Engine employed by the Label, MacRed, and User parsers
*/

Engine_t CSParse_targetParser;

/*

.substate methods.
All methods return a StateRet.

(Check)
see if a value is legitimate, may also record it
*/

typedef StateRet_t Check_t(CSParse_t * pCSParse, StateToken_t * pStateToken, 
			   char * token, char demark);

/*

Punctuation checker to be employed by Check_t functions
*/

extern BOOL Punct_badDemark(Punct_t validPunctuation, char demark);

/*

(Open)
create a new data structure to be filled by the parser
*/

typedef StateRet_t Open_t(CSParse_t * pCSParse, char * token, char demark);

/*

(Close)
tell the state that the data structure is no longer current
*/

typedef StateRet_t Close_t(CSParse_t * pCSParse, char * token, char demark);

/*

(Prep)
get ready for next state
*/

typedef StateRet_t Prep_t(CSParse_t * pCSParse, char * token, char demark);

/*

(Destroy)
something went wrong, throw away the current object
*/

typedef void Destroy_t(CSParse_t * pCSParse);

/*


.Command.
substate commands

o open - call the open function for the current data structure
	 o close - call the close
	 o chain - call again on the next state without re-reading data
	 o notoken - clear the token before a chain (so next state just gets punct)
	 o matchany - match any string
	 
	 */

typedef enum {Command_NONE = 0, Command_OPEN = 1, Command_CLOSE = 2, 
	      Command_CHAIN = 4, Command_NOTOKEN = 8, 
	      Command_MATCHANY = 0x10} Command_t;

/*

.StateToken structure.
Contains all the information about what tokens are expected in what substates.
The StateTokens are kept in array referenced by a TargetObject.

*/

struct StateToken_s {
    char * note;		/* some usefull text that describes the state - usefulll for debugging */
    SubState_t validSubStates;
    Punct_t validPunctuation;
    Check_t * pCheck;   /* call this function to check token */
    char * name1;       /* or compare to this name */
    char * name2;		/* many strings have 2 spellings ("ratings" vs. "r") */
    CSParseTC_t targetChange; /* whether target change implies diving or climbing from current state */
    TargetObject_t * pNextTargetObject;
    SubState_t nextSubState;
    Command_t command;	/* open, close, chain, etc. */
    Prep_t * pPrep;		/* prepare for next state */
    };

/*

.TargetObject structure.
Methods and a lists of StateTokens associated with a data structure. The
methods know how to read data into current object and the StateTokens tell
when to proceed to the next object.

*/

struct TargetObject_s {
    char * note;
    Open_t * pOpen;   /* call this function to open structure */
    Close_t * pClose;   /* call this function to close structure */
    Destroy_t * pDestroy;
    StateToken_t * stateTokens; /* array of sub states */
    int stateTokenCount;        /* number of sub states */
    CSParseTC_t targetChange; /* target change signal for opening this parse state */
    };

/*

.ValTarget.

*/

typedef union {
    BVal_t * pTargetBVal;
    FVal_t * pTargetFVal;
    SVal_t * pTargetSVal;
    DVal_t * pTargetDVal;
    HTList ** pTargetList;
    } ValTarget_t;

/*

.ValType.
Write down what value is to be read, and what type it is

*/

typedef enum {ValType_NONE, ValType_BVAL, ValType_FVAL, 
	      ValType_SVAL, ValType_DVAL, 
	      ValType_COMMENT} ValType_t;

/*

.ParseContext.
Part of a CSParse. The boundry is a litte fuzzy. Maybe it should not exist.

*/

typedef struct {
    Engine_t * engineOf;
    TargetChangeCallback_t * pTargetChangeCallback;
    ParseErrorHandler_t * pParseErrorHandler;

    /* for reading [BFSD]Val_t */
    ValTarget_t valTarget;
    ValType_t valType;

    char * pTokenError;

    BOOL observeQuotes;
    BOOL observedQuotes;
    char * legalChars;
    int legalCharCount;
    } ParseContext_t;

/*

.CSParse structure.
Full parser state and pointer to the object that it is reading.

*/

struct CSParse_s {
    char quoteState;
    NowIn_t nowIn;
    HTChunk * token;
    char demark;
    int offset;
    int depth;
    ParseContext_t * pParseContext;
    union { /* all the types this parse engine fills */
        CSMachRead_t * pCSMachRead; /* defined in CSMacRed.c */
        CSLabel_t * pCSLabel; /* defined in CSLabel.c */
        CSUser_t * pCSUser; /* defined in CSUser.c */
        } target;
    TargetObject_t * pTargetObject;
    SubState_t currentSubState;
    StateToken_t * pStateToken;
    };

/*

*/

#endif /* CSPARSE_H */

/*

End of Declaration

*/
