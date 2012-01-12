/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to structFiles in the COPYRIGHT directory ***/

/* haawSubStructFileDriver.h - header structFile for haawSubStructFileDriver.c
 */



#ifndef HAAW_STRUCT_FILE_DRIVER_H_H
#define HAAW_STRUCT_FILE_DRIVER_H_H

#include "rods.h"
#include "structFileDriver.h"

int
haawSubStructFileCreate (rsComm_t *rsComm, subFile_t *subFile);
int 
haawSubStructFileOpen (rsComm_t *rsComm, subFile_t *subFile); 
int 
haawSubStructFileRead (rsComm_t *rsComm, int fd, void *buf, int len);
int
haawSubStructFileWrite (rsComm_t *rsComm, int fd, void *buf, int len);
int 
haawSubStructFileClose (rsComm_t *rsComm, int fd);
int 
haawSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile); 
int
haawSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile,
rodsStat_t **subStructFileStatOut); 
int
haawSubStructFileFstat (rsComm_t *rsComm, int fd, 
rodsStat_t **subStructFileStatOut);
rodsLong_t
haawSubStructFileLseek (rsComm_t *rsComm, int fd, rodsLong_t offset, int whence);
int 
haawSubStructFileRename (rsComm_t *rsComm, subFile_t *subFile, char *newFileName);
int
haawSubStructFileMkdir (rsComm_t *rsComm, subFile_t *subFile);
int
haawSubStructFileRmdir (rsComm_t *rsComm, subFile_t *subFile);
int
haawSubStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile);
int
haawSubStructFileReaddir (rsComm_t *rsComm, int fd, rodsDirent_t **rodsDirent);
int
haawSubStructFileClosedir (rsComm_t *rsComm, int fd);
int
haawSubStructFileTruncate (rsComm_t *rsComm, subFile_t *subFile);
int
haawStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp);

#endif	/* HAAW_STRUCT_FILE_DRIVER_H_H */
