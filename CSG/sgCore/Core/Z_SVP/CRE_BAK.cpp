#include "../sg.h"
#include "../common_hdr.h"

static char LastBakPathName[MAXPATH];

//    
bool CreateBakFile(char *pathname)
{
	char			drive[MAXDRIVE], dir[MAXDIR], file[MAXFILE], ext[MAXEXT],
						ext2[MAXEXT];

	fnsplit(pathname, drive, dir, file, ext);
	strcpy(ext2, ".~");
  if (strlen(ext) > 0) strncat(ext2, &ext[1], 2);
	fnmerge(LastBakPathName, drive, dir, file, ext2);
	if (!nb_access(LastBakPathName, 0)) {
		//    : 
		nb_unlink(LastBakPathName);
	}
	if (nb_rename(pathname, LastBakPathName)) {
		LastBakPathName[0] = 0;
		put_message(IDS_SG259, pathname, 0);
    return FALSE;
	}
  return TRUE;
}

void RestoreFromBakFile(char *pathname)
//        
{
	if(LastBakPathName[0])
	  if(!nb_access(LastBakPathName, 0))
	    nb_rename(LastBakPathName, pathname);
}
