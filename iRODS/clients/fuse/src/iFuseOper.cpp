/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "iFuseOper.hpp"
#include "iFuse.Preload.hpp"
#include "iFuse.BufferedFS.hpp"
#include "iFuse.FS.hpp"
#include "iFuse.Lib.Fd.hpp"
#include "iFuse.Lib.Conn.hpp"
#include "iFuse.Lib.Util.hpp"
#include "iFuse.Lib.RodsClientAPI.hpp"
#include "sockComm.h"

void *iFuseInit(struct fuse_conn_info *conn) {
    UNUSED(conn);
    
    iFuseLibInit();
    iFuseFsInit();
    iFuseBufferedFSInit();
    iFusePreloadInit();
    return NULL;
}

void iFuseDestroy(void *data) {
    UNUSED(data);
    
    iFusePreloadDestroy();
    iFuseBufferedFSDestroy();
    iFuseFsDestroy();
    iFuseLibDestroy();
}

int iFuseGetAttr(const char *path, struct stat *stbuf) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseGetAttr: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    if(iFuseLibGetOption()->bufferedFS) {
        status = iFuseBufferedFsGetAttr(iRodsPath, stbuf);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseGetAttr: iFuseBufferedFsGetAttr of %s error", iRodsPath);
        }
    } else {
        status = iFuseFsGetAttr(iRodsPath, stbuf);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseGetAttr: iFuseFsGetAttr of %s error", iRodsPath);
        }
    }
    
    return status;
}

int iFuseOpen(const char *path, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseFd_t *iFuseFd = NULL;
    int flag = fi->flags;
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseOpen: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    if(iFuseLibGetOption()->bufferedFS) {
        if(iFuseLibGetOption()->preload) {
            status = iFusePreloadOpen(iRodsPath, &iFuseFd, flag);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, 
                        "iFuseOpen: cannot open file descriptor for %s error", iRodsPath);
                return -ENOENT;
            }
        } else {
            status = iFuseBufferedFsOpen(iRodsPath, &iFuseFd, flag);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, 
                        "iFuseOpen: cannot open file descriptor for %s error", iRodsPath);
                return -ENOENT;
            }
        }
    } else {
        status = iFuseFsOpen(iRodsPath, &iFuseFd, flag);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseOpen: cannot open file descriptor for %s error", iRodsPath);
            return -ENOENT;
        }
    }
    
    fi->fh = (uint64_t)iFuseFd;
    return 0;
}

int iFuseClose(const char *path, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseFd_t *iFuseFd = NULL;

    assert(fi->fh != 0);
    
    iFuseFd = (iFuseFd_t *)fi->fh;
    
    assert(iFuseFd->fd > 0);
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseClose: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    if(iFuseLibGetOption()->bufferedFS) {
        if(iFuseLibGetOption()->preload) {
            status = iFusePreloadClose(iFuseFd);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, 
                        "iFuseClose: cannot close file descriptor for %s error", iRodsPath);
                return -ENOENT;
            }
        } else {
            status = iFuseBufferedFsClose(iFuseFd);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, 
                        "iFuseClose: cannot close file descriptor for %s error", iRodsPath);
                return -ENOENT;
            }
        }
    } else {
        status = iFuseFsClose(iFuseFd);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseClose: cannot close file descriptor for %s error", iRodsPath);
            return -ENOENT;
        }
    }
    
    return 0;
}

int iFuseFlush(const char *path, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseFd_t *iFuseFd = NULL;
    
    assert(fi->fh != 0);
    
    iFuseFd = (iFuseFd_t *)fi->fh;
    
    assert(iFuseFd->fd > 0);
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseFlush: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    if(iFuseLibGetOption()->bufferedFS) {
        status = iFuseBufferedFsFlush(iFuseFd);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseFlush: cannot flush file content for %s error", iRodsPath);
            return -ENOENT;
        }
    } else {
        status = iFuseFsFlush(iFuseFd);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseFlush: cannot flush file content for %s error", iRodsPath);
            return -ENOENT;
        }
    }
    
    return status;
}

int iFuseFsync(const char *path, int isdatasync, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseFd_t *iFuseFd = NULL;
    
    UNUSED(isdatasync);
    assert(fi->fh != 0);
    
    iFuseFd = (iFuseFd_t *)fi->fh;
    
    assert(iFuseFd->fd > 0);
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseFsync: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    if(iFuseLibGetOption()->bufferedFS) {
        status = iFuseBufferedFsFlush(iFuseFd);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseFsync: cannot flush file content for %s error", iRodsPath);
            return -ENOENT;
        }
    } else {
        status = iFuseFsFlush(iFuseFd);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseFsync: cannot flush file content for %s error", iRodsPath);
            return -ENOENT;
        }
    }
    
    return status;
}

int iFuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseFd_t *iFuseFd = NULL;

    assert(buf != NULL);
    assert(size > 0);
    assert(offset >= 0);
    assert(fi->fh != 0);
    
    iFuseFd = (iFuseFd_t *)fi->fh;
    
    assert(iFuseFd->fd > 0);
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseRead: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    if(iFuseLibGetOption()->bufferedFS) {
        if(iFuseLibGetOption()->preload) {
            status = iFusePreloadRead(iFuseFd, buf, offset, size);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, 
                        "iFuseRead: cannot read file content for %s error", iRodsPath);
                return -ENOENT;
            }
        } else {
            status = iFuseBufferedFsRead(iFuseFd, buf, offset, size);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, 
                        "iFuseRead: cannot read file content for %s error", iRodsPath);
                return -ENOENT;
            }
        }
    } else {
        status = iFuseFsRead(iFuseFd, buf, offset, size);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseRead: cannot read file content for %s error", iRodsPath);
            return -ENOENT;
        }
    }
    
    return status;
}

int iFuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseFd_t *iFuseFd = NULL;

    iFuseFd = (iFuseFd_t *)fi->fh;
    
    assert(iFuseFd->fd > 0);

    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseWrite: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    if(iFuseLibGetOption()->bufferedFS) {
        status = iFuseBufferedFsWrite(iFuseFd, buf, offset, size);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseWrite: cannot write file content for %s error", iRodsPath);
            return -ENOENT;
        }
    } else {
        status = iFuseFsWrite(iFuseFd, buf, offset, size);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseWrite: cannot write file content for %s error", iRodsPath);
            return -ENOENT;
        }
    }

    return status;
}

int iFuseCreate(const char *path, mode_t mode, dev_t) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseCreate: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    status = iFuseFsCreate(iRodsPath, mode);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseCreate: cannot create a file for %s error", iRodsPath);
        return -ENOENT;
    }
    
    return 0;
}

int iFuseUnlink(const char *path) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseUnlink: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    status = iFuseFsUnlink(iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseUnlink: cannot delete a file for %s error", iRodsPath);
        return status;
    }
    
    return 0;
}

int iFuseLink(const char *from, const char *to) {
    UNUSED(from);
    UNUSED(to);
    
    return 0;
}

int iFuseStatfs(const char *path, struct statvfs *stbuf) {
    int status = 0;
    
    UNUSED(path);
    
    if (stbuf == NULL) {
        return 0;
    }
    
    status = statvfs( "/", stbuf );
    if ( status < 0 ) {
        // error cond?
    }
    
    stbuf->f_bsize = FILE_BLOCK_SIZE;
    stbuf->f_blocks = 2000000000;
    stbuf->f_bfree = stbuf->f_bavail = 1000000000;
    stbuf->f_files = 200000000;
    stbuf->f_ffree = stbuf->f_favail = 100000000;
    stbuf->f_fsid = 777;
    stbuf->f_namemax = MAX_NAME_LEN;
    
    return 0;
}

int iFuseOpenDir(const char *path, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseDir_t *iFuseDir = NULL;
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseOpenDir: iFuseRodsClientMakeRodsPath of %s error", path);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    status = iFuseFsOpenDir(iRodsPath, &iFuseDir);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseOpenDir: cannot open a directory for %s error", iRodsPath);
        return status;
    }
    
    fi->fh = (uint64_t)iFuseDir;
    return 0;
}

int iFuseCloseDir(const char *path, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseDir_t *iFuseDir = NULL;

    assert(fi->fh != 0);
    
    iFuseDir = (iFuseDir_t *)fi->fh;
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseCloseDir: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    status = iFuseFsCloseDir(iFuseDir);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseCloseDir: cannot close a directory for %s error", iRodsPath);
        return -ENOENT;
    }
    
    return 0;
}

int iFuseReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseDir_t *iFuseDir = NULL;

    assert(buf != NULL);
    assert(fi->fh != 0);
    
    iFuseDir = (iFuseDir_t *)fi->fh;
    
    assert(iFuseDir->handle != NULL);
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseReadDir: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    status = iFuseFsReadDir(iFuseDir, filler, buf, offset);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseReadDir: cannot read directory for %s error", iRodsPath);
        return status;
    }
    
    return 0;
}

int iFuseMakeDir(const char *path, mode_t mode) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    struct stat stbuf;
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseMakeDir: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    bzero(&stbuf, sizeof(struct stat));
    status = iFuseFsGetAttr(iRodsPath, &stbuf);
    if (status >= 0) {
        return -EEXIST;
    }
    
    status = iFuseFsMakeDir(iRodsPath, mode);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseMakeDir: cannot create a directory for %s error", iRodsPath);
        return -ENOENT;
    }
    
    return 0;
}

int iFuseRemoveDir(const char *path) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseRemoveDir: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    status = iFuseFsRemoveDir(iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseRemoveDir: cannot delete a directory for %s error", iRodsPath);
        return status;
    }
    
    return 0;
}

int iFuseRename(const char *from, const char *to) {
    int status = 0;
    char iRodsFromPath[MAX_NAME_LEN];
    char iRodsToPath[MAX_NAME_LEN];
    struct stat stbuf;
    
    bzero(iRodsFromPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(from, iRodsFromPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseRename: iFuseRodsClientMakeRodsPath of %s error", iRodsFromPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    bzero(iRodsToPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(to, iRodsToPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseRename: iFuseRodsClientMakeRodsPath of %s error", iRodsToPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    bzero(&stbuf, sizeof(struct stat));
    status = iFuseFsGetAttr(iRodsToPath, &stbuf);
    if (status >= 0) {
        status = iFuseFsUnlink(iRodsToPath);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseUnlink: cannot delete a file for %s error", iRodsToPath);
            return status;
        }
    }
    
    status = iFuseFsRename(iRodsFromPath, iRodsToPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseFsRename: cannot rename a file or a directory for %s to %s error", iRodsFromPath, iRodsToPath);
        return status;
    }
    
    return 0;
}

int iFuseTruncate(const char *path, off_t size) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseTruncate: iFuseRodsClientMakeRodsPath of %s error", path);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    status = iFuseFsTruncate(iRodsPath, size);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseTruncate: cannot truncate a file for %s error", iRodsPath);
        return status;
    }
    
    return 0;
}

int iFuseSymlink(const char *to, const char *from) {
    int status = 0;
    char iRodsFromPath[MAX_NAME_LEN];
    char iRodsToPath[MAX_NAME_LEN];
    struct stat stbuf;
    iFuseFd_t *iFuseFd = NULL;
    
    bzero(iRodsFromPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(from, iRodsFromPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseSymlink: iFuseRodsClientMakeRodsPath of %s error", iRodsFromPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    bzero(iRodsToPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(to, iRodsToPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseSymlink: iFuseRodsClientMakeRodsPath of %s error", iRodsToPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    bzero(&stbuf, sizeof(struct stat));
    status = iFuseFsGetAttr(iRodsFromPath, &stbuf);
    if (status >= 0) {
        status = iFuseFsTruncate(iRodsFromPath, 0);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseSymlink: cannot truncate a file for %s error", iRodsFromPath);
            return status;
        }
    } else if(status == -ENOENT) {
        status = iFuseFsCreate(iRodsFromPath, S_IFLNK);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, 
                    "iFuseSymlink: cannot create a file for %s error", iRodsFromPath);
            return -ENOENT;
        }
    }
    
    status = iFuseFsOpen(iRodsFromPath, &iFuseFd, O_WRONLY);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseSymlink: cannot open file descriptor for %s error", iRodsFromPath);
        return -ENOENT;
    }
    
    status = iFuseFsWrite(iFuseFd, iRodsToPath, 0, strlen(iRodsToPath));
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseSymlink: cannot write file content for %s error", iRodsToPath);
        iFuseFsClose(iFuseFd);
        return -ENOENT;
    }

    status = iFuseFsClose(iFuseFd);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseSymlink: cannot close file descriptor for %s error", iRodsFromPath);
        return -ENOENT;
    }
    
    return 0;
}

int iFuseReadLink(const char *path, char *buf, size_t size) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    iFuseFd_t *iFuseFd = NULL;
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseReadLink: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    status = iFuseFsOpen(iRodsPath, &iFuseFd, O_RDONLY);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseReadLink: cannot open file descriptor for %s error", iRodsPath);
        return -ENOENT;
    }
    
    status = iFuseFsRead(iFuseFd, buf, 0, size - 1);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseReadLink: cannot read file content for %s error", iRodsPath);
        iFuseFsClose(iFuseFd);
        return -ENOENT;
    }
    
    buf[status] = 0;

    status = iFuseFsClose(iFuseFd);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseReadLink: cannot close file descriptor for %s error", iRodsPath);
        return -ENOENT;
    }
    
    return 0;
}

int iFuseChmod(const char *path, mode_t mode) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath(path, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "iFuseChmod: iFuseRodsClientMakeRodsPath of %s error", path);
        // use ENOTDIR for this type of error
        return -ENOTDIR;
    }
    
    status = iFuseFsChmod(iRodsPath, mode);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, 
                "iFuseChmod: cannot change mode of a file for %s error", iRodsPath);
        return status;
    }
    
    return 0;
}

int iFuseChown(const char *path, uid_t uid, gid_t gid) {
    UNUSED(path);
    UNUSED(uid);
    UNUSED(gid);
    
    return 0;
}

int iFuseUtimens(const char *path, const struct timespec ts[2]) {
    UNUSED(path);
    UNUSED(ts);
    
    return 0;
}
