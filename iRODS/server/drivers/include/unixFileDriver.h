/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* unixFileDriver.h - header file for unixFileDriver.c
 */



#ifndef UNIX_FILE_DRIVER_H
#define UNIX_FILE_DRIVER_H

#include <stdio.h>
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>  
#endif
#include <dirent.h>
   
#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif
#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#if defined(aix_platform) || defined(sgi_platform)
#include <sys/statfs.h>
#endif
#if defined(osx_platform)
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <sys/stat.h>



#include "rods.h"
#include "rcConnect.h"
#include "msParam.h"
#include "miscServerFunct.h"

#define NB_READ_TOUT_SEC	60	/* 60 sec timeout */
#define NB_WRITE_TOUT_SEC	60	/* 60 sec timeout */

int
unixFileCreate (rsComm_t *rsComm, char *fileName, int mode, rodsLong_t mySize);
int
unixFileOpen (rsComm_t *rsComm, char *fileName, int flags, int mode);
int
unixFileRead (rsComm_t *rsComm, int fd, void *buf, int len);
int
unixFileWrite (rsComm_t *rsComm, int fd, void *buf, int len);
int
unixFileClose (rsComm_t *rsComm, int fd);
int
unixFileUnlink (rsComm_t *rsComm, char *filename);
int
unixFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf);
int
unixFileFstat (rsComm_t *rsComm, int fd, struct stat *statbuf);
int
unixFileFsync (rsComm_t *rsComm, int fd);
int
unixFileMkdir (rsComm_t *rsComm, char *filename, int mode);
int
unixFileChmod (rsComm_t *rsComm, char *filename, int mode);
int
unixFileOpendir (rsComm_t *rsComm, char *dirname, void **outDirPtr);
int
unixFileReaddir (rsComm_t *rsComm, void *dirPtr, struct  dirent *direntPtr);
int
unixFileStage (rsComm_t *rsComm, char *path, int flag);
int
unixFileRename (rsComm_t *rsComm, char *oldFileName, char *newFileName);
rodsLong_t
unixFileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag);
rodsLong_t
unixFileLseek (rsComm_t *rsComm, int fd, rodsLong_t offset, int whence);
int
unixFileRmdir (rsComm_t *rsComm, char *filename);
int
unixFileClosedir (rsComm_t *rsComm, void *dirPtr);
int
unixFileTruncate (rsComm_t *rsComm, char *filename, rodsLong_t dataSize);
int
unixStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput);
int
unixSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput);
int
unixFileCopy (int mode, char *srcFileName, char *destFileName);
int
nbFileRead (rsComm_t *rsComm, int fd, void *buf, int len);
int
nbFileWrite (rsComm_t *rsComm, int fd, void *buf, int len);

#endif	/* UNIX_FILE_DRIVER_H */
