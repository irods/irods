/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* haawSubStructFileDriver.c - Module of the UK HAAW structFile driver.
 */

#include "haawSubStructFileDriver.h"

int
haawSubStructFileCreate (rsComm_t *rsComm, subFile_t *subFile)
{
    return (0);
}

int 
haawSubStructFileOpen (rsComm_t *rsComm, subFile_t *subFile)
{
    return (0);
}

int 
haawSubStructFileRead (rsComm_t *rsComm, int fd, void *buf, int len)
{
    if (len > 0) {
        rstrcpy ((char *) buf, "This is a test msg from server", len);
    }

    return (strlen ("This is a test msg from server") + 1);
}

int
haawSubStructFileWrite (rsComm_t *rsComm, int fd, void *buf, int len)
{
    if (len > 0 && buf != NULL) {
	return (strlen ((char *)buf) + 1);
    } else {
        return (0);
    }
}

int
haawSubStructFileClose (rsComm_t *rsComm, int fd)
{
    return (0);
}

int
haawSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile)
{
    return (0);
}

int
haawSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile,
rodsStat_t **subStructFileStatOut)
{
    *subStructFileStatOut = malloc (sizeof (rodsStat_t));
    (*subStructFileStatOut)->st_size = 1000;
    (*subStructFileStatOut)->st_blocks = 10000;

    /* return -1 or give false success */
    return (-1);
}

int
haawSubStructFileFstat (rsComm_t *rsComm, int fd, 
rodsStat_t **subStructFileStatOut)
{
    *subStructFileStatOut = malloc (sizeof (rodsStat_t));
    (*subStructFileStatOut)->st_size = 1000;
    (*subStructFileStatOut)->st_blocks = 10000;

    return (0);
}

rodsLong_t
haawSubStructFileLseek (rsComm_t *rsComm, int fd, rodsLong_t offset, int whence)
{
    return (offset);
}

int
haawSubStructFileRename (rsComm_t *rsComm, subFile_t *subFile, char *newFileName)
{
    return (123);
}

int
haawSubStructFileMkdir (rsComm_t *rsComm, subFile_t *subFile)
{
    return (0);
}

int
haawSubStructFileRmdir (rsComm_t *rsComm, subFile_t *subFile)
{
    return (0);
}

int
haawSubStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile)
{
    return (0);
}

int
haawSubStructFileReaddir (rsComm_t *rsComm, int fd, rodsDirent_t **rodsDirent)
{
    /* XXXX a return of >= 0 will cause querySpecColl to go into info loop */ 
    return (-1);
}

int
haawSubStructFileClosedir (rsComm_t *rsComm, int fd)
{
    return (0);
}

int
haawSubStructFileTruncate (rsComm_t *rsComm, subFile_t *subFile)
{
    return (0);
}

int
haawStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    return (0);
}

