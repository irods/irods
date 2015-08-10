/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_OPER_HPP
#define IFUSE_OPER_HPP

#include <sys/statvfs.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#ifdef  __cplusplus
extern "C" {
#endif
    void *iFuseInit(struct fuse_conn_info *conn);
    void iFuseDestroy(void *data);
    
    int iFuseGetAttr(const char *path, struct stat *stbuf);
    int iFuseOpen(const char *path, struct fuse_file_info *fi);
    int iFuseClose(const char *path, struct fuse_file_info *fi);
    int iFuseFlush(const char *path, struct fuse_file_info *fi);
    int iFuseFsync(const char *path, int isdatasync, struct fuse_file_info *fi);
    int iFuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int iFuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int iFuseCreate(const char *path, mode_t mode, dev_t rdev);
    int iFuseUnlink(const char *path);
    int iFuseLink(const char *from, const char *to);
    int iFuseStatfs(const char *path, struct statvfs *stbuf);
    int iFuseOpenDir(const char *path, struct fuse_file_info *fi);
    int iFuseCloseDir(const char *path, struct fuse_file_info *fi);
    int iFuseReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
    int iFuseMakeDir(const char *path, mode_t mode);
    int iFuseRemoveDir(const char *path);
    int iFuseRename(const char *from, const char *to);
    int iFuseTruncate(const char *path, off_t size);
    int iFuseSymlink(const char *from, const char *to);
    int iFuseReadLink(const char *path, char *buf, size_t size);
    int iFuseChmod(const char *path, mode_t mode);
    int iFuseChown(const char *path, uid_t uid, gid_t gid);
    int iFuseUtimens(const char *path, const struct timespec ts[]);
#ifdef  __cplusplus
}
#endif

#endif	/* IFUSE_OPER_HPP */
