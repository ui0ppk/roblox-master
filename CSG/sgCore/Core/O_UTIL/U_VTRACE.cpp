#include "../sg.h"

unsigned long TotalMallocCount   = 0L;
unsigned long BadMallocCount     = 0L;
unsigned long BadReAllocCount    = 0L;
unsigned long TotalFreeCount     = 0L;
unsigned long BadFreeCount       = 0L;
unsigned long TotalMallocMemSize = 0L;

#define  MAXNUMMALLOC 100000
unsigned long *pPtrs          = NULL;
unsigned long *pMallocNumber  = NULL;
extern   long ControlMalloc;
long     FirstEmptyIdx = 0;
long     AMallocLen    = 0;


static  long   SG_MSize(void* memBlock)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_MAC)
    return malloc_size(memBlock);
#endif
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
	return  _msize(memBlock);  //  MinGW
#endif
}

//  malloc          
//  0  
void *SGMalloc(size_t size)
{
void*         v;
long          Idx;
unsigned long ss;

	if(size <= 0){
    StopInNullSizeMalloc();
    FatalInternalError(2);
    return NULL;
  }
	else{
    TotalMallocCount++;
	  v = malloc(size);
  }
  if(v){
//    ss = ((*(((unsigned long*)v) - 1))>>2)<<2;
    ss = SG_MSize(v);
    TotalMallocMemSize += ss;
    memset(v, 0, size);
    if(nMEM_TRACE_LEVEL < 2)
      return v;
    if(FirstEmptyIdx > 0){
      Idx = FirstEmptyIdx - 1;
      pPtrs[Idx] = (unsigned long)v;
      FirstEmptyIdx = pMallocNumber[Idx];
      pMallocNumber[Idx] = TotalMallocCount;
    }
    else if(AMallocLen < MAXNUMMALLOC){
      Idx = AMallocLen;
      pPtrs[Idx] = (unsigned long)v;
      pMallocNumber[Idx] = TotalMallocCount;
      AMallocLen++;
    }
    if(ControlMalloc == TotalMallocCount)
      StopInControlMalloc();
    return v;
  }
  FatalInternalError(3);
  return NULL;
}


//  malloc   .     NULL
//  0  
void *SGMallocN(size_t size)
{
void*         v;
long          Idx;
unsigned long ss;

	if(size <= 0){
    StopInNullSizeMalloc();
    BadMallocCount++;
    return NULL;
  }
	else{
    TotalMallocCount++;
	  v = malloc(size);
  }
  if(v){
    ss = ((*(((unsigned long*)v) - 1))>>2)<<2;
    TotalMallocMemSize += ss;
    memset(v, 0, size);
    if(nMEM_TRACE_LEVEL < 2)
      return v;
    if(FirstEmptyIdx > 0){
      Idx = FirstEmptyIdx - 1;
      pPtrs[Idx] = (unsigned long)v;
      FirstEmptyIdx = pMallocNumber[Idx];
      pMallocNumber[Idx] = TotalMallocCount;
    }
    else if(AMallocLen < MAXNUMMALLOC){
      Idx = AMallocLen;
      pPtrs[Idx] = (unsigned long)v;
      pMallocNumber[Idx] = TotalMallocCount;
      AMallocLen++;
    }
    if(ControlMalloc == TotalMallocCount)
      StopInControlMalloc();
    return v;
  }
  return NULL;
}

void SGFree(void* address)
{
long          i;
unsigned long ss;

if (!address)
	return;

	if(address){
    if(nMEM_TRACE_LEVEL < 2){
//      ss = ((*(((unsigned long*)address) - 1))>>2)<<2;
      ss = SG_MSize(address);
      TotalMallocMemSize -= ss;
	    free(address);
	    TotalFreeCount++;
      return;
    }
    for(i = 0; i < AMallocLen; i++){
      if(pPtrs[i] == (unsigned long)address){
        pPtrs[i] = 0;
        pMallocNumber[i] = FirstEmptyIdx;
        FirstEmptyIdx = i + 1;
//        ss = ((*(((unsigned long*)address) - 1))>>2)<<2;
        ss = SG_MSize(address);
        TotalMallocMemSize -= ss;
	      free(address);
	      TotalFreeCount++;
        return;
      }
    }
    BadFreeCount++;
    StopInBadPointerFree();
		return;
  }
  BadFreeCount++;
  StopInBadPointerFree();
}

//  realloc          
void *SGRealloc(void *block, size_t size)
{
unsigned long ss;
void*         v;
long          i;

  if(!size){    //   
    if(block)   //      -   free
      SGFree(block);
    else{       //   -   
      StopInBadPointerRealloc();
      FatalInternalError(4);
    }
    return NULL;
  }
  if(!block)    //   -    -   malloc
    return SGMalloc(size);
  //     
  if(nMEM_TRACE_LEVEL < 2){
//    ss = ((*(((unsigned long*)block) - 1))>>2)<<2;
    ss = SG_MSize(block);
    TotalMallocMemSize -= ss;
    v = realloc(block, size);
    if(v){
      ss = ((*(((unsigned long*)v) - 1))>>2)<<2;
      ss = SG_MSize(v);
      TotalMallocMemSize += ss;
      return v;
    }
    FatalInternalError(3);
    return NULL;
  }
  for(i = 0; i < AMallocLen; i++){
    if(pPtrs[i] == (unsigned long)block)
      goto next;
  }
    StopInBadPointerRealloc();
  FatalInternalError(5);
  return NULL;
next:
//  ss = ((*(((unsigned long*)block) - 1))>>2)<<2;
  ss = SG_MSize(block);
  TotalMallocMemSize -= ss;
  v = realloc(block, size);
  if(v){
    ss = ((*(((unsigned long*)v) - 1))>>2)<<2;
    ss = SG_MSize(v);
    TotalMallocMemSize += ss;
    pPtrs[i] = (unsigned long)v;
    return v;
  }
  FatalInternalError(3);
  return NULL;
}




void *SGReallocN(void *block, size_t size)
{
unsigned long ss;
void*         v;
long          i;

  if(!size){    //   
    if(block)   //      -   free
      SGFree(block);
    else{       //   -   
      BadReAllocCount++;
      StopInBadPointerRealloc();
    }
    return NULL;
  }
  if(!block)    //   -    -   malloc
    return SGMalloc(size);
  //     
  if(nMEM_TRACE_LEVEL < 2){
    ss = ((*(((unsigned long*)block) - 1))>>2)<<2;
    TotalMallocMemSize -= ss;
    v = realloc(block, size);
    if(v){
      ss = ((*(((unsigned long*)v) - 1))>>2)<<2;
      TotalMallocMemSize += ss;
    }
    return v;
  }
  for(i = 0; i < AMallocLen; i++){
    if(pPtrs[i] == (unsigned long)block)
      goto next;
  }
  //   -  .  NULL
  StopInBadPointerRealloc();
  BadReAllocCount++;
  return NULL;
next:
  ss = ((*(((unsigned long*)block) - 1))>>2)<<2;
  TotalMallocMemSize -= ss;
  v = realloc(block, size);
  if(v){
    ss = ((*(((unsigned long*)v) - 1))>>2)<<2;
    TotalMallocMemSize += ss;
    pPtrs[i] = (unsigned long)v;
  }
  return v;
}

void InitDbgMem(void)
{
  if(nMEM_TRACE_LEVEL < 2)
    return;
	pPtrs = (unsigned long*)malloc(MAXNUMMALLOC*sizeof(unsigned long));
	pMallocNumber = (unsigned long*)malloc(MAXNUMMALLOC*sizeof(unsigned long));
  if(!pPtrs || !pMallocNumber)
    nMEM_TRACE_LEVEL = 1;
}

void FreeDbgMem(void)
{
  if(pPtrs)
    free(pPtrs);
  pPtrs = NULL;
  if(pMallocNumber)
    free(pMallocNumber);
  pMallocNumber = NULL;
}

#include <IOSTREAM>

bool        show_memory_leaks=true;

void vReport(void)
{
  if(nMEM_TRACE_LEVEL < 1)
    return;
  {
  char buf[100];
  long i;

    ControlMalloc = 0;
    for(i = 0; i < AMallocLen; i++){
      if(pPtrs[i]){
        ControlMalloc = pMallocNumber[i];
        break;
      }
    }
    if(TotalMallocCount != TotalFreeCount ||
      BadMallocCount != 0  ||
      BadFreeCount != 0    ||
      BadReAllocCount != 0 ||
      ControlMalloc != 0
    )
	{
			if (show_memory_leaks)
			{
					if(TotalMallocCount != TotalFreeCount)
					{
						int j=0;
						j=sprintf(buf," Non free objects count:  %ld", TotalMallocCount-TotalFreeCount);
						sprintf(buf + j,"\r\n Non free memory size : %ld bytes", TotalMallocMemSize+126);
					#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
					  	MessageBox(NULL, buf, "MEMORY LEAK",MB_OK|MB_ICONERROR) ;
					#endif
                        
                    #if (SG_CURRENT_PLATFORM==SG_PLATFORM_IOS)
                        fprintf(stderr,"sgCore MEMORY LEAK - %s\n", buf);
                    #endif
					}
					else
					{
						assert(0);
					}
			}

    }
    FreeDbgMem();
  }
}

#pragma argsused
void FatalInternalError(int code)
//      Main.    
{
char Buf[256];

  strcpy(Buf, "Fatal internal error");
//  MessageBox(NULL, Buf, GetSystemName(), MB_OK|MB_ICONERROR);
	exit(code);
}

