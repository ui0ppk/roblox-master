#include "../sg.h"
long ControlMalloc = 833;

//          

void StopInControlMalloc(void)
//   malloc c   ControlMalloc
{
}

void StopInNullSizeMalloc(void)
//   malloc c  
{
}

void StopInBadPointerFree(void)
//   free c  
{
}

void StopInBadPointerRealloc(void)
//   realloc c  
{
}