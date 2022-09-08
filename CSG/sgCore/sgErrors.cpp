#include "CORE//sg.h"

SG_ERROR   global_sg_error = SG_ER_SUCCESS;

SG_ERROR    sgGetLastError()
{
	return global_sg_error;
}