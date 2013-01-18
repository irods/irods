/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* unixFileDriver.c - The UNIX file driver
 */


#include "unixFileDriver.h"
#include "eirods_stacktrace.h"

int
unixFileCreate (rsComm_t *rsComm, char *fileName, int mode, rodsLong_t mySize)
{
    int fd;
    mode_t myMask;

    myMask = umask((mode_t) 0000);
    fd = open (fileName, O_RDWR|O_CREAT|O_EXCL, mode);
    /* reset the old mask */
    (void) umask((mode_t) myMask);
    
    if (fd == 0) {
        close (fd);
        rodsLog (LOG_NOTICE, "unixFileCreate: 0 descriptor");
        open ("/dev/null", O_RDWR, 0);
        fd = open (fileName, O_RDWR|O_CREAT|O_EXCL, mode);
    }

    if (fd < 0) {
        fd = UNIX_FILE_CREATE_ERR - errno;
        if (errno == EEXIST) {
            rodsLog (LOG_DEBUG, 
                     "unixFileCreate: open error for %s, status = %d",
                     fileName, fd);
        } else {
            rodsLog (LOG_DEBUG, 
                     "unixFileCreate: open error for %s, status = %d",
                     fileName, fd);
        }
    }

    return (fd);
}

int
unixFileOpen (rsComm_t *rsComm, char *fileName, int flags, int mode)
{
    int fd;

#if defined(osx_platform)
    /* For osx, O_TRUNC = 0x0400, O_TRUNC = 0x200 for other system */
    if (flags & 0x200) {
        flags = flags ^ 0x200;
        flags = flags | O_TRUNC;
    }
#endif

    fd = open (fileName, flags, mode);

    if (fd == 0) {
        close (fd);
        rodsLog (LOG_NOTICE, "unixFileOpen: 0 descriptor");
        open ("/dev/null", O_RDWR, 0);
        fd = open (fileName, flags, mode);
    }

    if (fd < 0) {
        fd = UNIX_FILE_OPEN_ERR - errno;
        rodsLog (LOG_NOTICE, "unixFileOpen: open error for %s, status = %d",
                 fileName, fd);
        eirods::stacktrace st;
        st.trace();
        st.dump();

    }

    return (fd);
}

int
unixFileRead (rsComm_t *rsComm, int fd, void *buf, int len)
{
    int status;

    status = read (fd, buf, len);

    if (status < 0) {
        status = UNIX_FILE_READ_ERR - errno;
        rodsLog (LOG_NOTICE, "unixFileRead: read error fd = %d, status = %d",
                 fd, status);
    }
    return (status);
}

int
nbFileRead (rsComm_t *rsComm, int fd, void *buf, int len)
{
    int status;
    struct timeval tv;
    int nbytes;
    int toRead;
    char *tmpPtr;
    fd_set set;

    bzero (&tv, sizeof (tv));
    tv.tv_sec = NB_READ_TOUT_SEC;

    /* Initialize the file descriptor set. */
    FD_ZERO (&set);
    FD_SET (fd, &set);

    toRead = len;
    tmpPtr = (char *) buf;

    while (toRead > 0) {
#ifndef _WIN32
        status = select (fd + 1, &set, NULL, NULL, &tv);
        if (status == 0) {
            /* timedout */
            if (len - toRead > 0) {
                return (len - toRead);
            } else {
                return UNIX_FILE_OPR_TIMEOUT_ERR - errno;
            }
        } else if (status < 0) {
            if ( errno == EINTR) {
                errno = 0;
                continue;
            } else {
                return UNIX_FILE_READ_ERR - errno;
            }
        }
#endif
        nbytes = read (fd, (void *) tmpPtr, toRead);
        if (nbytes < 0) {
            if (errno == EINTR) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            } else if (toRead == len) {
                return UNIX_FILE_READ_ERR - errno;
            } else {
                nbytes = 0;
                break;
            }
        } else if (nbytes == 0) {
            break;
        }
        toRead -= nbytes;
        tmpPtr += nbytes;
    }
    return (len - toRead);
}

int
unixFileWrite (rsComm_t *rsComm, int fd, void *buf, int len)
{
    int status;

    status = write (fd, buf, len);

    if (status < 0) {
        status = UNIX_FILE_WRITE_ERR - errno;
        rodsLog (LOG_NOTICE, "unixFileWrite: open write fd = %d, status = %d",
                 fd, status);
    }
    return (status);

}

int
nbFileWrite (rsComm_t *rsComm, int fd, void *buf, int len)
{
    int nbytes;
    int toWrite;
    char *tmpPtr;
    fd_set set;
    struct timeval tv;
    int status;

    bzero (&tv, sizeof (tv));
    tv.tv_sec = NB_WRITE_TOUT_SEC;

    /* Initialize the file descriptor set. */
    FD_ZERO (&set);
    FD_SET (fd, &set);

    toWrite = len;
    tmpPtr = (char *) buf;

    while (toWrite > 0) {
#ifndef _WIN32
        status = select (fd + 1, NULL, &set, NULL, &tv);
        if (status == 0) {
            /* timedout */
            return UNIX_FILE_OPR_TIMEOUT_ERR - errno;
        } else if (status < 0) {
            if ( errno == EINTR) {
                errno = 0;
                continue;
            } else {
                return UNIX_FILE_WRITE_ERR - errno;
            }
        }
#endif
        nbytes = write (fd, (void *) tmpPtr, len);
        if (nbytes < 0) {
            if (errno == EINTR) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            } else  {
                return UNIX_FILE_WRITE_ERR - errno;
            }
        }
        toWrite -= nbytes;
        tmpPtr += nbytes;
    }
    return (len);
}

int
unixFileClose (rsComm_t *rsComm, int fd)
{
    int status;

    status = close (fd);

    if (fd == 0) {
        rodsLog (LOG_NOTICE, "unixFileClose: 0 descriptor");
        open ("/dev/null", O_RDWR, 0);
    }
    if (status < 0) {
        status = UNIX_FILE_CLOSE_ERR - errno;
        rodsLog (LOG_NOTICE, "unixFileClose: open write fd = %d, status = %d",
                 fd, status);
    }
    return (status);
}

int
unixFileUnlink (rsComm_t *rsComm, char *filename)
{
    int status;

    status = unlink (filename);

    if (status < 0) {
        status = UNIX_FILE_UNLINK_ERR - errno;
        rodsLog (LOG_NOTICE, "unixFileUnlink: unlink of %s error, status = %d",
                 filename, status);
    }

    return (status);
}

int
unixFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf)
{
    int status;

    status = stat (filename, statbuf);

#ifdef RUN_SERVER_AS_ROOT
    if (status < 0 && errno == EACCES && isServiceUserSet()) {
        /* if the file can't be accessed due to permission denied */
        /* try again using root credentials.                      */
        if (changeToRootUser() == 0) {
            status = stat (filename, statbuf);
            changeToServiceUser();
        }
    }
#endif

    if (status < 0) {
        status = UNIX_FILE_STAT_ERR - errno;
        rodsLog (LOG_DEBUG, "unixFileStat: stat of %s error, status = %d",
                 filename, status);
    }
    
    return (status);
}

int
unixFileFstat (rsComm_t *rsComm, int fd, struct stat *statbuf)
{
    int status;

    status = fstat (fd, statbuf);

#ifdef RUN_SERVER_AS_ROOT
    if (status < 0 && errno == EACCES && isServiceUserSet()) {
        /* if the file can't be accessed due to permission denied */
        /* try again using root credentials.                      */
        if (changeToRootUser() == 0) {
            status = fstat (fd, statbuf);
            changeToServiceUser();
        }
    }
#endif

    if (status < 0) {
        status = UNIX_FILE_FSTAT_ERR - errno;
        rodsLog (LOG_DEBUG, "unixFileFstat: stat of fd %d error, status = %d",
                 fd, status);
    }
   
    return (status);
}

rodsLong_t
unixFileLseek (rsComm_t *rsComm, int fd, rodsLong_t offset, int whence)
{
    rodsLong_t  status;

    status = lseek (fd, offset, whence);

    if (status < 0) {
        status = UNIX_FILE_LSEEK_ERR - errno;
        rodsLog (LOG_NOTICE, 
                 "unixFileLseek: lseek of fd %d error, status = %d", fd, status);
    }

    return (status);
}

int
unixFileFsync (rsComm_t *rsComm, int fd)
{
    int status;

    status = fsync (fd);

    if (status < 0) {
        status = UNIX_FILE_FSYNC_ERR - errno;
        rodsLog (LOG_NOTICE, 
                 "unixFileFsync: fsync of fd %d error, status = %d", fd, status);
    }
    return (status);
}

int
unixFileMkdir (rsComm_t *rsComm, char *filename, int mode)
{
    int status;
    mode_t myMask;

    myMask = umask((mode_t) 0000);


    status = mkdir (filename, mode);
    /* reset the old mask */
    (void) umask((mode_t) myMask);


    if (status < 0) {
        status = UNIX_FILE_MKDIR_ERR - errno;
        if (errno != EEXIST)
            rodsLog (LOG_NOTICE,
                     "unixFileMkdir: mkdir of %s error, status = %d", 
                     filename, status);
    }

    return (status);
}       

int
unixFileChmod (rsComm_t *rsComm, char *filename, int mode)
{
    int status;

    status = chmod (filename, mode);

    if (status < 0) {
        status = UNIX_FILE_CHMOD_ERR - errno;
        rodsLog (LOG_NOTICE,
                 "unixFileChmod: chmod of %s error, status = %d", 
                 filename, status);
    }

    return (status);
}

int
unixFileRmdir (rsComm_t *rsComm, char *filename)
{
    int status;

    status = rmdir (filename);

    if (status < 0) {
        status = UNIX_FILE_RMDIR_ERR - errno;
        rodsLog (LOG_DEBUG,
                 "unixFileRmdir: rmdir of %s error, status = %d",
                 filename, status);
    }

    return (status);
}

int
unixFileOpendir (rsComm_t *rsComm, char *dirname, void **outDirPtr)
{
    int status;
    DIR *dirPtr;


    dirPtr = opendir (dirname);

#ifdef RUN_SERVER_AS_ROOT
    if (dirPtr == NULL && errno == EACCES && isServiceUserSet()) {
        /* if the directory can't be accessed due to permission */
        /* denied try again using root credentials.             */
        if (changeToRootUser() == 0) {
            dirPtr = opendir (dirname);
            changeToServiceUser();
        }
    }
#endif

    if (dirPtr != NULL) {
        *outDirPtr = (void *) dirPtr;
        status = 0;
        return (0);
    } else {
        status = UNIX_FILE_OPENDIR_ERR - errno;
        rodsLog (LOG_NOTICE,
                 "unixFileOpendir: opendir of %s error, status = %d",
                 dirname, status);
    }
    return (status);
}

int
unixFileClosedir (rsComm_t *rsComm, void *dirPtr)
{
    int status;

    status = closedir ((DIR *) dirPtr);

    if (status < 0) {
        status = UNIX_FILE_CLOSEDIR_ERR - errno;
        rodsLog (LOG_NOTICE, 
                 "unixFileClosedir: closedir error, status = %d", status);
    }
    return (status);
}

int
unixFileReaddir (rsComm_t *rsComm, void *dirPtr, struct dirent *direntPtr)
{
    int status;
    struct dirent *tmpDirentPtr;

    errno = 0;
    tmpDirentPtr = readdir ((DIR*)dirPtr);

    if (tmpDirentPtr == NULL) {
        if (errno == 0) {
            /* just the end */
            status = -1;
        } else {
            status = UNIX_FILE_READDIR_ERR - errno;
            rodsLog (LOG_NOTICE,
                     "unixFileReaddir: readdir error, status = %d", status);
        }
    } else {
        status = 0;
        *direntPtr = *tmpDirentPtr;
#if defined(solaris_platform)
        rstrcpy (direntPtr->d_name, tmpDirentPtr->d_name, MAX_NAME_LEN);
#endif
    }
    return (status);
}

int
unixFileStage (rsComm_t *rsComm, char *path, int flag)
{
#ifdef SAMFS_STAGE
    int status;
    status = sam_stage (path, "i");

    if (status < 0) {
        status = UNIX_FILE_STAGE_ERR - errno;
        rodsLog (LOG_NOTICE,
                 "unixFileStage: sam_stage error, status = %d\n",
                 status);
    }

    return (status);
#else
    return (0);
#endif
}

int
unixFileRename (rsComm_t *rsComm, char *oldFileName, char *newFileName)
{
    int status;
    status = rename (oldFileName, newFileName);

    if (status < 0) {
        status = UNIX_FILE_RENAME_ERR - errno;
        rodsLog (LOG_NOTICE,
                 "unixFileRename: rename error, status = %d\n",
                 status);

        eirods::stacktrace st;
        st.trace();
        st.dump();
                                    
    }

    return (status);
}

int
unixFileTruncate (rsComm_t *rsComm, char *filename, rodsLong_t dataSize)
{
    int status;

    status = truncate (filename, dataSize);

    if (status < 0) {
        status = UNIX_FILE_TRUNCATE_ERR - errno;
        rodsLog (LOG_NOTICE, 
                 "unixFileTruncate: truncate of %s error, status = %d",
                 filename, status);
    }

    return (status);
}

rodsLong_t
unixFileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag)
{
    int status;
    rodsLong_t fssize = USER_NO_SUPPORT_ERR;
#if defined(solaris_platform)
    struct statvfs statbuf;
#else
    struct statfs statbuf;
#endif
#if defined(solaris_platform) || defined(sgi_platform) || defined(aix_platform) || defined(linux_platform) || defined(osx_platform)
#if defined(solaris_platform)
    status = statvfs (path, &statbuf);
#else
#if defined(sgi_platform)
    status = statfs (path, &statbuf, sizeof (struct statfs), 0);
#else
    status = statfs (path, &statbuf);
#endif
#endif
    if (status < 0) {
        status = UNIX_FILE_GET_FS_FREESPACE_ERR - errno;
        rodsLog (LOG_NOTICE,
                 "UNIX statfs error for %s. errorCode = %d", path, status);
        return (status);
    }
#if defined(sgi_platform)
    if (statbuf.f_frsize > 0) {
        fssize = statbuf.f_frsize;
    } else {
        fssize = statbuf.f_bsize;
    }
    fssize *= statbuf.f_bavail;
#endif

#if defined(aix_platform) || defined(osx_platform) || (linux_platform)
    fssize = statbuf.f_bavail * statbuf.f_bsize;
#endif
#if defined(sgi_platform)
    fssize = statbuf.f_bfree * statbuf.f_bsize;
#endif

#endif /* solaris_platform, sgi_platform .... */

    return (fssize);
}

/* unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
 * Just copy the file from filename to cacheFilename. optionalInfo info
 * is not used.
 * 
 */
  
int
unixStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
                  int mode, int flags, char *filename, 
                  char *cacheFilename, rodsLong_t dataSize,
                  keyValPair_t *condInput)
{
    int status;

    status = unixFileCopy (mode, filename, cacheFilename);
    return status;
}

/* unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
 * Just copy the file from cacheFilename to filename. optionalInfo info
 * is not used.
 *
 */

int
unixSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
                int mode, int flags, char *filename,
                char *cacheFilename,  rodsLong_t dataSize,
                keyValPair_t *condInput)
{
    int status;

    status = unixFileCopy (mode, cacheFilename, filename);
    return status;
}

int
unixFileCopy (int mode, char *srcFileName, char *destFileName)
{
    int inFd, outFd;
    char myBuf[TRANS_BUF_SZ];
    rodsLong_t bytesCopied = 0;
    int bytesRead;
    int bytesWritten;
    int status;
    struct stat statbuf;

    status = stat (srcFileName, &statbuf);

    if (status < 0) {
        status = UNIX_FILE_STAT_ERR - errno;
        rodsLog (LOG_ERROR, "unixFileCopy: stat of %s error, status = %d",
                 srcFileName, status);
        return status;
    }

    inFd = open (srcFileName, O_RDONLY, 0);
    if (inFd < 0 || (statbuf.st_mode & S_IFREG) == 0) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog (LOG_ERROR,
                 "unixFileCopy: open error for srcFileName %s, status = %d",
                 srcFileName, status);
        close( inFd ); // JMC cppcheck - resource
        eirods::stacktrace st;
        st.trace();
        st.dump();
        return status;
    }

    outFd = open (destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (outFd < 0) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog (LOG_ERROR,
                 "unixFileCopy: open error for destFileName %s, status = %d",
                 destFileName, status);
        close (inFd);
        eirods::stacktrace st;
        st.trace();
        st.dump();
        return status;
    }

    while ((bytesRead = read (inFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
        bytesWritten = write (outFd, (void *) myBuf, bytesRead);
        if (bytesWritten <= 0) {
            status = UNIX_FILE_WRITE_ERR - errno;
            rodsLog (LOG_ERROR,
                     "unixFileCopy: write error for srcFileName %s, status = %d",
                     destFileName, status);
            close (inFd);
            close (outFd);
            return status;
        }
        bytesCopied += bytesWritten;
    }

    close (inFd);
    close (outFd);

    if (bytesCopied != statbuf.st_size) {
        rodsLog (LOG_ERROR,
                 "unixFileCopy: Copied size %lld does not match source size %lld of %s",
                 bytesCopied, statbuf.st_size, srcFileName);
        return SYS_COPY_LEN_ERR;
    } else {
        return 0;
    }
}

