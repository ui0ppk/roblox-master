#ifndef  __sgErrors__
#define  __sgErrors__

typedef enum
{
    SG_ER_SUCCESS								= 0,
    SG_ER_UNKNOWN								= 500,
	SG_ER_INTERNAL								= 600,

	SG_ER_BAD_ARGUMENT_UNKNOWN					= 1000, // 1000-2000 - reserve for BAD_ARGUMENT errors
	SG_ER_BAD_ARGUMENT_NULL_POINTER				= 1001, 
	SG_ER_BAD_ARGUMENT_ZERO						= 1002,
	SG_ER_BAD_ARGUMENT_NON_POSITIVE				= 1003,
	SG_ER_BAD_ARGUMENT_ZERO_LENGTH				= 1004,
	SG_ER_BAD_ARGUMENT_CONFLICT_BEETWEEN_ARGS   = 1005,
} SG_ERROR;

sgCore_API  SG_ERROR    sgGetLastError();

#endif