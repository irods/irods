/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* wosFileDriver.h - header file for wosFileDriver.c
 */



#ifndef WOS_FILE_DRIVER_H
#define WOS_FILE_DRIVER_H

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

int
wosFileUnlink (rsComm_t *rsComm, char *filename);
int
wosFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf);
rodsLong_t
wosFileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag);
int
wosStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput);
int
wosSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput);
#endif	/* WOS_FILE_DRIVER_H */
