/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileOpr.h - header file for fileOpr.c
 */



#ifndef FILE_OPR_H
#define FILE_OPR_H

#include "rods.h"
#include "initServer.h"
#include "fileDriver.h"
#include "chkNVPathPerm.h"

#define NUM_FILE_DESC	1026 	/* number of FileDesc */

/* definition for inuseFlag */

#define FD_FREE		0
#define FD_INUSE	1

#define STREAM_FILE_NAME	"stream"   /* a fake file name for stream */
typedef struct {
    int inuseFlag;	/* whether the fileDesc is in use, 0=no */
    rodsServerHost_t *rodsServerHost;
    char *fileName;
    fileDriverType_t fileType;
    int mode;
    int chkPerm;        /* check for permission in the file vault */
    int fd;		/* the file descriptor from driver */
    int writtenFlag;	/* indicated whether the file has been written to */
    void *driverDep;	/* driver dependent stuff */
} fileDesc_t;

int
initFileDesc ();

int
allocFileDesc ();

int
allocAndFillFileDesc (rodsServerHost_t *rodsServerHost, char *fileName,
fileDriverType_t fileType, int fd, int mode);

int
freeFileDesc (int fileInx);

int
getServerHostByFileInx (int fileInx, rodsServerHost_t **rodsServerHost);
int
mkDirForFilePath (int fileType, rsComm_t *rsComm, char *startDir,
char *filePath, int mode);
int
mkFileDirR (int fileType, rsComm_t *rsComm, char *startDir,
char *destDir, int mode);
int
procCacheDir (rsComm_t *rsComm, char *cacheDir, char *resource);
int
chkFilePathPerm (rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
rodsServerHost_t *rodsServerHost);
int
matchVaultPath (rsComm_t *rsComm, char *filePath,
rodsServerHost_t *rodsServerHost, char **outVaultPath);
int
matchCliVaultPath (rsComm_t *rsComm, char *filePath,
rodsServerHost_t *rodsServerHost);
int
chkEmptyDir (int fileType, rsComm_t *rsComm, char *cacheDir);
int
filePathTypeInResc (rsComm_t *rsComm, char *fileName, rescInfo_t *rescInfo);
int
bindStreamToIRods (rodsServerHost_t *rodsServerHost, int fd);
#endif	/* FILE_OPR_H */
