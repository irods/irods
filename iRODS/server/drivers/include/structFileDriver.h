/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/

/* subStructFileDriver.h - common header subStructFile for subStructFile driver
 */



#ifndef STRUCT_FILE_DRIVER_H
#define STRUCT_FILE_DRIVER_H

#include "rods.h"
#include "rcConnect.h"
#include "objInfo.h"
#include "structFileSync.h"

typedef struct {
    structFileType_t type;
    int              (*subStructFileCreate)( rsComm_t*, subFile_t* ); /* JMC */
    int              (*subStructFileOpen)( rsComm_t*, subFile_t* ); /* JMC */
    int              (*subStructFileRead)( rsComm_t*, int, void*, int ); /* JMC */
    int              (*subStructFileWrite)( rsComm_t*, int, void*, int ); /* JMC */
    int              (*subStructFileClose)( rsComm_t*, int ); /* JMC */
    int              (*subStructFileUnlink)( rsComm_t*, subFile_t* ); /* JMC */
    int              (*subStructFileStat)( rsComm_t*, subFile_t*, rodsStat_t** ); /* JMC */
    int              (*subStructFileFstat)( rsComm_t*, int, rodsStat_t** );/* JMC */
    rodsLong_t       (*subStructFileLseek)( rsComm_t*, int, rodsLong_t, int ); /* JMC */
    int              (*subStructFileRename)( rsComm_t*, subFile_t*, char* ); /* JMC */
    int              (*subStructFileMkdir)( rsComm_t*, subFile_t* ); /* JMC */
    int              (*subStructFileRmdir)( rsComm_t*, subFile_t* ); /* JMC */
    int              (*subStructFileOpendir)( rsComm_t*, subFile_t* ); /* JMC */
    int              (*subStructFileReaddir)( rsComm_t*, int, rodsDirent_t** ); /* JMC */
    int              (*subStructFileClosedir)( rsComm_t*, int ); /* JMC */
    int              (*subStructFileTruncate)( rsComm_t*, subFile_t* ); /* JMC */
    int              (*structFileSync)( rsComm_t*, structFileOprInp_t*); /* JMC */
    int              (*structFileExtract)( rsComm_t*, structFileOprInp_t* ); /* JMC */
} structFileDriver_t;

#define CACHE_DIR_STR "cacheDir"

typedef struct structFileDesc {
    int inuseFlag;
    rsComm_t *rsComm;
    specColl_t *specColl;
    rescInfo_t *rescInfo;
    int openCnt;
} structFileDesc_t;

#define NUM_STRUCT_FILE_DESC 16

int
subStructFileIndexLookup (structFileType_t myType);
int
subStructFileCreate (rsComm_t *rsComm, subFile_t *subFile);
int
subStructFileOpen (rsComm_t *rsComm, subFile_t *subFile);
int
subStructFileRead (structFileType_t myType, rsComm_t *rsComm, int fd, void *buf, int len);
int
subStructFileWrite (structFileType_t myType, rsComm_t *rsComm, int fd, void *buf, int len);
int
subStructFileClose (structFileType_t myType, rsComm_t *rsComm, int fd);
int
subStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile);
int
subStructFileStat (rsComm_t *rsComm, subFile_t *subFile,
rodsStat_t **subStructFileStatOut);
int
subStructFileFstat (structFileType_t myType, rsComm_t *rsComm, int fd,
rodsStat_t **subStructFileStatOut);
rodsLong_t
subStructFileLseek (structFileType_t myType, rsComm_t *rsComm, int fd,
rodsLong_t offset, int whence);
int
subStructFileRename (rsComm_t *rsComm, subFile_t *subFile, char *newFileName);
int
subStructFileMkdir (rsComm_t *rsComm, subFile_t *subFile);
int
subStructFileRmdir (rsComm_t *rsComm, subFile_t *subFile);
int
subStructFileReaddir (structFileType_t myType, rsComm_t *rsComm, int fd, 
rodsDirent_t **rodsDirent);
int
subStructFileClosedir (structFileType_t myType, rsComm_t *rsComm, int fd);
int
subStructFileTruncate (rsComm_t *rsComm, subFile_t *subFile);
int
subStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile);
int
structFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp);
int
structFileExtract (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp);
int
initStructFileDesc ();
int
allocStructFileDesc ();
int
freeStructFileDesc (int structFileInx);

#endif	/* STRUCT_FILE_DRIVER_H */
