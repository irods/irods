/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* tarSubStructFileDriver.c - Module of the tar structFile driver.
 */

#ifndef TAR_EXEC_PATH
#include <libtar.h>

#ifdef HAVE_LIBZ
# include <zlib.h>
#endif

#include <compat.h>
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

    status = rsFileCreate (rsComm, &fileCreateInp);

    if (status < 0) {
       rodsLog (LOG_ERROR,
          "specCollCreate: rsFileCreate for %s error, status = %d",
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

#if 0	/* no long use tarLogStructFileSync */
int
tarStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    int status;

    if ((structFileOprInp->oprType & LOGICAL_BUNDLE) != 0) {
	status = tarLogStructFileSync (rsComm, structFileOprInp);
    } else {
	status = tarPhyStructFileSync (rsComm, structFileOprInp);
    }
    return (status);
}

/* tarLogStructFileSync - bundle files in the cacheDir UNIX directory,
 */
int
tarLogStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    int structFileInx;
    specColl_t *specColl;
    rescInfo_t *rescInfo;
    collInp_t openCollInp;
    collEnt_t *collEnt = NULL;
    int handleInx;
    int status = 0;
    TAR *t;
    int myMode;
    int collLen;
    int parLen;
    char savepath[MAX_NAME_LEN];
    char topTmpDir[MAX_NAME_LEN];

    structFileInx = rsTarStructFileOpen (rsComm, structFileOprInp->specColl);
    if (structFileInx < 0) {
        rodsLog (LOG_NOTICE,
         "tarLogStructFileSync: rsTarStructFileOpen error for %s, stat=%d",
         structFileOprInp->specColl->collection, structFileInx);
        return (structFileInx);
    }

    rescInfo = StructFileDesc[structFileInx].rescInfo;
    specColl = structFileOprInp->specColl;

    memset (&openCollInp, 0, sizeof (openCollInp));
    rstrcpy (openCollInp.collName, specColl->collection, MAX_NAME_LEN);
    openCollInp.flags =
      RECUR_QUERY_FG | VERY_LONG_METADATA_FG | INCLUDE_CONDINPUT_IN_QUERY;
    addKeyVal (&openCollInp.condInput, RESC_NAME_KW, rescInfo->rescName);

    handleInx = rsOpenCollection (rsComm, &openCollInp);
    if (handleInx < 0) {
        rodsLog (LOG_ERROR,
          "tarLogStructFileSync: rsOpenCollection of %s error. status = %d",
          openCollInp.collName, handleInx);
        return (handleInx);
    }

    myMode = encodeIrodsTarfd (structFileInx, getDefFileMode ());

    status = tar_open (&t, specColl->phyPath, &irodstype,
      O_WRONLY, myMode, TAR_GNU);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "tarLogStructFileSync: tar_open error for %s, errno = %d",
          specColl->phyPath, errno);
        return (SYS_TAR_OPEN_ERR - errno);
    }
    collLen = strlen (openCollInp.collName) + 1;
    parLen = getParentPathlen (openCollInp.collName);
    snprintf (topTmpDir, MAX_NAME_LEN, "/tmp/%s", 
      openCollInp.collName + parLen);

    while ((status = rsReadCollection (rsComm, &handleInx, &collEnt)) >= 0) {
	if (collEnt->objType == DATA_OBJ_T) {
	    if (collEnt->collName[collLen] == '\0') {
	        snprintf (savepath, MAX_NAME_LEN, "./%s", 
	          collEnt->dataName);
	    } else {
	        snprintf (savepath, MAX_NAME_LEN, "./%s/%s", 
	          collEnt->collName + collLen, collEnt->dataName);
	    }
	    status = tar_append_file (t, collEnt->phyPath, savepath);
    	    if (status != 0) {
        	rodsLog (LOG_NOTICE,
      		 "tarLogStructFileSync: tar_append_tree error for %s, errno=%d",
                  savepath, errno);
		rsCloseCollection (rsComm, &handleInx);;
		rmTmpDirAll (topTmpDir);
		tar_close(t);
                return (SYS_TAR_APPEND_ERR - errno);
            }
	} else {	/* a collection */
	    char tmpDir[MAX_NAME_LEN];
	    snprintf (savepath, MAX_NAME_LEN, "./%s",
              collEnt->collName + collLen);
	   /* have to mkdir to fool tar_append_file */
	    snprintf (tmpDir, MAX_NAME_LEN, "/tmp/%s", 
	     collEnt->collName + parLen);
	    mkdirR ("/tmp", tmpDir, getDefDirMode ());
	    status = tar_append_file (t, tmpDir, savepath);
            if (status != 0) {
                rodsLog (LOG_NOTICE,
                 "tarLogStructFileSync: tar_append_tree error for %s, errno=%d",
                  collEnt->collName + collLen, errno);
		rmTmpDirAll (topTmpDir);
		rsCloseCollection (rsComm, &handleInx);
		tar_close(t);
                return (SYS_TAR_APPEND_ERR - errno);
            }
	}
    }

    rmTmpDirAll (topTmpDir);
    rsCloseCollection (rsComm, &handleInx);;
    tar_close(t);

    return 0;
}
#endif
    
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
	rstrcpy (specCollCache->specColl.phyPath, specColl->phyPath, 
	  MAX_NAME_LEN);
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
    status = verifyStructFileDesc (structFileInx, pathname, &specColl);
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

int
extractTarFile (int structFileInx)
{
    int status; 
#ifdef TAR_EXEC_PATH
    status = extractTarFileWithExec (structFileInx);
#else
    status = extractTarFileWithLib (structFileInx);
#endif
    return status;
}

#ifdef TAR_EXEC_PATH
int
extractTarFileWithExec (int structFileInx)
{
    int status;
#if 0
    char cmdStr[MAX_NAME_LEN];
#else
     char *av[NAME_LEN];
#endif
#ifndef GNU_TAR
    char tmpPath[MAX_NAME_LEN];
#endif

    specColl_t *specColl = StructFileDesc[structFileInx].specColl;

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

#if 0
    snprintf (cmdStr, MAX_NAME_LEN, "%s -xf %s -C %s",
      TAR_EXEC_PATH, specColl->phyPath, specColl->cacheDir);

    status = system (cmdStr);

    if (status != 0) {
        rodsLog (LOG_ERROR,
          "extractTarFileWithExec:: tar of %s to %s failed. stat = %d",
          specColl->cacheDir, specColl->phyPath, status);
        status = SYS_EXEC_TAR_ERR;
    }
#else
    bzero (av, sizeof (av));
#ifdef GNU_TAR
    av[0] = TAR_EXEC_PATH;
    av[1] = "-x";
    av[2] = "-f";
    av[3] = specColl->phyPath;
    av[4] = "-C";
    av[5] = specColl->cacheDir;
#else	/* GNU_TAR */
    mkdir (specColl->cacheDir, getDefDirMode ());
    if (getcwd (tmpPath, MAX_NAME_LEN) == NULL) {
        rodsLog (LOG_ERROR,
          "extractTarFileWithExec:: getcwd failed. errno = %d", errno);
	return SYS_EXEC_TAR_ERR - errno;
    }
    chdir (specColl->cacheDir);
    av[0] = TAR_EXEC_PATH;
    av[1] = "-xf";
    av[2] = specColl->phyPath;
#endif	/* GNU_TAR */
    status = forkAndExec (av);
#ifndef GNU_TAR
    chdir (tmpPath);
#endif

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


#ifdef TAR_EXEC_PATH
    status = bundleCacheDirWithExec (structFileInx);
#else
    status = bundleCacheDirWithLib (structFileInx);
#endif
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
bundleCacheDirWithExec (int structFileInx)
{
    int status;
#if 0
    char cmdStr[MAX_NAME_LEN];
#else
     char *av[NAME_LEN];
#endif

    specColl_t *specColl = StructFileDesc[structFileInx].specColl;
    if (specColl == NULL || specColl->cacheDirty <= 0 ||
      strlen (specColl->cacheDir) == 0) return 0;

#if 0
    snprintf (cmdStr, MAX_NAME_LEN, "%s -chlf %s -C %s .",
      TAR_EXEC_PATH, specColl->phyPath, specColl->cacheDir);

    status = system (cmdStr);

    if (status != 0) {
        rodsLog (LOG_ERROR,
          "bundleCacheDirWithExec: tar of %s to %s failed. stat = %d",
          specColl->cacheDir, specColl->phyPath, status);
	status = SYS_EXEC_TAR_ERR;
    }
#else
    bzero (av, sizeof (av));
    av[0] = TAR_EXEC_PATH;
    av[1] = "-chf";
    av[2] = specColl->phyPath;
    av[3] = "-C";
    av[4] = specColl->cacheDir;
    av[5] = ".";

    status = forkAndExec (av);
#endif

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

