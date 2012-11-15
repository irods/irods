
#if 0

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* tarSubStructFileDriver.c - Module of the tar structFile driver.
 */
#if 1
#ifndef TAR_EXEC_PATH
#include "libtar.h"

#ifdef HAVE_LIBZ
# include <zlib.h>
#endif

// JMC - #include <compat.h>
#endif	/* TAR_EXEC_PATH */
#ifndef windows_platform
#include <sys/wait.h>
#else
#include "Unix2Nt.h"
#endif
#include "tarSubStructFileDriver.h"
#include "rsGlobalExtern.h"
#include "modColl.h"
#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "collection.h"
#include "specColl.h"
#include "resource.h"
#include "miscServerFunct.h"
#include "physPath.h"

int
rmTmpDirAll (char *myDir);

#ifndef TAR_EXEC_PATH
/* prototype here because typedef TAR */
int
writeDirToTarStruct (rsComm_t *rsComm, char *fileDir, int fileType,
specColl_t *specColl, TAR *t);

tartype_t irodstype = { (openfunc_t) irodsTarOpen, (closefunc_t) irodsTarClose,
        (readfunc_t) irodsTarRead, (writefunc_t) irodsTarWrite
};
#endif

int
tarSubStructFileCreate (rsComm_t *rsComm, subFile_t *subFile)
{
    int structFileInx;
    specColl_t *specColl;
    int subInx;
    int rescTypeInx;
    int status;
    fileCreateInp_t fileCreateInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileCreate: rsTarStructFileOpen error for %s, stat=%d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    subInx = allocTarSubFileDesc ();

    if (subInx < 0) return subInx;

    TarSubFileDesc[subInx].structFileInx = structFileInx;

    memset (&fileCreateInp, 0, sizeof (fileCreateInp));
    status = getSubStructFilePhyPath (fileCreateInp.fileName, specColl,
      subFile->subFilePath);
    if (status < 0) return status;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileCreateInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    rstrcpy (fileCreateInp.addr.hostAddr,
    StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);
    fileCreateInp.mode = subFile->mode;
    fileCreateInp.flags = subFile->flags;
    fileCreateInp.otherFlags = NO_CHK_PERM_FLAG; // JMC - backport 4768

    status = rsFileCreate (rsComm, &fileCreateInp);

    if (status < 0) {
       rodsLog (LOG_ERROR,
          "tarSubStructFileCreate: rsFileCreate for %s error, status = %d",
          fileCreateInp.fileName, status);
        return status;
    } else {
        TarSubFileDesc[subInx].fd = status;
        StructFileDesc[structFileInx].openCnt++;
        return (subInx);
    }
}

int 
tarSubStructFileOpen (rsComm_t *rsComm, subFile_t *subFile)
{
    int structFileInx;
    specColl_t *specColl;
    int subInx;
    int rescTypeInx;
    int status;
    fileOpenInp_t fileOpenInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileOpen: rsTarStructFileOpen error for %s, status = %d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    subInx = allocTarSubFileDesc ();

    if (subInx < 0) return subInx;

    TarSubFileDesc[subInx].structFileInx = structFileInx;

    memset (&fileOpenInp, 0, sizeof (fileOpenInp));
    status = getSubStructFilePhyPath (fileOpenInp.fileName, specColl,
      subFile->subFilePath);
    if (status < 0) return status;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileOpenInp.fileType = (fileDriverType_t)UNIX_FILE_TYPE;	/* the only type for cache */
    rstrcpy (fileOpenInp.addr.hostAddr,
    StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);
    fileOpenInp.mode = subFile->mode;
    fileOpenInp.flags = subFile->flags;

    status = rsFileOpen (rsComm, &fileOpenInp);

    if (status < 0) {
       rodsLog (LOG_ERROR,
          "specCollOpen: rsFileOpen for %s error, status = %d",
          fileOpenInp.fileName, status);
        return status;
    } else {
        TarSubFileDesc[subInx].fd = status;
        StructFileDesc[structFileInx].openCnt++;
        return (subInx);
    }
}

int 
tarSubStructFileRead (rsComm_t *rsComm, int subInx, void *buf, int len)
{
    fileReadInp_t fileReadInp;
    int status;
    bytesBuf_t fileReadOutBBuf;

    if (subInx < 1 || subInx >= NUM_TAR_SUB_FILE_DESC ||
      TarSubFileDesc[subInx].inuseFlag == 0) {
        rodsLog (LOG_ERROR,
         "tarSubStructFileRead: subInx %d out of range", subInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    memset (&fileReadInp, 0, sizeof (fileReadInp));
    memset (&fileReadOutBBuf, 0, sizeof (fileReadOutBBuf));
    fileReadInp.fileInx = TarSubFileDesc[subInx].fd;
    fileReadInp.len = len;
    fileReadOutBBuf.buf = buf;
    status = rsFileRead (rsComm, &fileReadInp, &fileReadOutBBuf);

    return (status);
}

int
tarSubStructFileWrite (rsComm_t *rsComm, int subInx, void *buf, int len)
{
    fileWriteInp_t fileWriteInp;
    int status;
    bytesBuf_t fileWriteOutBBuf;

    if (subInx < 1 || subInx >= NUM_TAR_SUB_FILE_DESC ||
      TarSubFileDesc[subInx].inuseFlag == 0) {
        rodsLog (LOG_ERROR,
         "tarSubStructFileWrite: subInx %d out of range", subInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    memset (&fileWriteInp, 0, sizeof (fileWriteInp));
    memset (&fileWriteOutBBuf, 0, sizeof (fileWriteOutBBuf));
    fileWriteInp.fileInx = TarSubFileDesc[subInx].fd;
    fileWriteInp.len = fileWriteOutBBuf.len = len;
    fileWriteOutBBuf.buf = buf;
    status = rsFileWrite (rsComm, &fileWriteInp, &fileWriteOutBBuf);

    if (status > 0) {
	specColl_t *specColl;
	int status1;
	int structFileInx = TarSubFileDesc[subInx].structFileInx;
	/* cache has been written */
	specColl = StructFileDesc[structFileInx].specColl;
	if (specColl->cacheDirty == 0) {
	    specColl->cacheDirty = 1;    
	    status1 = modCollInfo2 (rsComm, specColl, 0);
	    if (status1 < 0) return status1;
        }
    }
    return (status);
}

int
tarSubStructFileClose (rsComm_t *rsComm, int subInx)
{
    fileCloseInp_t fileCloseInp;
    int structFileInx;
    int status;

    if (subInx < 1 || subInx >= NUM_TAR_SUB_FILE_DESC ||
      TarSubFileDesc[subInx].inuseFlag == 0) {
        rodsLog (LOG_ERROR,
         "tarSubStructFileClose: subInx %d out of range",
          subInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    fileCloseInp.fileInx = TarSubFileDesc[subInx].fd;
    status = rsFileClose (rsComm, &fileCloseInp);

    structFileInx = TarSubFileDesc[subInx].structFileInx;
    StructFileDesc[structFileInx].openCnt++;
    freeTarSubFileDesc (subInx);

    return (status);
}

int
tarSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile)
{
    int structFileInx;
    specColl_t *specColl;
    int rescTypeInx;
    int status;
    fileUnlinkInp_t fileUnlinkInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileCreate: rsTarStructFileOpen error for %s, stat=%d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    memset (&fileUnlinkInp, 0, sizeof (fileUnlinkInp));
    status = getSubStructFilePhyPath (fileUnlinkInp.fileName, specColl,
      subFile->subFilePath);
    if (status < 0) return status;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;

    fileUnlinkInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    rstrcpy (fileUnlinkInp.addr.hostAddr,
    StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);

    status = rsFileUnlink (rsComm, &fileUnlinkInp);
    if (status >= 0) {
        specColl_t *specColl;
        int status1;
        /* cache has been written */
        specColl = StructFileDesc[structFileInx].specColl;
        if (specColl->cacheDirty == 0) {
            specColl->cacheDirty = 1;
            status1 = modCollInfo2 (rsComm, specColl, 0);
            if (status1 < 0) return status1;
        }
    }

    return (0);
}

int
tarSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile,
rodsStat_t **subStructFileStatOut)
{
    specColl_t *specColl;
    int structFileInx;
    int rescTypeInx;
    int status; 
    fileStatInp_t fileStatInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileStat: rsTarStructFileOpen error for %s, status = %d",
	  specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    memset (&fileStatInp, 0, sizeof (fileStatInp));

    status = getSubStructFilePhyPath (fileStatInp.fileName, specColl, 
      subFile->subFilePath);
    if (status < 0) return status;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileStatInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    rstrcpy (fileStatInp.addr.hostAddr,  
    StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);

    status = rsFileStat (rsComm, &fileStatInp, subStructFileStatOut);

    return (status);
}

int
tarSubStructFileFstat (rsComm_t *rsComm, int subInx, 
rodsStat_t **subStructFileStatOut)
{
    fileFstatInp_t fileFstatInp;
    int status;

    if (subInx < 1 || subInx >= NUM_TAR_SUB_FILE_DESC ||
      TarSubFileDesc[subInx].inuseFlag == 0) {
        rodsLog (LOG_ERROR,
         "tarSubStructFileFstat: subInx %d out of range", subInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    memset (&fileFstatInp, 0, sizeof (fileFstatInp));
    fileFstatInp.fileInx = TarSubFileDesc[subInx].fd;
    status = rsFileFstat (rsComm, &fileFstatInp, subStructFileStatOut);

    return (status);
}

rodsLong_t
tarSubStructFileLseek (rsComm_t *rsComm, int subInx, rodsLong_t offset, 
int whence)
{
    fileLseekInp_t fileLseekInp;
    int status;
    fileLseekOut_t *fileLseekOut = NULL;

    if (subInx < 1 || subInx >= NUM_TAR_SUB_FILE_DESC ||
      TarSubFileDesc[subInx].inuseFlag == 0) {
        rodsLog (LOG_ERROR,
         "tarSubStructFileLseek: subInx %d out of range", subInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    memset (&fileLseekInp, 0, sizeof (fileLseekInp));
    fileLseekInp.fileInx = TarSubFileDesc[subInx].fd;
    fileLseekInp.offset = offset;
    fileLseekInp.whence = whence;
    status = rsFileLseek (rsComm, &fileLseekInp, &fileLseekOut);

    if (status < 0 || NULL == fileLseekOut ) { // JMC cppcheck - nullptr
        return ((rodsLong_t) status);
    } else {
        rodsLong_t offset = fileLseekOut->offset;
        free (fileLseekOut);
        return (offset);
    }
}

int
tarSubStructFileRename (rsComm_t *rsComm, subFile_t *subFile, 
char *newFileName)
{
    int structFileInx;
    specColl_t *specColl;
    int rescTypeInx;
    int status;
    fileRenameInp_t fileRenameInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileRename: rsTarStructFileOpen error for %s, stat=%d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;

    memset (&fileRenameInp, 0, sizeof (fileRenameInp));
    fileRenameInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    status = getSubStructFilePhyPath (fileRenameInp.oldFileName, specColl,
      subFile->subFilePath);
    if (status < 0) return status;
    status = getSubStructFilePhyPath (fileRenameInp.newFileName, specColl,
      newFileName);
    if (status < 0) return status;
    rstrcpy (fileRenameInp.addr.hostAddr,
      StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);
    status = rsFileRename (rsComm, &fileRenameInp);

    if (status >= 0) {
        int status1;
	/* use the specColl in StructFileDesc */
	specColl = StructFileDesc[structFileInx].specColl;
        /* cache has been written */
        if (specColl->cacheDirty == 0) {
            specColl->cacheDirty = 1;
            status1 = modCollInfo2 (rsComm, specColl, 0);
            if (status1 < 0) return status1;
        }
    }

    return status;
}

int
tarSubStructFileMkdir (rsComm_t *rsComm, subFile_t *subFile)
{
    specColl_t *specColl;
    int structFileInx;
    int rescTypeInx;
    int status;
    fileMkdirInp_t fileMkdirInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileMkdir: rsTarStructFileOpen error for %s,stat=%d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileMkdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    status = getSubStructFilePhyPath (fileMkdirInp.dirName, specColl,
      subFile->subFilePath);
    if (status < 0) return status;

    rstrcpy (fileMkdirInp.addr.hostAddr,
      StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);
    fileMkdirInp.mode = subFile->mode;
    status = rsFileMkdir (rsComm, &fileMkdirInp);

    if (status >= 0) {
        int status1;
        /* use the specColl in StructFileDesc */
        specColl = StructFileDesc[structFileInx].specColl;
        /* cache has been written */
        if (specColl->cacheDirty == 0) {
            specColl->cacheDirty = 1;
            status1 = modCollInfo2 (rsComm, specColl, 0);
            if (status1 < 0) return status1;
        }
    }
    return status;
}

int
tarSubStructFileRmdir (rsComm_t *rsComm, subFile_t *subFile)
{
    specColl_t *specColl;
    int structFileInx;
    int rescTypeInx;
    int status;
    fileRmdirInp_t fileRmdirInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileRmdir: rsTarStructFileOpen error for %s,stat=%d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileRmdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    status = getSubStructFilePhyPath (fileRmdirInp.dirName, specColl,
      subFile->subFilePath);
    if (status < 0) return status;

    rstrcpy (fileRmdirInp.addr.hostAddr,
      StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);
    status = rsFileRmdir (rsComm, &fileRmdirInp);

    if (status >= 0) {
        int status1;
        /* use the specColl in StructFileDesc */
        specColl = StructFileDesc[structFileInx].specColl;
        /* cache has been written */
        if (specColl->cacheDirty == 0) {
            specColl->cacheDirty = 1;
            status1 = modCollInfo2 (rsComm, specColl, 0);
            if (status1 < 0) return status1;
        }
    }
    return status;
}

int
tarSubStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile)
{
    specColl_t *specColl;
    int structFileInx;
    int subInx;
    int rescTypeInx;
    int status;
    fileOpendirInp_t fileOpendirInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileOpendir: rsTarStructFileOpen error for %s,stat=%d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    subInx = allocTarSubFileDesc ();

    if (subInx < 0) return subInx;

    TarSubFileDesc[subInx].structFileInx = structFileInx;
    
    memset (&fileOpendirInp, 0, sizeof (fileOpendirInp));
    status = getSubStructFilePhyPath (fileOpendirInp.dirName, specColl, 
      subFile->subFilePath);
    if (status < 0) return status;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileOpendirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    rstrcpy (fileOpendirInp.addr.hostAddr,
    StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);

    status = rsFileOpendir (rsComm, &fileOpendirInp);
    if (status < 0) {
       rodsLog (LOG_ERROR,
          "specCollOpendir: rsFileOpendir for %s error, status = %d",
          fileOpendirInp.dirName, status);
        return status;
    } else {
	TarSubFileDesc[subInx].fd = status;
	StructFileDesc[structFileInx].openCnt++;
        return (subInx);
    }
}

int
tarSubStructFileReaddir (rsComm_t *rsComm, int subInx, 
rodsDirent_t **rodsDirent)
{
    fileReaddirInp_t fileReaddirInp;
    int status;

    if (subInx < 1 || subInx >= NUM_TAR_SUB_FILE_DESC ||
      TarSubFileDesc[subInx].inuseFlag == 0) {
        rodsLog (LOG_ERROR,
         "tarSubStructFileReaddir: subInx %d out of range",
	  subInx);
	return (SYS_STRUCT_FILE_DESC_ERR);
    }
       
    fileReaddirInp.fileInx = TarSubFileDesc[subInx].fd;
    status = rsFileReaddir (rsComm, &fileReaddirInp, rodsDirent);

    return (status);
}

int
tarSubStructFileClosedir (rsComm_t *rsComm, int subInx)
{
    fileClosedirInp_t fileClosedirInp;
    int structFileInx;
    int status;

    if (subInx < 1 || subInx >= NUM_TAR_SUB_FILE_DESC ||
      TarSubFileDesc[subInx].inuseFlag == 0) {
        rodsLog (LOG_ERROR,
         "tarSubStructFileClosedir: subInx %d out of range",
          subInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }
    
    fileClosedirInp.fileInx = TarSubFileDesc[subInx].fd;
    status = rsFileClosedir (rsComm, &fileClosedirInp);
    
    structFileInx = TarSubFileDesc[subInx].structFileInx;
    StructFileDesc[structFileInx].openCnt++;
    freeTarSubFileDesc (subInx);
    
    return (status);
}

int
tarSubStructFileTruncate (rsComm_t *rsComm, subFile_t *subFile)
{
    specColl_t *specColl;
    int structFileInx;
    int rescTypeInx;
    int status;
    fileOpenInp_t fileTruncateInp;

    specColl = subFile->specColl;
    structFileInx = rsTarStructFileOpen (rsComm, specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarSubStructFileTruncate: rsTarStructFileOpen error for %s,stat=%d",
          specColl->objPath, structFileInx);
        return (structFileInx);
    }

    /* use the cached specColl. specColl may have changed */
    specColl = StructFileDesc[structFileInx].specColl;

    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileTruncateInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    status = getSubStructFilePhyPath (fileTruncateInp.fileName, specColl,
      subFile->subFilePath);
    if (status < 0) return status;

    rstrcpy (fileTruncateInp.addr.hostAddr,
      StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);
    fileTruncateInp.dataSize = subFile->offset;
    status = rsFileTruncate (rsComm, &fileTruncateInp);

    if (status >= 0) {
        int status1;
        /* use the specColl in StructFileDesc */
        specColl = StructFileDesc[structFileInx].specColl;
        /* cache has been written */
        if (specColl->cacheDirty == 0) {
            specColl->cacheDirty = 1;
            status1 = modCollInfo2 (rsComm, specColl, 0);
            if (status1 < 0) return status1;
        }
    }
    return status;
}

    
int
rmTmpDirAll (char *myDir)
{
    char childDir[MAX_NAME_LEN];
    DIR *dp;
    struct dirent *dent;

    dp = opendir(myDir);
    if (dp == NULL) return -1;

    while ((dent = readdir(dp)) != NULL) {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

	snprintf(childDir, MAX_NAME_LEN, "%s/%s", myDir, dent->d_name);
    
        rmTmpDirAll (childDir);
    }
    rmdir (myDir);
    closedir( dp ); // JMC cppcheck - resource
    return (0);
}


/* this is the driver handler for structFileSync */

/* tarStructFileSync - bundle files in the cacheDir UNIX directory,
 */
int
tarStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    int structFileInx;
    specColl_t *specColl;
    rescInfo_t *rescInfo;
    fileRmdirInp_t fileRmdirInp;
	 char *dataType; // JMC - backport 4635
    int status = 0;

    structFileInx = rsTarStructFileOpen (rsComm, structFileOprInp->specColl);

    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarPhyStructFileSync: rsTarStructFileOpen error for %s, stat=%d",
         structFileOprInp->specColl->collection, structFileInx);
        return (structFileInx);
    }

    if (StructFileDesc[structFileInx].openCnt > 0) {
	return (SYS_STRUCT_FILE_BUSY_ERR);
    }

    /* use the specColl in StructFileDesc. More up to date */
    rescInfo = StructFileDesc[structFileInx].rescInfo;
    specColl = StructFileDesc[structFileInx].specColl;
    if ((structFileOprInp->oprType & DELETE_STRUCT_FILE) != 0) {
	/* remove cache and the struct file */
	freeStructFileDesc (structFileInx);
	return (status);
    }
    if ((dataType = getValByKey (&structFileOprInp->condInput, DATA_TYPE_KW)) // JMC - backport 4635
      != NULL) {
        rstrcpy (StructFileDesc[structFileInx].dataType, dataType, NAME_LEN);
    }

    if (strlen (specColl->cacheDir) > 0) {
	if (specColl->cacheDirty > 0) {
	    /* write the tar file and register no dirty */
	    status = syncCacheDirToTarfile (structFileInx, 
	      structFileOprInp->oprType);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "tarPhyStructFileSync:syncCacheDirToTarfile %s error,stat=%d",
                  specColl->cacheDir, status);
		freeStructFileDesc (structFileInx);
                return (status);
            }
	    specColl->cacheDirty = 0;
	    if ((structFileOprInp->oprType & NO_REG_COLL_INFO) == 0) {
	        status = modCollInfo2 (rsComm, specColl, 0);
                if (status < 0) {
		    freeStructFileDesc (structFileInx);
		    return status;
		}
	    }
	}

        if ((structFileOprInp->oprType & PURGE_STRUCT_FILE_CACHE) != 0) {
            /* unregister cache before remove */
	    status = modCollInfo2 (rsComm, specColl, 1);
	    if (status < 0) {
		freeStructFileDesc (structFileInx);
		return status;
	    }
            /* remove cache */
            memset (&fileRmdirInp, 0, sizeof (fileRmdirInp));
            fileRmdirInp.fileType = UNIX_FILE_TYPE;  /* the type for cache */
            rstrcpy (fileRmdirInp.dirName, specColl->cacheDir,
              MAX_NAME_LEN);
            rstrcpy (fileRmdirInp.addr.hostAddr, rescInfo->rescLoc, NAME_LEN);
	    fileRmdirInp.flags = RMDIR_RECUR;
            status = rsFileRmdir (rsComm, &fileRmdirInp);
	    if (status < 0) {
                rodsLog (LOG_ERROR,
                  "tarPhyStructFileSync: XXXXX Rmdir error for %s, status = %d",
	          specColl->cacheDir, status);
		freeStructFileDesc (structFileInx);
                return (status);
	    }
        }
    }
    freeStructFileDesc (structFileInx);
    return (status);
}

/* tarStructFileExtract - this is the handler for "structFileExtract"
 * called by rsStructFileExtract.
 */

int
tarStructFileExtract (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    int structFileInx;
    int status;
    specColl_t *specColl;
    char* dataType; // JMC - backport 4635

    if (rsComm == NULL || structFileOprInp == NULL || 
      structFileOprInp->specColl == NULL) {
        rodsLog (LOG_ERROR,
          "tarStructFileExtract: NULL input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    specColl = structFileOprInp->specColl;

    if ((structFileInx = allocStructFileDesc ()) < 0) {
        return (structFileInx);
    }

    StructFileDesc[structFileInx].specColl = specColl;
    StructFileDesc[structFileInx].rsComm = rsComm;

    status = resolveResc (StructFileDesc[structFileInx].specColl->resource,
      &StructFileDesc[structFileInx].rescInfo);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "tarStructFileExtract: resolveResc error for %s, status = %d",
          specColl->resource, status);
        freeStructFileDesc (structFileInx);
        return (status);
    }
    if ((dataType = getValByKey (&structFileOprInp->condInput, DATA_TYPE_KW))  // JMC - backport 4635
      != NULL) {
       rstrcpy (StructFileDesc[structFileInx].dataType, dataType, NAME_LEN);
    }
    status = extractTarFile (structFileInx);
    if (status < 0) {
        rodsLog (LOG_ERROR,
         "tarStructFileExtract:extract error for %s in cacheDir %s,errno=%d",
             specColl->objPath, specColl->cacheDir, errno);
            /* XXXX may need to remove the cacheDir too */
        status = SYS_TAR_STRUCT_FILE_EXTRACT_ERR - errno;
    }
    freeStructFileDesc (structFileInx);
    return (status);
}

int
rsTarStructFileOpen (rsComm_t *rsComm, specColl_t *specColl)
{
    int structFileInx;
    int status;
    specCollCache_t *specCollCache;

    if (specColl == NULL) {
        rodsLog (LOG_NOTICE,
         "rsTarStructFileOpen: NULL specColl input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    /* look for opened StructFileDesc */

    structFileInx = matchStructFileDesc (specColl);

    if (structFileInx > 0) return structFileInx;
 
    if ((structFileInx = allocStructFileDesc ()) < 0) {
        return (structFileInx);
    }

    /* Have to do this because specColl could come from a remote host */
    if ((status = getSpecCollCache (rsComm, specColl->collection, 0,
      &specCollCache)) >= 0) {
	StructFileDesc[structFileInx].specColl = &specCollCache->specColl;
	/* getSpecCollCache does not give phyPath nor resource */
    if (strlen (specColl->phyPath) > 0) { // JMC - backport 4517
        rstrcpy (specCollCache->specColl.phyPath, specColl->phyPath, MAX_NAME_LEN);
	}
	if (strlen (specCollCache->specColl.resource) == 0) {
	    rstrcpy (specCollCache->specColl.resource, specColl->resource,
	      NAME_LEN);
	}
    } else {
	StructFileDesc[structFileInx].specColl = specColl;
    }

    StructFileDesc[structFileInx].rsComm = rsComm;

    status = resolveResc (StructFileDesc[structFileInx].specColl->resource, 
      &StructFileDesc[structFileInx].rescInfo);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "rsTarStructFileOpen: resolveResc error for %s, status = %d",
          specColl->resource, status);
	freeStructFileDesc (structFileInx);
        return (status);
    }

    /* XXXXX need to deal with remote open here */

    status = stageTarStructFile (structFileInx);

    if (status < 0) {
	freeStructFileDesc (structFileInx);
	return status;
    }

    return (structFileInx);
}

int
matchStructFileDesc (specColl_t *specColl)
{
    int i;

    for (i = 1; i < NUM_STRUCT_FILE_DESC; i++) {
        if (StructFileDesc[i].inuseFlag == FD_INUSE &&
	  StructFileDesc[i].specColl != NULL &&
	   strcmp (StructFileDesc[i].specColl->collection, specColl->collection)	    == 0 &&
	    strcmp (StructFileDesc[i].specColl->objPath, specColl->objPath) 
	    == 0) {
            return (i);
        };
    }

    return (SYS_OUT_OF_FILE_DESC);

}

int
stageTarStructFile (int structFileInx)
{
    int status;
    specColl_t *specColl; 
    rsComm_t *rsComm;

    rsComm = StructFileDesc[structFileInx].rsComm;
    specColl = StructFileDesc[structFileInx].specColl;
    if (specColl == NULL) {
        rodsLog (LOG_NOTICE,
         "stageTarStructFile: NULL specColl input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strlen (specColl->cacheDir) == 0) {
        /* no cache. stage one. make the CacheDir first */
        status = mkTarCacheDir (structFileInx);
	if (status < 0) return status;

        status = extractTarFile (structFileInx);
        if (status < 0) {
            rodsLog (LOG_NOTICE,
             "stageTarStructFile:extract error for %s in cacheDir %s,errno=%d",
	     specColl->objPath, specColl->cacheDir, errno);
	    /* XXXX may need to remove the cacheDir too */
	    return SYS_TAR_STRUCT_FILE_EXTRACT_ERR - errno; 
	}
	/* register the CacheDir */
	status = modCollInfo2 (rsComm, specColl, 0);
        if (status < 0) return status;
    }
    return (0);
}

int
mkTarCacheDir (int structFileInx) 
{
    int i = 0;
    int status;
    fileMkdirInp_t fileMkdirInp;
    int rescTypeInx;
    rescInfo_t *rescInfo;
    rsComm_t *rsComm = StructFileDesc[structFileInx].rsComm;
    specColl_t *specColl = StructFileDesc[structFileInx].specColl;

    if (specColl == NULL) {
        rodsLog (LOG_NOTICE, "mkTarCacheDir: NULL specColl input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    rescInfo = StructFileDesc[structFileInx].rescInfo;

    memset (&fileMkdirInp, 0, sizeof (fileMkdirInp));
    rescTypeInx = rescInfo->rescTypeInx;
    fileMkdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
    fileMkdirInp.mode = DEFAULT_DIR_MODE;
    rstrcpy (fileMkdirInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);

    while (1) {
        snprintf (fileMkdirInp.dirName, MAX_NAME_LEN, "%s.%s%d",
          specColl->phyPath, CACHE_DIR_STR, i);
        status = rsFileMkdir (rsComm, &fileMkdirInp);
        if (status >= 0) {
	    break;
	} else {
	    if (getErrno (status) == EEXIST) {
		i++;
		continue;
	    } else {
                return (status);
	    }
        }
    }
    rstrcpy (specColl->cacheDir, fileMkdirInp.dirName, MAX_NAME_LEN);

    return (0);
}

/* this set of codes irodsTarXYZ are used by libtar to perform file level
 * I/O in iRODS */
 
int
irodsTarOpen (char *pathname, int oflags, int mode)
{
    rodsLog( LOG_NOTICE, "!!!!!!!!!!!!! irodsTarOpen !!!!!!!!!" );
    return -1;

    int structFileInx;
    int myMode;
    int status;
    fileOpenInp_t fileOpenInp;
    rescInfo_t *rescInfo;
    specColl_t *specColl = NULL;
    int rescTypeInx;
    int l3descInx;

    /* the upper most 4 bits of mode is the structFileInx */ 
    decodeIrodsTarfd (mode, &structFileInx, &myMode); 
//    status = verifyStructFileDesc (structFileInx, pathname, &specColl);
    if (status < 0 || NULL == specColl ) return -1;	/* tar lib looks for -1 return */ // JMC cppcheck - nullptr

    rescInfo = StructFileDesc[structFileInx].rescInfo;
    rescTypeInx = rescInfo->rescTypeInx;

    memset (&fileOpenInp, 0, sizeof (fileOpenInp));
    fileOpenInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
    rstrcpy (fileOpenInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);
    rstrcpy (fileOpenInp.fileName, specColl->phyPath, MAX_NAME_LEN);
    fileOpenInp.mode = myMode;
    fileOpenInp.flags = oflags;

    l3descInx = rsFileOpen (StructFileDesc[structFileInx].rsComm, 
      &fileOpenInp);
    
    if (l3descInx < 0) {
        rodsLog (LOG_NOTICE, 
	  "irodsTarOpen: rsFileOpen of %s in Resc %s error, status = %d",
	  fileOpenInp.fileName, rescInfo->rescName, l3descInx);
        return (-1);	/* libtar only accept -1 */
    }
    return (encodeIrodsTarfd (structFileInx, l3descInx));
}

int
encodeIrodsTarfd (int upperInt, int lowerInt)
{
    /* use the upper 5 of the 6 bits for upperInt */

    if (upperInt > 60) {	/* 0x7c */
        rodsLog (LOG_NOTICE,
          "encodeIrodsTarfd: upperInt %d larger than 60", lowerInt);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    if ((lowerInt & 0xfc000000) != 0) {
        rodsLog (LOG_NOTICE,
          "encodeIrodsTarfd: lowerInt %d too large", lowerInt);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    return (lowerInt | (upperInt << 26));
    
}

int
decodeIrodsTarfd (int inpInt, int *upperInt, int *lowerInt)
{
    *lowerInt = inpInt & 0x03ffffff;
    *upperInt = (inpInt & 0x7c000000) >> 26;

    return (0);
}

int
irodsTarClose (int fd)
{
    fileCloseInp_t fileCloseInp;
    int structFileInx, l3descInx;
    int status;

    decodeIrodsTarfd (fd, &structFileInx, &l3descInx);
    memset (&fileCloseInp, 0, sizeof (fileCloseInp));
    fileCloseInp.fileInx = l3descInx;
    status = rsFileClose (StructFileDesc[structFileInx].rsComm, 
      &fileCloseInp);

    return (status);
}

int
irodsTarRead (int fd, char *buf, int len)
{
    fileReadInp_t fileReadInp;
    int structFileInx, l3descInx;
    int status;
    bytesBuf_t readOutBBuf;

    decodeIrodsTarfd (fd, &structFileInx, &l3descInx);
    memset (&fileReadInp, 0, sizeof (fileReadInp));
    fileReadInp.fileInx = l3descInx;
    fileReadInp.len = len;
    memset (&readOutBBuf, 0, sizeof (readOutBBuf));
    readOutBBuf.buf = buf;
    status = rsFileRead (StructFileDesc[structFileInx].rsComm, 
      &fileReadInp, &readOutBBuf);

    return (status);

}

int
irodsTarWrite (int fd, char *buf, int len)
{
    fileWriteInp_t fileWriteInp;
    int structFileInx, l3descInx;
    int status;
    bytesBuf_t writeInpBBuf;

    decodeIrodsTarfd (fd, &structFileInx, &l3descInx);
    memset (&fileWriteInp, 0, sizeof (fileWriteInp));
    fileWriteInp.fileInx = l3descInx;
    fileWriteInp.len = len;
    memset (&writeInpBBuf, 0, sizeof (writeInpBBuf));
    writeInpBBuf.buf = buf;
    status = rsFileWrite (StructFileDesc[structFileInx].rsComm, 
      &fileWriteInp, &writeInpBBuf);

    return (status);
}
#if 0
int 
verifyStructFileDesc (int structFileInx, char *tarPathname, 
specColl_t **specColl)
{
    if (StructFileDesc[structFileInx].inuseFlag <= 0) {
        rodsLog (LOG_NOTICE,
          "verifyStructFileDesc: structFileInx %d not in use", structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }
    if (StructFileDesc[structFileInx].specColl == NULL) {
        rodsLog (LOG_NOTICE,
          "verifyStructFileDesc: NULL specColl for structFileInx %d", 
	  structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }
    if (strcmp (StructFileDesc[structFileInx].specColl->phyPath, tarPathname)
      != 0) {
        rodsLog (LOG_NOTICE,
          "verifyStructFileDesc: phyPath %s in Inx %d does not match %s",
          StructFileDesc[structFileInx].specColl->phyPath, structFileInx, 
	  tarPathname);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }
    if (specColl != NULL) {
	*specColl = StructFileDesc[structFileInx].specColl;
    }

    return 0;
}
#endif
int
extractTarFile (int structFileInx)
{
    int status; 
    if( strcmp( StructFileDesc[structFileInx].dataType, ZIP_DT_STR ) == NULL) { // JMC - backport 4637, 4658, - changed != to == due to logic issue
        #ifdef UNZIP_EXEC_PATH // JMC - backport 4639
        status = extractFileWithUnzip (structFileInx);
        #else
	    return SYS_ZIP_FORMAT_NOT_SUPPORTED;
        #endif
    } else {
        #ifdef TAR_EXEC_PATH
        status = extractTarFileWithExec (structFileInx);
        #else
        status = extractTarFileWithLib (structFileInx);
        #endif
	} // JMC - backport 4637
    return status;
}

#ifdef TAR_EXEC_PATH
int
extractTarFileWithExec (int structFileInx)
{
    int status;
     char *av[NAME_LEN];
#ifndef GNU_TAR
    char tmpPath[MAX_NAME_LEN];
#endif
    int inx = 0; // JMC - backport 4635
    
	specColl_t *specColl = StructFileDesc[structFileInx].specColl;
	char *dataType = StructFileDesc[structFileInx].dataType; // JMC - backport 4635

    if (StructFileDesc[structFileInx].inuseFlag <= 0) {
        rodsLog (LOG_NOTICE,
          "extractTarFileWithExec: structFileInx %d not in use", 
	  structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    if (specColl == NULL || strlen (specColl->cacheDir) == 0 ||
     strlen (specColl->phyPath) == 0) {
        rodsLog (LOG_NOTICE,
          "extractTarFileWithExec: Bad specColl for structFileInx %d ",
          structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }
    bzero (av, sizeof (av));
// =-=-=-=-=-=-=-
// JMC - backport 4635
    av[inx] = TAR_EXEC_PATH; 
    inx++; 
#ifdef GNU_TAR
    av[inx] = "-x";
    inx++;
    if (strstr (dataType, GZIP_TAR_DT_STR) != NULL) { // JMC - backport 4658
       av[inx] = "-z";
        inx++;
    } else if (strstr (dataType, BZIP2_TAR_DT_STR) != NULL) { // JMC - backport 4658
        av[inx] = "-j";
        inx++;
    }
#ifdef TAR_EXTENDED_HDR // JMC - backport 4596
    av[inx] = "-E";
    inx++;
#endif
    av[inx] = "-f";
    inx++;
    av[inx] = specColl->phyPath;
    inx++;
    av[inx] = "-C";
    inx++;
    av[inx] = specColl->cacheDir;
#else  /* GNU_TAR */
    if( strstr (dataType, GZIP_TAR_DT_STR) != NULL || // JMC - backport 4658
        strstr (dataType, BZIP2_TAR_DT_STR) != NULL) {
        /* non GNU_TAR don't seem to support -j nor -z option */
        rodsLog (LOG_ERROR,
         "extractTarFileWithExec:gzip/bzip2 %s not supported by non-GNU_TAR",
          specColl->phyPath);
    }
    mkdir (specColl->cacheDir, getDefDirMode ());
    if (getcwd (tmpPath, MAX_NAME_LEN) == NULL) {
        rodsLog (LOG_ERROR,
          "extractTarFileWithExec:: getcwd failed. errno = %d", errno);
	return SYS_EXEC_TAR_ERR - errno;
    }
    chdir (specColl->cacheDir);
#ifdef TAR_EXTENDED_HDR // JMC - backport 4596
    av[inx] = "-xEf";
#else
    av[inx] = "-xf";
#endif
	inx++;
    av[inx] = specColl->phyPath;
#endif	/* GNU_TAR */
    status = forkAndExec (av);
#ifndef GNU_TAR
    chdir (tmpPath);
#endif

    return status;
}

#else	/* TAR_EXEC_PATH */
int
extractTarFileWithLib (int structFileInx)
{
    TAR *t;
    specColl_t *specColl = StructFileDesc[structFileInx].specColl;
    int myMode;
    int status; 
    

    if (StructFileDesc[structFileInx].inuseFlag <= 0) {
        rodsLog (LOG_NOTICE,
          "extractTarFile: structFileInx %d not in use", structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    if (specColl == NULL || strlen (specColl->cacheDir) == 0 ||
     strlen (specColl->phyPath) == 0) {
        rodsLog (LOG_NOTICE,
          "extractTarFile: Bad specColl for structFileInx %d ", 
	  structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }
    myMode = encodeIrodsTarfd (structFileInx, getDefFileMode ());

    if (myMode < 0) {
        return (myMode);
    }

    status = tar_open (&t, specColl->phyPath, &irodstype, 
      O_RDONLY, myMode, TAR_GNU);

    if (status < 0) {
	rodsLog (LOG_NOTICE,
          "extractTarFile: tar_open error for %s, errno = %d",
          specColl->phyPath, errno);
        return (SYS_TAR_OPEN_ERR - errno);
    }

    if (tar_extract_all (t, specColl->cacheDir) != 0) {
        rodsLog (LOG_NOTICE,
          "extractTarFile: tar_extract_all error for %s, errno = %d",
          specColl->phyPath, errno);
        return (SYS_TAR_EXTRACT_ALL_ERR - errno);
    }

    if (tar_close(t) != 0) {
        rodsLog (LOG_NOTICE,
          "extractTarFile: tar_close error for %s, errno = %d",
          specColl->phyPath, errno);
        return (SYS_TAR_CLOSE_ERR - errno);
    }

    return 0;
}
#endif	/*TAR_EXEC_PATH */

/* getSubStructFilePhyPath - get the phy path in the cache dir */
int
getSubStructFilePhyPath (char *phyPath, specColl_t *specColl, 
char *subFilePath)
{
    int len;

    /* subFilePath is composed by appending path to the objPath which is
     * the logical path of the tar file. So we need to substitude the
     * objPath with cacheDir */ 

    len = strlen (specColl->collection);
    if (strncmp (specColl->collection, subFilePath, len) != 0) {
            rodsLog (LOG_NOTICE,
             "getSubStructFilePhyPath: collection %s subFilePath %s mismatch",
              specColl->collection, subFilePath);
            return (SYS_STRUCT_FILE_PATH_ERR);
        }

        snprintf (phyPath, MAX_NAME_LEN, "%s%s", specColl->cacheDir,
         subFilePath + len);


    return (0);
}

int
syncCacheDirToTarfile (int structFileInx, int oprType)
{
    int status;
    int rescTypeInx;
    fileStatInp_t fileStatInp;
    rodsStat_t *fileStatOut = NULL;
    specColl_t *specColl = StructFileDesc[structFileInx].specColl;
    rsComm_t *rsComm = StructFileDesc[structFileInx].rsComm;

    if (strstr (StructFileDesc[structFileInx].dataType, ZIP_DT_STR) != 0) { // JMC - backport 4637
       #ifdef ZIP_EXEC_PATH
        status = bundleCacheDirWithZip (structFileInx );// JMC - backport 4665?  , oprType); // JMC - backport 4643
       #else
       return SYS_ZIP_FORMAT_NOT_SUPPORTED;
       #endif
    } else {
        #ifdef TAR_EXEC_PATH
        status = bundleCacheDirWithExec (structFileInx, oprType); // JMC - backport 4643
        #else

		if ((oprType & ADD_TO_TAR_OPR) != 0) // JMC - backport 4643
		    return SYS_ADD_TO_ARCH_OPR_NOT_SUPPORTED;
        status = bundleCacheDirWithLib (structFileInx);

        #endif
	} // JMC - backport 4637
    if (status < 0) return status;

    /* register size change */
    memset (&fileStatInp, 0, sizeof (fileStatInp));
    rstrcpy (fileStatInp.fileName, specColl->phyPath, MAX_NAME_LEN);
    rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
    fileStatInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
    rstrcpy (fileStatInp.addr.hostAddr,
    StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN);

    status = rsFileStat (rsComm, &fileStatInp, &fileStatOut);

    if (status < 0 || NULL == fileStatOut ) { // JMC cppcheck - nullptr
       rodsLog (LOG_ERROR,
          "syncCacheDirToTarfile: rsFileStat error for %s, status = %d",
          specColl->phyPath, status);
	return (status);
    }

    if ((oprType & NO_REG_COLL_INFO) == 0) {
	/* for bundle opr, done at datObjClose */
        status = regNewObjSize (rsComm, specColl->objPath, specColl->replNum, 
          fileStatOut->st_size);
    }

    free (fileStatOut);

    return (status);
}

#ifdef TAR_EXEC_PATH
int
bundleCacheDirWithExec (int structFileInx, int oprType) // JMC - backport 4643
{
    int status;
    char *av[NAME_LEN];

#ifndef GNU_TAR // JMC - backport 4643
    char optStr[NAME_LEN];
#endif

    char *dataType; // JMC - backport 4635
    int inx = 0; // JMC - backport 4635

    specColl_t *specColl = StructFileDesc[structFileInx].specColl;
    if (specColl == NULL || specColl->cacheDirty <= 0 ||
      strlen (specColl->cacheDir) == 0) return 0;

	// =-=-=-=-=-=-=-
	// JMC - backport 4635
	dataType = StructFileDesc[structFileInx].dataType;
    bzero (av, sizeof (av));
    av[inx] = TAR_EXEC_PATH;
	inx++;
#ifdef GNU_TAR
    if ((oprType & ADD_TO_TAR_OPR) == 0) { // JMC - backport 4643
        av[inx] = "-c";
    } else {
        av[inx] = "-r";
    }

    inx++;
    av[inx] = "-h";
    inx++;
    if (strstr (dataType, GZIP_TAR_DT_STR) != NULL) { // JMC - backport 4658
       if ((oprType & ADD_TO_TAR_OPR) != 0) // JMC - backport 4643
           return SYS_ADD_TO_ARCH_OPR_NOT_SUPPORTED;
        av[inx] = "-z";
        inx++;
    } else if (strstr  (dataType, BZIP2_TAR_DT_STR) != NULL) { // JMC - backport 4658
       if ((oprType & ADD_TO_TAR_OPR) != 0) // JMC - backport 4643
           return SYS_ADD_TO_ARCH_OPR_NOT_SUPPORTED;
        av[inx] = "-j";
        inx++;
    }
#ifdef TAR_EXTENDED_HDR // JMC - backport 4596
    av[inx] = "-E";
#endif
    av[inx] = "-f";
    inx++;
#else  /* GNU_TAR */
    if( strstr (dataType, GZIP_TAR_DT_STR ) != NULL || // JMC - backport 4658
        strstr (dataType, BZIP2_TAR_DT_STR) != NULL) { // JMC - backport 4658
        rodsLog (LOG_ERROR,
         "bundleCacheDirWithExec: gzip/bzip2 %s not supported for non-GNU_TAR",
          specColl->phyPath);
       return SYS_ZIP_FORMAT_NOT_SUPPORTED;
    }
	rstrcpy (optStr, "-h", NAME_LEN); // JMC  - backport 4643, 4645
#ifdef TAR_EXTENDED_HDR
    strcat (optStr, "E"); // JMC - backport 4643
#endif
    if ((oprType & ADD_TO_TAR_OPR) != 0) { // JMC - backport 
       /* update */
       strcat (optStr, "u");
    }  else { // JMC - backport 4645
	    strcat (optStr, "c");
    }

    strcat (optStr, "f");
    av[inx] = optStr;
    inx++;
#endif /* GNU_TAR */
    av[inx] = specColl->phyPath;
    inx++;
    av[inx] = "-C";
    inx++;
    av[inx] = specColl->cacheDir;
    inx++;
    av[inx] = ".";
    status = forkAndExec (av);


    return status;
}

#else 	/* TAR_EXEC_PATH */
int
bundleCacheDirWithLib (int structFileInx)
{
    TAR *t;
    int status;
    int myMode;

    specColl_t *specColl = StructFileDesc[structFileInx].specColl;
    if (specColl == NULL || specColl->cacheDirty <= 0 ||
      strlen (specColl->cacheDir) == 0) return 0;

    myMode = encodeIrodsTarfd (structFileInx, getDefFileMode ());

    status = tar_open (&t, specColl->phyPath, &irodstype,
      O_WRONLY, myMode, TAR_GNU);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "syncCacheDirToTarfile: tar_open error for %s, errno = %d",
          specColl->phyPath, errno);
        return (SYS_TAR_OPEN_ERR - errno);
    }
    status = tar_append_tree (t, specColl->cacheDir, ".");

    if (status != 0) {
        rodsLog (LOG_NOTICE,
          "syncCacheDirToTarfile: tar_append_tree error for %s, errno = %d",
          specColl->cacheDir, errno);
        if (status < 0) {
            return (status);
        } else {
            return (SYS_TAR_APPEND_ERR - errno);
        }
    }

   if ((status = tar_close(t)) != 0) {
        rodsLog (LOG_NOTICE,
          "syncCacheDirToTarfile: tar_close error for %s, errno = %d",
          specColl->phyPath, errno);
        if (status < 0) {
            return (status);
        } else {
            return (SYS_TAR_CLOSE_ERR - errno);
        }
    }

    return 0;
}
#endif 	/* TAR_EXEC_PATH */

int
initTarSubFileDesc ()
{
    memset (TarSubFileDesc, 0,
      sizeof (tarSubFileDesc_t) * NUM_TAR_SUB_FILE_DESC);
    return (0);
}

int
allocTarSubFileDesc ()
{
    int i;

    for (i = 1; i < NUM_TAR_SUB_FILE_DESC; i++) {
        if (TarSubFileDesc[i].inuseFlag == FD_FREE) {
            TarSubFileDesc[i].inuseFlag = FD_INUSE;
            return (i);
        };
    }

    rodsLog (LOG_NOTICE,
     "allocTarSubFileDesc: out of TarSubFileDesc");

    return (SYS_OUT_OF_FILE_DESC);
}

int
freeTarSubFileDesc (int tarSubFileInx)
{
    if (tarSubFileInx < 0 || tarSubFileInx >= NUM_TAR_SUB_FILE_DESC) {
        rodsLog (LOG_NOTICE,
         "freeTarSubFileDesc: tarSubFileInx %d out of range", tarSubFileInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    memset (&TarSubFileDesc[tarSubFileInx], 0, sizeof (tarSubFileDesc_t));

    return (0);
}


#ifdef ZIP_EXEC_PATH // JMC - backport 4639
// =-=-=-=-=-=-=-
// JMC - backport 4637
int 
bundleCacheDirWithZip (int structFileInx)
{
    char *av[NAME_LEN];
    char tmpPath[MAX_NAME_LEN];
    int status;
    int inx = 0;

    specColl_t *specColl = StructFileDesc[structFileInx].specColl;

    if (specColl == NULL || specColl->cacheDirty <= 0 ||
      strlen (specColl->cacheDir) == 0) return 0;

    /* cd to the cacheDir */
    if (getcwd (tmpPath, MAX_NAME_LEN) == NULL) {
        rodsLog (LOG_ERROR,
          "bundleCacheDirWithZip: getcwd failed. errno = %d", errno);
        return SYS_EXEC_TAR_ERR - errno;
    }
    chdir (specColl->cacheDir);
    bzero (av, sizeof (av));
    av[inx] = ZIP_EXEC_PATH;
    inx++;
    av[inx] = "-r";
    inx++;
    av[inx] = "-q";
    inx++;
    av[inx] = specColl->phyPath;
    inx++;
    av[inx] = ".";
    status = forkAndExec (av);
    chdir (tmpPath);

    return status;
}
#endif // JMC - backport 4639

#ifdef ZIP_EXEC_PATH // JMC - backport 4639
int
extractFileWithUnzip (int structFileInx)
{
    char *av[NAME_LEN];
    int status;
    int inx = 0;

    specColl_t *specColl = StructFileDesc[structFileInx].specColl;
    if (StructFileDesc[structFileInx].inuseFlag <= 0) {
        rodsLog (LOG_NOTICE,
          "extractFileWithUnzip: structFileInx %d not in use",
          structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    if (specColl == NULL || strlen (specColl->cacheDir) == 0 ||
     strlen (specColl->phyPath) == 0) {
        rodsLog (LOG_NOTICE,
          "extractFileWithUnzip: Bad specColl for structFileInx %d ",
          structFileInx);
        return (SYS_STRUCT_FILE_DESC_ERR);
    }

    /* cd to the cacheDir */
    bzero (av, sizeof (av));
    av[inx] = UNZIP_EXEC_PATH;
    inx++;
    av[inx] = "-q";
    inx++;
    av[inx] = "-d";
    inx++;
    av[inx] = specColl->cacheDir;
    inx++;
    av[inx] = specColl->phyPath;
    status = forkAndExec (av);

    return status;
}
#endif // JMC - backport 4639
// =-=-=-=-=-=-=-
#endif


#endif
