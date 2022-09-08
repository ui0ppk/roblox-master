#ifndef __COMMON_HDR__
#define __COMMON_HDR__

#include <stdarg.h>
#include <iostream>
//#include <ctype.h>

#ifndef _INC_STDIO
  #include <stdio.h>
#endif

#ifndef _INC_FCNTL
  #include <fcntl.h>
#endif

#ifndef _INC_FLOAT
  #include <float.h>
#endif

#ifndef _INC_STRING
  #include <string.h>
#endif

/*#ifndef _INC_MATH
  #include <math.h>
#endif*/

#include <cmath>
using namespace std;

#ifndef _INC_ERRNO
  #include <errno.h>
#endif

#ifndef _INC_TIME
  #include <time.h>
#endif

#include <assert.h>

#include "..//sgCore.h"


#if (SG_CURRENT_PLATFORM!=SG_PLATFORM_WINDOWS)
#include <malloc/malloc.h>   // MAC
#endif

#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
#include <malloc.h>
#endif



#if (SG_CURRENT_PLATFORM!=SG_PLATFORM_WINDOWS)
typedef  long long   HANDLE;
typedef  unsigned char  UCHAR;
typedef  unsigned long  ULONG;
typedef  int           BOOL;
typedef unsigned short   WORD;
typedef char *LPSTR;
typedef const char*  LPCTSTR;
typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef unsigned long long  LPARAM;
typedef unsigned int __int64;      // MAC
typedef  unsigned int  UINT;
typedef  long          LONG;
typedef  unsigned long COLORREF;
typedef  void*         LPCVOID;
typedef short SHORT;

#define RGB(r, g ,b) ((DWORD) (((BYTE) (r) | ((WORD) (g) << 8)) |   (((DWORD) (BYTE) (b)) << 16))) 
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb)>>16))

// FOR MinGW
#define   TRUE   true
#define   FALSE  false

#else
    #include  <windows.h>
#endif

typedef     int   SG_PTR_TO_DIGIT;         // __int64 for 64 bit


#ifndef MAXPATH
	#define MAXPATH 256
#endif

#ifndef _MAX_PATH
	#define _MAX_PATH 256
#endif

#ifndef M_PI
	#define   M_PI   3.14159265
#endif
//

extern  sgCScene*  scene;

extern  SG_ERROR   global_sg_error;

#define   KERNEL_VERSION_MAJOR    2
#define   KERNEL_VERSION_MINOR    1
#define   KERNEL_VERSION_RELEASE  0
#define   KERNEL_VERSION_BUILD    521

#define  NEED_MEMORY_REPORT

#define  NEW_TRIANGULATION     11

extern   SG_TRIANGULATION_TYPE     triangulation_temp_flag;

SG_OBJ_HANDLE   GetObjectHandle(const sgCObject* obj);
void         SetObjectHandle(sgCObject* ob,SG_OBJ_HANDLE hndl);
sgCObject*   ObjectFromHandle(SG_OBJ_HANDLE objH);
void         DeleteExtendedClass(void* objH);
void         AllocMemoryForBRepPiecesInBRep(sgCBRep*,int);
void         Set3DObjectType(sgC3DObject*, SG_3DOBJECT_TYPE);
// Set_i_BRepPiece(TMP_STRUCT_FOR_BREP_PIECES_SETTING*,lpNPW) - after lpNPW

extern  ICoreAppInterface*     application_interface;
extern  sgCFont*               temp_font;

extern  LPSTR   ids; ///  buffer for all strings

extern  bool        ignore_undo_redo;
extern  bool        show_memory_leaks;

typedef void*   VADR;       // virtual address

typedef VADR        hOBJ;
typedef VADR        hCSG;


typedef enum
{
  TCPR_EXIST_NO_CLOSE,
  TCPR_EXIST_NO_FLAT,
  TCPR_EXIST_WITH_BAD_ORIENT,
  TCPR_NOT_IN_ONE_PLANE,
  TCPR_EXIST_INTERSECTS,
  TCPR_BAD_ORIENT,
  TCPR_BAD_INCLUDES,
  TCPR_SUCCESS
} TEST_CORRECT_PATHS_RES;

TEST_CORRECT_PATHS_RES test_correct_paths_array_for_face_command(hOBJ out_obj,
                          hOBJ* int_objcs,
                          int int_objcs_count);

#define _MAX_FNAME     256
#define _MAX_EXT       256
#define _MAX_DIR       256
#define _MAX_DRIVE     3

#ifndef MAX_PATH
	#define MAX_PATH   _MAX_PATH
#endif

#define MAXFILE   _MAX_FNAME
#define MAXEXT    _MAX_EXT
#define MAXDIR    _MAX_DIR
#define MAXDRIVE  _MAX_DRIVE

#ifndef MAXINT
	#define MAXINT    32767  
#endif

// ROBLOX change
// #if (SG_CURRENT_PLATFORM==SG_PLATFORM_IOS)
// if sgFloat == float
	 #define MAXsgFloat   FLT_MAX
	 #define MINsgFloat   FLT_MIN
	 #define MINFLOAT   FLT_MIN
	 #define MAXLsgFloat  FLT_MAX
	 #define MINLsgFloat  FLT_MIN
//#endif
 



#ifndef MAXLONG
	#define MAXLONG 0x7FFFFFFFL
#endif

#ifndef MAXSHORT
	#define MAXSHORT 0x7FFF   
#endif
/*#define M_PI    3.14159265358979323846
#define M_PI_2  1.57079632679489661923*/

#define argsused  


void splitpath(const char *path, char *drive, char *dir, char *fName, char *ext);
void makepath(char *path, const char *drive, const char *dir, const char *fName, const char *ext);

#define fnmerge makepath
#define fnsplit splitpath

#define getcwd    _getcwd
#define chdir   _chdir
#define strncmpi  _strnicmp


#define getdisk   _getdrive
#define setdisk   _chdrive


#define SIZE_VPAGE    16384   // SIZE OF ONE PAGE OF VIRTUAL MEMORY

// Screen region
typedef struct {
  short x1;
  short y1;
  short x2;
  short y2;
}REGION;
typedef REGION  * lpREGION;

typedef struct {
  sgFloat x;
  sgFloat y;
  sgFloat z;
}D_POINT;

typedef struct {
  float x;
  float y;
  float z;
}F_POINT;

typedef struct {
  sgFloat x;
  sgFloat y;
}D_PLPOINT;

typedef struct {
  float x;
  float y;
}F_PLPOINT;

typedef struct {
  D_POINT v;
  sgFloat d;
}D_PLANE;

typedef struct {
  D_POINT min, max;
} REGION_3D;

typedef sgFloat      MATR[16];
typedef sgFloat      DA_POINT[3];

typedef D_POINT          * lpD_POINT;
typedef D_PLPOINT        * lpD_PLPOINT;
typedef F_POINT          * lpF_POINT;
typedef F_PLPOINT        * lpF_PLPOINT;
typedef D_PLANE          * lpD_PLANE;
typedef sgFloat           * lpMATR;
typedef REGION_3D        * lpREGION_3D;
typedef DA_POINT         * lpDA_POINT;

#define MAXFILE16 9 //256 name
#define MAXEXT16    5 //256 ext; includes leading dot (.)

extern sgFloat eps_d;   // distances precision
extern sgFloat eps_n;   // normal coefficints precision



#endif
