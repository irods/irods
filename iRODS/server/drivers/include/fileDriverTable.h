/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriverTable.h - header file for the global file driver table
 */



#ifndef FILE_DRIVER_TABLE_H
#define FILE_DRIVER_TABLE_H

#include "rods.h"
#include "fileDriver.h"
#ifdef _WIN32
#include "ntFileDriver.h"
#else
#include "unixFileDriver.h"
#endif
#include "miscServerFunct.h"
#ifdef HPSS
#include "hpssFileDriver.h"
#endif
#ifdef AMAZON_S3
#include "s3FileDriver.h"
#endif
#include "univMSSDriver.h"
#ifdef DDN_WOS
#include "wosFileDriver.h"
#endif
#include "msoFileDriver.h"

// =-=-=-=-=-=-=-
// JMC - moving from intNoSupport to no-op fcns with matching signatures for g++
#include "fileDriverNoOpFunctions.h"

/*
#define NO_FILE_DRIVER_FUNCTIONS intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,longNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport, intNoSupport, longNoSupport, intNoSupport, intNoSupport, intNoSupport

fileDriver_t FileDriverTable[] = {
#ifndef windows_platform
    {UNIX_FILE_TYPE, unixFileCreate, unixFileOpen, unixFileRead, unixFileWrite,
    unixFileClose, unixFileUnlink, unixFileStat, unixFileFstat, unixFileLseek,
    unixFileFsync, unixFileMkdir, unixFileChmod, unixFileRmdir, unixFileOpendir,
    unixFileClosedir, unixFileReaddir, unixFileStage, unixFileRename,
    unixFileGetFsFreeSpace, unixFileTruncate, intNoSupport, intNoSupport},
#ifdef HPSS
    {HPSS_FILE_TYPE, intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    intNoSupport, hpssFileUnlink, hpssFileStat, intNoSupport, longNoSupport,
    intNoSupport, hpssFileMkdir, hpssFileChmod, hpssFileRmdir, hpssFileOpendir,
    hpssFileClosedir, hpssFileReaddir, intNoSupport, hpssFileRename,
    hpssFileGetFsFreeSpace, intNoSupport, hpssStageToCache, hpssSyncToArch},
#else
    {HPSS_FILE_TYPE, NO_FILE_DRIVER_FUNCTIONS},
#endif
#else
	{NT_FILE_TYPE, ntFileCreate, ntFileOpen, ntFileRead, ntFileWrite,
    ntFileClose, ntFileUnlink, ntFileStat, ntFileFstat, ntFileLseek,
    intNoSupport, ntFileMkdir, ntFileChmod, ntFileRmdir, ntFileOpendir,
    ntFileClosedir, ntFileReaddir, intNoSupport, ntFileRename,
    longNoSupport, intNoSupport, intNoSupport, intNoSupport},
#endif

#ifndef windows_platform
#ifdef AMAZON_S3
    {S3_FILE_TYPE, intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    intNoSupport, s3FileUnlink, s3FileStat, intNoSupport, longNoSupport,
    intNoSupport, s3FileMkdir, s3FileChmod, s3FileRmdir, intNoSupport,
    intNoSupport, intNoSupport, intNoSupport, s3FileRename,
    s3FileGetFsFreeSpace, intNoSupport, s3StageToCache, s3SyncToArch},
#else
    {S3_FILE_TYPE, NO_FILE_DRIVER_FUNCTIONS},
#endif
    {TEST_STAGE_FILE_TYPE,intNoSupport,intNoSupport, intNoSupport, intNoSupport,
    intNoSupport, unixFileUnlink, unixFileStat, unixFileFstat, longNoSupport,
    intNoSupport, unixFileMkdir, unixFileChmod, unixFileRmdir, unixFileOpendir,
    unixFileClosedir, unixFileReaddir, intNoSupport, unixFileRename,
    unixFileGetFsFreeSpace, intNoSupport, unixStageToCache, unixSyncToArch},
    {UNIV_MSS_FILE_TYPE,intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    intNoSupport, univMSSFileUnlink, univMSSFileStat, intNoSupport, longNoSupport,
    intNoSupport, univMSSFileMkdir, univMSSFileChmod, intNoSupport, intNoSupport,
    intNoSupport, intNoSupport, intNoSupport, univMSSFileRename,
    longNoSupport, intNoSupport, univMSSStageToCache, univMSSSyncToArch},
#endif
#ifdef DDN_WOS
    {WOS_FILE_TYPE, intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    intNoSupport, wosFileUnlink, wosFileStat, intNoSupport, longNoSupport,
    intNoSupport, intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    wosFileGetFsFreeSpace, intNoSupport, wosStageToCache, wosSyncToArch},
#else
    {WOS_FILE_TYPE, NO_FILE_DRIVER_FUNCTIONS},
#endif
    {MSO_FILE_TYPE, intNoSupport, intNoSupport, intNoSupport, intNoSupport,
     intNoSupport, msoFileUnlink, msoFileStat, intNoSupport, longNoSupport,
    intNoSupport, intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    intNoSupport, intNoSupport, intNoSupport, intNoSupport,
    msoFileGetFsFreeSpace, intNoSupport, msoStageToCache, msoSyncToArch},
    {NON_BLOCKING_FILE_TYPE,unixFileCreate,unixFileOpen,nbFileRead,nbFileWrite,
    unixFileClose, unixFileUnlink, unixFileStat, unixFileFstat, unixFileLseek,
    unixFileFsync, unixFileMkdir, unixFileChmod, unixFileRmdir, unixFileOpendir,
    unixFileClosedir, unixFileReaddir, unixFileStage, unixFileRename,
    unixFileGetFsFreeSpace, unixFileTruncate, intNoSupport, intNoSupport},
};
*/


#define NO_FILE_DRIVER_FUNCTIONS \
noSupportFsFileCreate, \
noSupportFsFileOpen, \
noSupportFsFileRead, \
noSupportFsFileWrite, \
noSupportFsFileClose, \
noSupportFsFileUnlink, \
noSupportFsFileStat, \
noSupportFsFileFstat, \
noSupportFsFileLseek, \
noSupportFsFileFsync, \
noSupportFsFileMkdir, \
noSupportFsFileChmod, \
noSupportFsFileRmdir, \
noSupportFsFileOpendir, \
noSupportFsFileClosedir, \
noSupportFsFileReaddir, \
noSupportFsFileStage, \
noSupportFsFileRename, \
noSupportFsFileGetFsFreeSpace, \
noSupportFsFileTruncate, \
noSupportFsFileStageToCache, \
noSupportFsFileSyncToArch



fileDriver_t FileDriverTable[] = {

#ifndef windows_platform
    { UNIX_FILE_TYPE, unixFileCreate, unixFileOpen, unixFileRead, unixFileWrite,
      unixFileClose, unixFileUnlink, unixFileStat, unixFileFstat, unixFileLseek,
      unixFileFsync, unixFileMkdir, unixFileChmod, unixFileRmdir, unixFileOpendir,
      unixFileClosedir, unixFileReaddir, unixFileStage, unixFileRename,
      unixFileGetFsFreeSpace, unixFileTruncate, noSupportFsFileStageToCache, 
      noSupportFsFileSyncToArch},
    #ifdef HPSS
        {HPSS_FILE_TYPE, noSupportFsFileCreate, noSupportFsFileOpen, noSupportFsFileRead, 
         noSupportFsFileWrite, noSupportFsFileClose, hpssFileUnlink, hpssFileStat, 
         noSupportFsFileFstat,noSupportFsFileLseek,noSupportFsFileFsync,hpssFileMkdir, 
         hpssFileChmod, hpssFileRmdir, hpssFileOpendir, hpssFileClosedir, hpssFileReaddir, 
         noSupportFsFileStage, hpssFileRename, hpssFileGetFsFreeSpace, noSupportFsFileTruncate,
         hpssStageToCache, hpssSyncToArch },
    #else
        {HPSS_FILE_TYPE, NO_FILE_DRIVER_FUNCTIONS},
    #endif
#else
	{NT_FILE_TYPE, ntFileCreate, ntFileOpen, ntFileRead, ntFileWrite,
         ntFileClose, ntFileUnlink, ntFileStat, ntFileFstat, ntFileLseek,
         noSupportFsFileFsync, ntFileMkdir, ntFileChmod, ntFileRmdir, ntFileOpendir,
         ntFileClosedir, ntFileReaddir, noSupportFsFileStage, ntFileRename, 
         noSupportFsFileGetFsFreeSpace, noSupportFsFileTruncate, noSupportFsFileStageToCache, 
         noSupportFsFileSyncToArch },
#endif

#ifndef windows_platform
   #ifdef AMAZON_S3
       {S3_FILE_TYPE, noSupportFsFileCreate, noSupportFsFileOpen, noSupportFsFileRead, 
        noSupportFsFileWrite, noSupportFsFileClose, s3FileUnlink, s3FileStat, noSupportFsFileFstat, 
        noSupportFsFileLseek, noSupportFsFileFsync, s3FileMkdir, s3FileChmod, s3FileRmdir, 
        noSupportFsFileOpendir, noSupportFsFileClosedir, noSupportFsFileReaddir, noSupportFsFileStage, 
        s3FileRename, s3FileGetFsFreeSpace, noSupportFsFileTruncate, s3StageToCache, s3SyncToArch},
    #else
        {S3_FILE_TYPE, NO_FILE_DRIVER_FUNCTIONS},
    #endif
    {TEST_STAGE_FILE_TYPE, noSupportFsFileCreate, noSupportFsFileOpen, noSupportFsFileRead, 
     noSupportFsFileWrite, noSupportFsFileClose, unixFileUnlink, unixFileStat, unixFileFstat, 
     noSupportFsFileLseek, noSupportFsFileFsync, unixFileMkdir, unixFileChmod, unixFileRmdir, 
     unixFileOpendir, unixFileClosedir, unixFileReaddir, noSupportFsFileStage, unixFileRename, 
     unixFileGetFsFreeSpace, noSupportFsFileTruncate, unixStageToCache, unixSyncToArch},

    {UNIV_MSS_FILE_TYPE, noSupportFsFileCreate, noSupportFsFileOpen, noSupportFsFileRead, 
     noSupportFsFileWrite, noSupportFsFileClose, univMSSFileUnlink, univMSSFileStat, 
     noSupportFsFileFstat, noSupportFsFileLseek, noSupportFsFileFsync, univMSSFileMkdir, 
     univMSSFileChmod, noSupportFsFileRmdir, noSupportFsFileOpendir, noSupportFsFileClosedir, 
     noSupportFsFileReaddir, noSupportFsFileStage, noSupportFsFileRename, 
     noSupportFsFileGetFsFreeSpace, noSupportFsFileTruncate, univMSSStageToCache, univMSSSyncToArch},
#endif

#ifdef DDN_WOS
    {WOS_FILE_TYPE, noSupportFsFileCreate, noSupportFsFileOpen, noSupportFsFileRead, 
     noSupportFsFileWrite, noSupportFsFileClose, wosFileUnlink, wosFileStat, noSupportFsFileFstat, 
     noSupportFsFileLseek, noSupportFsFileFsync, noSupportFsFileMkdir, noSupportFsFileChmod, 
     noSupportFsFileRmdir, noSupportFsFileOpendir, noSupportFsFileClosedir, noSupportFsFileReaddir, 
     noSupportFsFileStage, noSupportFsFileRename, wosFileGetFsFreeSpace, noSupportFsFileTruncate, 
     wosStageToCache, wosSyncToArch},
#else
    {WOS_FILE_TYPE, NO_FILE_DRIVER_FUNCTIONS},
#endif
    {MSO_FILE_TYPE, noSupportFsFileCreate, noSupportFsFileOpen, noSupportFsFileRead,
     noSupportFsFileWrite, noSupportFsFileClose, msoFileUnlink, msoFileStat, noSupportFsFileFstat,
     noSupportFsFileLseek, noSupportFsFileFsync, noSupportFsFileMkdir, noSupportFsFileChmod,
     noSupportFsFileRmdir, noSupportFsFileOpendir, noSupportFsFileClosedir, noSupportFsFileReaddir,
     noSupportFsFileStage, noSupportFsFileRename, msoFileGetFsFreeSpace, noSupportFsFileTruncate,
     msoStageToCache, msoSyncToArch},
    {NON_BLOCKING_FILE_TYPE,unixFileCreate,unixFileOpen,nbFileRead,nbFileWrite,
     unixFileClose, unixFileUnlink, unixFileStat, unixFileFstat, unixFileLseek,
     unixFileFsync, unixFileMkdir, unixFileChmod, unixFileRmdir, unixFileOpendir,
     unixFileClosedir, unixFileReaddir, unixFileStage, unixFileRename,
     unixFileGetFsFreeSpace, unixFileTruncate, noSupportFsFileStageToCache,
      noSupportFsFileSyncToArch},

};






int NumFileDriver = sizeof (FileDriverTable) / sizeof (fileDriver_t);

#endif	/* FILE_DRIVER_TABLE_H */
