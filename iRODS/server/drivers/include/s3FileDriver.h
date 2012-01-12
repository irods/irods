/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* s3FileDriver.h - header file for s3FileDriver.c
 */



#ifndef S3_FILE_DRIVER_H
#define S3_FILE_DRIVER_H

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
#include "libs3.h"

#define S3_AUTH_FILE "s3Auth"

typedef struct S3Auth {
  char accessKeyId[MAX_NAME_LEN];
  char secretAccessKey[MAX_NAME_LEN];
} s3Auth_t;

typedef struct s3Stat
{
    char key[MAX_NAME_LEN];
    rodsLong_t size;
    time_t lastModified;
} s3Stat_t;

typedef struct callback_data
{
    FILE *fd;
    rodsLong_t contentLength, originalContentLength;
    int isTruncated;
    char nextMarker[1024];
    int keyCount;
    int allDetails;
    s3Stat_t s3Stat;    /* should be a pointer if keyCount > 1 */
    int status;
} callback_data_t;

int
s3FileUnlink (rsComm_t *rsComm, char *filename);
int
s3FileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf);
int
s3FileMkdir (rsComm_t *rsComm, char *filename, int mode);
int
s3FileChmod (rsComm_t *rsComm, char *filename, int mode);
int
s3FileRename (rsComm_t *rsComm, char *oldFileName, char *newFileName);
rodsLong_t
s3FileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag);
int
s3FileRmdir (rsComm_t *rsComm, char *filename);
int
s3StageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput);
int
s3SyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput);
void 
responseCompleteCallback(S3Status status, const S3ErrorDetails *error,
void *callbackData);
S3Status 
responsePropertiesCallback(const S3ResponseProperties *properties,
void *callbackData);
int 
putObjectDataCallback(int bufferSize, char *buffer, void *callbackData);
int
putFileIntoS3(char *fileName, char *s3ObjName, rodsLong_t fileSize);
int
myS3Init (void);
int
readS3AuthInfo (void);
int
myS3Error (int status, int irodsErrorCode);
int
parseS3Path (char *s3ObjName, char *bucket, char *key);
int
list_bucket(const char *bucketName, const char *prefix, const char *marker,
const char *delimiter, int maxkeys, int allDetails, s3Stat_t *s3Stat);
S3Status 
listBucketCallback(int isTruncated, const char *nextMarker, int contentsCount,
const S3ListBucketContent *contents, int commonPrefixesCount,
const char **commonPrefixes, void *callbackData);
int
getFileFromS3 (char *fileName, char *s3ObjName, rodsLong_t fileSize);
S3Status
getObjectDataCallback(int bufferSize, const char *buffer, void *callbackData);
int
copyS3Obj (char *srcObj, char *destObj);
#endif	/* S3_FILE_DRIVER_H */
