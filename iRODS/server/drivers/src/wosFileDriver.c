/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* wosFileDriver.c - The Wos file driver
 */


#include "wosFileDriver.h"
#include "rsGlobalExtern.h"
#include "wosFunctPP.hpp"



int
wosFileUnlink (rsComm_t *rsComm, char *filename)
{
    int status;

    status = wosFileUnlinkPP (filename);

    return status;
}

int
wosFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf)
{
    rodsLong_t len;

    len = wosGetFileSizePP (filename);

    if (len >= 0) {
	statbuf->st_mode = S_IFREG;
	statbuf->st_nlink = 1;
	statbuf->st_uid = getuid ();
	statbuf->st_gid = getgid ();
        statbuf->st_atime = statbuf->st_mtime = statbuf->st_ctime = time(0);
	statbuf->st_size = len;
    }
    return 0;
}


rodsLong_t
wosFileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag)
{
    int space = LARGE_SPACE;
    return (space * 1024 * 1024);
}

/* wosStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
 * Just copy the file from filename to cacheFilename. optionalInfo info
 * is not used.
 * 
 */
  
int
wosStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
int mode, int flags, char *wosObjName, 
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput)
{
    int status;

    status = wosStageToCachePP (mode, flags, wosObjName, cacheFilename, 
      dataSize);

    return status;
}

/* wosSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
 * Just copy the file from cacheFilename to filename. optionalInfo info
 * is not used.
 *
 */

int
wosSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
int mode, int flags, char *wosObjName,
char *cacheFilename,  rodsLong_t dataSize, keyValPair_t *condInput)
{
    int status;

    status = wosSyncToArchPP (mode, flags, wosObjName, cacheFilename, 
      dataSize);

    return status;
}

