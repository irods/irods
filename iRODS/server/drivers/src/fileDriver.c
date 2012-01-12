/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.c - The general driver for all file types. */

#include "fileDriver.h"
#include "fileDriverTable.h"

int 
fileCreate (fileDriverType_t myType, rsComm_t *rsComm, char *fileName, 
int mode, rodsLong_t mySize)
{
    int fileInx;
    int fd;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
	return (fileInx);
    }

    fd = FileDriverTable[fileInx].fileCreate (rsComm, fileName, mode, mySize);  
    return (fd);
}

int
fileOpen (fileDriverType_t myType, rsComm_t *rsComm, char *fileName, int flags, int mode)
{
    int fileInx;
    int fd;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    fd = FileDriverTable[fileInx].fileOpen (rsComm, fileName, flags, mode);
    return (fd);
}

int
fileRead (fileDriverType_t myType, rsComm_t *rsComm, int fd, void *buf, 
int len)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileRead (rsComm, fd, buf, len);
    return (status);
}

int
fileWrite (fileDriverType_t myType, rsComm_t *rsComm, int fd, void *buf, 
int len)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileWrite (rsComm, fd, buf, len);
    return (status);
}

int 
fileClose (fileDriverType_t myType, rsComm_t *rsComm, int fd)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileClose (rsComm, fd);
    return (status);
}

int
fileUnlink (fileDriverType_t myType, rsComm_t *rsComm, char *filename)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileUnlink (rsComm, filename);
    return (status);
}

int
fileStat (fileDriverType_t myType, rsComm_t *rsComm, char *filename, 
struct stat *statbuf)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileStat (rsComm, filename, statbuf);
    return (status);
}

int
fileFstat (fileDriverType_t myType, rsComm_t *rsComm, int fd, 
struct stat *statbuf)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileFstat (rsComm, fd, statbuf);
    return (status);
}

rodsLong_t
fileLseek (fileDriverType_t myType, rsComm_t *rsComm, int fd, 
rodsLong_t offset, int whence)
{
    int fileInx;
    rodsLong_t status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileLseek (rsComm, fd, offset, whence);
    return (status);
}

int
fileFsync (fileDriverType_t myType, rsComm_t *rsComm, int fd)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileFsync (rsComm, fd);
    return (status);
}

int
fileMkdir (fileDriverType_t myType, rsComm_t *rsComm, char *filename, int mode)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileMkdir (rsComm, filename, mode);
    return (status);
}

int
fileChmod (fileDriverType_t myType, rsComm_t *rsComm, char *filename, int mode)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileChmod (rsComm, filename, mode);
    return (status);
}

int
fileRmdir (fileDriverType_t myType, rsComm_t *rsComm, char *filename)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileRmdir (rsComm, filename);
    return (status);
}

int
fileOpendir (fileDriverType_t myType, rsComm_t *rsComm, char *filename,
void **outDirPtr)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileOpendir (rsComm, filename, 
      outDirPtr);

    return (status);
}

int
fileClosedir (fileDriverType_t myType, rsComm_t *rsComm, void *dirPtr)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileClosedir (rsComm, dirPtr);

    return (status);
}

int
fileReaddir (fileDriverType_t myType, rsComm_t *rsComm, void *dirPtr, 
struct dirent *direntPtr)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    bzero (direntPtr, sizeof (struct dirent));

    status = FileDriverTable[fileInx].fileReaddir (rsComm, dirPtr, direntPtr);

    return (status);
}

int
fileStage (fileDriverType_t myType, rsComm_t *rsComm, char *path, int flag)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileStage (rsComm, path, flag);

    return (status);
}

int
fileRename (fileDriverType_t myType, rsComm_t *rsComm, char *oldFileName,
char *newFileName)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileRename (rsComm, oldFileName, 
      newFileName);

    return (status);
}

rodsLong_t
fileGetFsFreeSpace (fileDriverType_t myType, rsComm_t *rsComm, 
char *path, int flag)
{
    int fileInx;
    rodsLong_t status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileGetFsFreeSpace (rsComm, path, flag);

    return (status);
}

int
fileTruncate (fileDriverType_t myType, rsComm_t *rsComm, char *path,
rodsLong_t dataSize)
{
    int fileInx;
    rodsLong_t status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileTruncate (rsComm, path, dataSize);

    return (status);
}

int
fileIndexLookup (fileDriverType_t myType)
{
    int i;

    for (i = 0; i < NumFileDriver; i++) {
	if (myType == FileDriverTable[i].driverType)
	    return (i);
    }
    rodsLog (LOG_NOTICE, "fileIndexLookup error for type %d", myType);

    return (FILE_INDEX_LOOKUP_ERR);
}

int
fileStageToCache (fileDriverType_t myType, rsComm_t *rsComm, 
fileDriverType_t cacheFileType, int mode, int flags,
char *filename, char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileStageToCache (rsComm, cacheFileType,
      mode, flags, filename, cacheFilename, dataSize, condInput);

    return (status);
}

int
fileSyncToArch (fileDriverType_t myType, rsComm_t *rsComm, 
fileDriverType_t cacheFileType, int mode, int flags,
char *filename, char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput)
{
    int fileInx;
    int status;

    if ((fileInx = fileIndexLookup (myType)) < 0) {
        return (fileInx);
    }

    status = FileDriverTable[fileInx].fileSyncToArch (rsComm, cacheFileType,
      mode, flags, filename, cacheFilename, dataSize, condInput);

    return (status);
}

