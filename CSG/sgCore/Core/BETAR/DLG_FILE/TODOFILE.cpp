#include "../../sg.h"


#include <sys/stat.h>         /* declare the 'stat' structure */
#include <sys/types.h>

#if (SG_CURRENT_PLATFORM!=SG_PLATFORM_WINDOWS)
        #include <unistd.h>
#endif

//  MingW

//

#include <stdio.h>     
//f+
// return only small file size < DWORD
//
LONG GetFileNameSize(LPSTR	fname)
{
   #if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
            HANDLE fin;
            DWORD dw;

            if (INVALID_HANDLE_VALUE == (fin = CreateFileA(fname, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, NULL, NULL)))
            return -1;
            dw = GetFileSize(fin, NULL);
            CloseHandle(fin);
            return dw;
    #else
        struct stat st;
        stat(fname, &st);
        return st.st_size;
    #endif
}

//f+
//
//
BOOL FileIsExist(LPSTR	fname)
{
     #if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
            HANDLE fin;

            if (INVALID_HANDLE_VALUE == (fin = CreateFileA(fname, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, NULL, NULL)))
            return FALSE;
             else
                {
                    CloseHandle(fin);
                    return TRUE;
                 }
     #else
        struct stat stat_p;
        stat (fname, &stat_p);
        return S_ISREG(stat_p.st_mode);
     #endif
}

//
//
//
LONG nb_filelength(HANDLE fin)
{
    #if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
        DWORD dw;
        dw = GetFileSize(fin, NULL);
        return dw;
     #else
	
        DWORD size = lseek(fin, 0, SEEK_END); // seek to end of file	fseek(fin, 0, SEEK_SET); // seek back to beginning of file
        lseek(fin, 0, SEEK_SET);
        // proceed with allocating memory and reading the file
	    return size;
      #endif

}

// amode == 0 -- file exist
//       == 4 -- file is readonly
//       == 6 -- file is writeable
//
//
int nb_access(const char *name, int amode)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)

      DWORD dw;
      dw = GetFileAttributesA(name);
      if ((DWORD)INVALID_HANDLE_VALUE == dw)
          return -1;
        switch (amode) {
      case 0:
      default:
        return 0;

      case 4:
        if (dw & FILE_ATTRIBUTE_READONLY)
            return 0;
        else
                return 0;//-1;

      case 6:
        if (dw & FILE_ATTRIBUTE_READONLY)
            return -1;
        else
                return 0;
      }
  #else
      return access(name, amode);
  #endif
}

//
//
//
DWORD nb_attr(const char *filename)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
  return GetFileAttributesA(filename);
#else
    return 0;
#endif
}

//
//
//
#pragma argsused
HANDLE nb_open(const char *fname, int access, WORD mode)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
   HANDLE hDest;

	if (access & O_CREAT)
  	if (mode)
      if (access & O_RDWR)
        hDest = CreateFileA(fname, GENERIC_READ | GENERIC_WRITE,
          0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_RANDOM_ACCESS, NULL);
      else
        hDest = CreateFileA(fname, GENERIC_READ,
          0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_RANDOM_ACCESS, NULL);
    else
      if (access & O_RDWR)
        hDest = CreateFileA(fname, GENERIC_READ | GENERIC_WRITE,
          0, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
      else
        hDest = CreateFileA(fname, GENERIC_READ,
          0, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
  else
  	if (access & O_RDWR)
            hDest = CreateFileA(fname, GENERIC_READ | GENERIC_WRITE,
	    	0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    else
            hDest = CreateFileA(fname, GENERIC_READ,
	    	0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return hDest;
#else
    return open(fname, access, mode);
#endif
}

//
//
//
BOOL nb_close(HANDLE hDest)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
    return CloseHandle(hDest);
 #else
    return close((int)hDest);
 #endif
}

//
//
//
DWORD nb_write(HANDLE h, LPCVOID buf, DWORD size)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
    DWORD reallen;
	WriteFile(h, buf, size, &reallen, NULL);
    return reallen;
#else
    return write((int)h, buf, size);
 #endif
}

//
//
//
DWORD nb_read(HANDLE h, LPCVOID buf, DWORD size)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
    DWORD reallen=0;
    ReadFile(h, const_cast<void*>(buf), size, &reallen, NULL);
    return reallen;
 #else
    return read((int)h,buf, size);
  #endif
}

//
//
//
DWORD	nb_lseek(HANDLE handle, long offset, int fromwhere)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)

    DWORD dw;

	switch (fromwhere) {
  case SEEK_CUR:	// Current file pointer position
		dw = SetFilePointer(handle, offset, NULL, FILE_CURRENT);
   	break;

	case SEEK_END:	// End-of-file
		dw = SetFilePointer(handle, 0, NULL, FILE_END);
  	break;

  case SEEK_SET:	// File beginning
	default:
		dw = SetFilePointer(handle, offset, NULL, FILE_BEGIN);
   	break;
  }
  return dw;
#else
  return lseek((int)handle, offset, fromwhere);
#endif
}

//
//
//
int nb_unlink(const char *name)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
  return !DeleteFile((LPCTSTR)name);
#else
    return unlink(name);
#endif
}

//
//
//
int nb_rename(const char *oldname, const char *newname)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
  return !MoveFile((LPCTSTR)oldname, (LPCTSTR)newname);
#else
    return rename(oldname, newname);
#endif
}

//	NB 20/III-01
//
//	fill size and time information about file
BOOL GetFileInfo(LPSTR fname, LPSTR infosize, LPSTR infotime)
{
#if (SG_CURRENT_PLATFORM==SG_PLATFORM_WINDOWS)
    HANDLE fin;
  BY_HANDLE_FILE_INFORMATION fi;
  SYSTEMTIME systime;

	*infosize = *infotime = 0;
    fin = CreateFileA(fname, GENERIC_READ, FILE_SHARE_READ,
  	NULL, OPEN_EXISTING, NULL, NULL);
	if ((int)fin < 0)
  	return FALSE;
  if (!GetFileInformationByHandle(fin, &fi))
  	goto BAD;
  sprintf(infosize, "%lu", fi.nFileSizeLow);
//  AddDelim(infosize, GetIDS(IDS_MSG161)[0]);
  strcat(infosize, GetIDS(IDS_SG309)); 
	if (!FileTimeToSystemTime(&fi.ftLastWriteTime, &systime))
  	goto BAD;
  GetShortDateStr(systime.wDay, systime.wMonth, systime.wYear, infotime);
  strcat(infotime, " ");
  GetShortTimeStr(systime.wHour, systime.wMinute, systime.wSecond, &infotime[strlen(infotime)]);
	CloseHandle(fin);
 	return TRUE;
BAD:
    CloseHandle(fin);
#else
  return FALSE;
#endif
}
