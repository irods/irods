/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.h - common header file for file driver
 */



#ifndef FILE_DRIVER_H
#define FILE_DRIVER_H

#ifndef windows_platform
#include <dirent.h>
#endif

#include "rods.h"
#include "rcConnect.h"
#include "objInfo.h"
#include "msParam.h"

typedef struct {
    fileDriverType_t	driverType; 
    int         	(*fileCreate)( rsComm_t*, char*, int, rodsLong_t ); /* JMC */
    int         	(*fileOpen)( rsComm_t*, char*, int, int ); /* JMC */
    int         	(*fileRead)( rsComm_t*, int, void*, int ); /* JMC */
    int         	(*fileWrite)( rsComm_t*, int, void*, int ); /* JMC */
    int         	(*fileClose)( rsComm_t*, int ); /* JMC */
    int         	(*fileUnlink)( rsComm_t*, char* ); /* JMC */
    int         	(*fileStat)( rsComm_t*, char*, struct stat* ); /* JMC */
    int         	(*fileFstat)( rsComm_t*, int, struct stat* ); /* JMC */
    rodsLong_t  	(*fileLseek)( rsComm_t*, int, rodsLong_t, int ); /* JMC */
    int         	(*fileFsync)( rsComm_t*, int ); /* JMC */
    int         	(*fileMkdir)( rsComm_t*, char *, int ); /* JMC */
    int         	(*fileChmod)( rsComm_t*, char*, int ); /* JMC */
    int         	(*fileRmdir)( rsComm_t*, char* ); /* JMC */
    int         	(*fileOpendir)( rsComm_t*, char*, void** ); /* JMC */
    int         	(*fileClosedir)( rsComm_t*, void* ); /* JMC */
    int         	(*fileReaddir)( rsComm_t*, void*, struct dirent* ); /* JMC */
    int         	(*fileStage)( rsComm_t*, char*, int ); /* JMC */
    int         	(*fileRename)( rsComm_t*, char*, char* ); /* JMC */
    rodsLong_t  	(*fileGetFsFreeSpace)( rsComm_t*, char*, int ); /* JMC */
    int         	(*fileTruncate)( rsComm_t*, char*, rodsLong_t ); /* JMC */
    int			(*fileStageToCache)( rsComm_t*, fileDriverType_t, int, int, char*, char*, rodsLong_t, keyValPair_t* ); /* JMC */
    int			(*fileSyncToArch)( rsComm_t*, fileDriverType_t, int, int, char*, char*, rodsLong_t, keyValPair_t*); /* JMC */
} fileDriver_t;



int
fileIndexLookup (fileDriverType_t myType);
int
fileCreate (fileDriverType_t myType, rsComm_t *rsComm, char *fileName,
int mode, rodsLong_t mySize);
int
fileOpen (fileDriverType_t myType, rsComm_t *rsComm, char *fileName, int flags, int mode);
int
fileRead (fileDriverType_t myType, rsComm_t *rsComm, int fd, void *buf,
int len);
int
fileWrite (fileDriverType_t myType, rsComm_t *rsComm, int fd, void *buf,
int len);
int
fileClose (fileDriverType_t myType, rsComm_t *rsComm, int fd);
int
fileUnlink (fileDriverType_t myType, rsComm_t *rsComm, char *filename);
int
fileStat (fileDriverType_t myType, rsComm_t *rsComm, char *filename,
struct stat *statbuf);
int
fileFstat (fileDriverType_t myType, rsComm_t *rsComm, int fd,
struct stat *statbuf);
rodsLong_t
fileLseek (fileDriverType_t myType, rsComm_t *rsComm, int fd,
rodsLong_t offset, int whence);
int
fileMkdir (fileDriverType_t myType, rsComm_t *rsComm, char *filename, int mode);
int
fileChmod (fileDriverType_t myType, rsComm_t *rsComm, char *filename, int mode);
int
fileRmdir (fileDriverType_t myType, rsComm_t *rsComm, char *filename);
int
fileOpendir (fileDriverType_t myType, rsComm_t *rsComm, char *filename,
void **outDirPtr);
int
fileClosedir (fileDriverType_t myType, rsComm_t *rsComm, void *dirPtr);
int
fileReaddir (fileDriverType_t myType, rsComm_t *rsComm, void *dirPtr,
struct dirent *direntPtr);
int
fileStage (fileDriverType_t myType, rsComm_t *rsComm, char *path, int flag);
int
fileRename (fileDriverType_t myType, rsComm_t *rsComm, char *oldFileName,
char *newFileName);
rodsLong_t
fileGetFsFreeSpace (fileDriverType_t myType, rsComm_t *rsComm,
char *path, int flag);
int
fileFsync (fileDriverType_t myType, rsComm_t *rsComm, int fd);
int
fileTruncate (fileDriverType_t myType, rsComm_t *rsComm, char *path,
rodsLong_t dataSize);
int
fileStageToCache (fileDriverType_t myType, rsComm_t *rsComm,
fileDriverType_t cacheFileType, int mode, int flag,
char *filename, char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput);
int
fileSyncToArch (fileDriverType_t myType, rsComm_t *rsComm, 
fileDriverType_t cacheFileType, int mode, int flag,
char *filename, char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput);
#endif	/* FILE_DRIVER_H */
