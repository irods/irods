/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/

/* structFileDriver.c - The general driver for all structFile types. */

#include "structFileDriver.h"
#include "structFileDriverTable.h"
#include "rsGlobalExtern.h"
#include "apiHeaderAll.h"	/* XXXXX no needed open structFile api done */

int
subStructFileCreate (rsComm_t *rsComm, subFile_t *subFile)
{
    structFileType_t myType;
    int subStructFileInx;
    int fd;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }
    
    fd = StructFileDriverTable[subStructFileInx].subStructFileCreate (rsComm, subFile);
    return (fd);
}


int
subStructFileOpen (rsComm_t *rsComm, subFile_t *subFile)
{
    structFileType_t myType;
    int subStructFileInx;
    int fd;
    
    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    fd = StructFileDriverTable[subStructFileInx].subStructFileOpen (rsComm, subFile);
    return (fd);
}

int
subStructFileRead (structFileType_t myType, rsComm_t *rsComm, int fd, void *buf,
int len)
{
    int subStructFileInx;
    int status;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileRead (rsComm, fd, buf, len);
    return (status);
}

int
subStructFileWrite (structFileType_t myType, rsComm_t *rsComm, int fd, void *buf, int len)
{
    int subStructFileInx;
    int status;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileWrite (rsComm, fd, buf, len);
    return (status);
}

int
subStructFileClose (structFileType_t myType, rsComm_t *rsComm, int fd)
{
    int subStructFileInx;
    int status;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileClose (rsComm, fd);
    return (status);
}

int
subStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileUnlink (rsComm, subFile);
    return (status);
}

int
subStructFileStat (rsComm_t *rsComm, subFile_t *subFile,
rodsStat_t **subStructFileStatOut)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileStat (rsComm, subFile,
      subStructFileStatOut);
    return (status);
}

int
subStructFileFstat (structFileType_t myType, rsComm_t *rsComm, int fd,
rodsStat_t **subStructFileStatOut)
{
    int subStructFileInx;
    int status;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileFstat (rsComm, fd, 
      subStructFileStatOut);
    return (status);
}

rodsLong_t
subStructFileLseek (structFileType_t myType, rsComm_t *rsComm, int fd,
rodsLong_t offset, int whence)
{
    int subStructFileInx;
    rodsLong_t status;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return ((rodsLong_t) subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileLseek (rsComm, fd, offset,
      whence);
    return (status);
}

int
subStructFileRename (rsComm_t *rsComm, subFile_t *subFile, char *newFileName)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileRename (rsComm, subFile,
      newFileName);
    return (status);
}

int
subStructFileMkdir (rsComm_t *rsComm, subFile_t *subFile)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileMkdir (rsComm, subFile);
    return (status);
}

int
subStructFileRmdir (rsComm_t *rsComm, subFile_t *subFile)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileRmdir (rsComm, subFile);
    return (status);
}

int
subStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileOpendir (rsComm, subFile);
    return (status);
}

int
subStructFileReaddir (structFileType_t myType, rsComm_t *rsComm, int fd, 
rodsDirent_t **rodsDirent)
{
    int subStructFileInx;
    rodsLong_t status;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return ((rodsLong_t) subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileReaddir (
      rsComm, fd, rodsDirent);
    return (status);
}

int
subStructFileClosedir (structFileType_t myType, rsComm_t *rsComm, int fd)
{
    int subStructFileInx;
    rodsLong_t status;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return ((rodsLong_t) subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileClosedir (rsComm, fd); 
    return (status);
}

int
subStructFileTruncate (rsComm_t *rsComm, subFile_t *subFile)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = subFile->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].subStructFileTruncate (rsComm, subFile);
    return (status);
}

int
subStructFileIndexLookup (structFileType_t myType)
{
    int i;

    for (i = 0; i < NumStructFileDriver; i++) {
        if (myType == StructFileDriverTable[i].type)
            return (i);
    }

    rodsLog (LOG_ERROR, "structFileIndexLookup error for type %d", myType);

    return (FILE_INDEX_LOOKUP_ERR);
}

int
structFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = structFileOprInp->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].structFileSync (rsComm, 
      structFileOprInp);
    return (status);

}

int
structFileExtract (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    structFileType_t myType;
    int subStructFileInx;
    int status;

    myType = structFileOprInp->specColl->type;

    if ((subStructFileInx = subStructFileIndexLookup (myType)) < 0) {
        return (subStructFileInx);
    }

    status = StructFileDriverTable[subStructFileInx].structFileExtract (rsComm,
      structFileOprInp);
    return (status);

}

int
initStructFileDesc ()
{
    memset (StructFileDesc, 0,
      sizeof (structFileDesc_t) * NUM_STRUCT_FILE_DESC);
    return (0);
}

int
allocStructFileDesc ()
{
    int i;

    for (i = 1; i < NUM_STRUCT_FILE_DESC; i++) {
        if (StructFileDesc[i].inuseFlag == FD_FREE) {
            StructFileDesc[i].inuseFlag = FD_INUSE;
            return (i);
        };
    }

    rodsLog (LOG_NOTICE,
     "allocStructFileDesc: out of StructFileDesc");

    return (SYS_OUT_OF_FILE_DESC);
}

int
freeStructFileDesc (int structFileInx)
{
    if (structFileInx < 0 || structFileInx >= NUM_STRUCT_FILE_DESC) {
        rodsLog (LOG_NOTICE,
         "freeStructFileDesc: structFileInx %d out of range", structFileInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    memset (&StructFileDesc[structFileInx], 0, sizeof (structFileDesc_t));

    return (0);
}

