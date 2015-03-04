/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileOpr.h - header file for fileOpr.c
 */



#ifndef FILE_OPR_HPP
#define FILE_OPR_HPP

#include "rodsConnect.h"
#include "fileDriver.hpp"
#include "chkNVPathPerm.hpp"

#define NUM_FILE_DESC   1026    /* number of FileDesc */

/* definition for inuseFlag */

#define FD_FREE         0
#define FD_INUSE        1

#define STREAM_FILE_NAME        "stream"   /* a fake file name for stream */
typedef struct {
    int inuseFlag;      /* whether the fileDesc is in use, 0=no */
    rodsServerHost_t *rodsServerHost;
    char *fileName;
    char *rescHier;             // The hierarchy of resources in which this file resides
    char *objPath;              // The logical path
    int mode;
    int chkPerm;        /* check for permission in the file vault */
    int fd;             /* the file descriptor from driver */
    int writtenFlag;    /* indicated whether the file has been written to */
    void *driverDep;    /* driver dependent stuff */
} fileDesc_t;

int
initFileDesc();

int
allocFileDesc();

int
allocAndFillFileDesc( rodsServerHost_t *rodsServerHost, const std::string& objPath,
                      const std::string& fileName, const std::string& rescHier, int fd, int mode );

int
freeFileDesc( int fileInx );

int
getServerHostByFileInx( int fileInx, rodsServerHost_t **rodsServerHost );
int
mkDirForFilePath( rsComm_t *rsComm, size_t startDirLen, const std::string& filePath, const std::string& hier, int mode );
int
mkFileDirR( rsComm_t *rsComm, size_t startDirLen, const std::string& destDir, const std::string& hier, int mode );
int
chkFilePathPerm( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
                 rodsServerHost_t *rodsServerHost, int chkType ); // JMC - backport 4774
int // JMC - backport 4766
isValidFilePath( const std::string& path );

int
matchCliVaultPath( rsComm_t*,
                   const std::string&,
                   rodsServerHost_t* );
int
chkEmptyDir( rsComm_t *rsComm, const std::string& cacheDir, const std::string& hier );
int
filePathTypeInResc( rsComm_t *rsComm, const std::string& objPath, const std::string& fileName, const std::string& rescHier );
int
bindStreamToIRods( rodsServerHost_t *rodsServerHost, int fd );
#endif  /* FILE_OPR_H */
