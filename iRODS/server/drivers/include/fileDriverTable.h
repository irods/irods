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


#ifdef XXXUSE_EIRODS
#include "eirods_fd_plugin.h"
namespace eirods {

    // =-=-=-=-=-=-=-
	// use ctor for initialization	
	//template<>
	class fd_table : public lookup_table< fileDriver_t, int, hash<int> > {
        public:	
        fd_table() : lookup_table< fileDriver_t, int, hash<int> > () {
        #ifndef windows_platform
			// =-=-=-=-=-=-=-
			// init UNIX_FILE_TYPE
			{   fileDriverType_t driver_index = UNIX_FILE_TYPE;
				table_[ driver_index ].fileCreate         = unixFileCreate;
				table_[ driver_index ].fileOpen           = unixFileOpen;
				table_[ driver_index ].fileRead           = unixFileRead;
				table_[ driver_index ].fileWrite          = unixFileWrite;
				table_[ driver_index ].fileClose          = unixFileClose;
				table_[ driver_index ].fileUnlink         = unixFileUnlink;
				table_[ driver_index ].fileStat           = unixFileStat;
				table_[ driver_index ].fileFstat          = unixFileFstat;
				table_[ driver_index ].fileLseek          = unixFileLseek;
				table_[ driver_index ].fileFsync          = unixFileFsync;
				table_[ driver_index ].fileMkdir          = unixFileMkdir;
				table_[ driver_index ].fileChmod          = unixFileChmod;
				table_[ driver_index ].fileRmdir          = unixFileRmdir;
				table_[ driver_index ].fileOpendir        = unixFileOpendir;
				table_[ driver_index ].fileClosedir       = unixFileClosedir;
				table_[ driver_index ].fileReaddir        = unixFileReaddir;
				table_[ driver_index ].fileStage          = unixFileStage;
				table_[ driver_index ].fileRename         = unixFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = unixFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = unixFileTruncate;
				table_[ driver_index ].fileStageToCache   = unixStageToCache;
				table_[ driver_index ].fileSyncToArch     = unixSyncToArch;    }
			// =-=-=-=-=-=-=-
			// init TEST_STAGE_FILE_TYPE
			{   fileDriverType_t driver_index =	TEST_STAGE_FILE_TYPE;
				table_[ driver_index ].fileCreate         = noSupportFsFileCreate;
				table_[ driver_index ].fileOpen           = noSupportFsFileOpen;
				table_[ driver_index ].fileRead           = noSupportFsFileRead;
				table_[ driver_index ].fileWrite          = noSupportFsFileWrite;
				table_[ driver_index ].fileClose          = noSupportFsFileClose;
				table_[ driver_index ].fileUnlink         = unixFileUnlink;
				table_[ driver_index ].fileStat           = unixFileStat;
				table_[ driver_index ].fileFstat          = unixFileFstat;
				table_[ driver_index ].fileLseek          = noSupportFsFileLseek;
				table_[ driver_index ].fileFsync          = noSupportFsFileFsync;
				table_[ driver_index ].fileMkdir          = unixFileMkdir;
				table_[ driver_index ].fileChmod          = unixFileChmod;
				table_[ driver_index ].fileRmdir          = unixFileRmdir;
				table_[ driver_index ].fileOpendir        = unixFileOpendir;
				table_[ driver_index ].fileClosedir       = unixFileClosedir;
				table_[ driver_index ].fileReaddir        = unixFileReaddir;
				table_[ driver_index ].fileStage          = noSupportFsFileStage;
				table_[ driver_index ].fileRename         = unixFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = unixFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = noSupportFsFileTruncate;
				table_[ driver_index ].fileStageToCache   = unixStageToCache;
				table_[ driver_index ].fileSyncToArch     = unixSyncToArch;    }
			// =-=-=-=-=-=-=-
			// init UNIV_MSS_FILE_TYPE
			{   fileDriverType_t driver_index =	UNIV_MSS_FILE_TYPE;
				table_[ driver_index ].fileCreate         = noSupportFsFileCreate;
				table_[ driver_index ].fileOpen           = noSupportFsFileOpen;
				table_[ driver_index ].fileRead           = noSupportFsFileRead;
				table_[ driver_index ].fileWrite          = noSupportFsFileWrite;
				table_[ driver_index ].fileClose          = noSupportFsFileClose;
				table_[ driver_index ].fileUnlink         = univMSSFileUnlink;
				table_[ driver_index ].fileStat           = univMSSFileStat;
				table_[ driver_index ].fileFstat          = noSupportFsFileFstat;
				table_[ driver_index ].fileLseek          = noSupportFsFileLseek;
				table_[ driver_index ].fileFsync          = noSupportFsFileFsync;
				table_[ driver_index ].fileMkdir          = univMSSFileMkdir;
				table_[ driver_index ].fileChmod          = univMSSFileChmod;
				table_[ driver_index ].fileRmdir          = noSupportFsFileRmdir;
				table_[ driver_index ].fileOpendir        = noSupportFsFileOpendir;
				table_[ driver_index ].fileClosedir       = noSupportFsFileClosedir;
				table_[ driver_index ].fileReaddir        = noSupportFsFileReaddir;
				table_[ driver_index ].fileStage          = noSupportFsFileStage;
				table_[ driver_index ].fileRename         = univMSSFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = noSupportFsFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = noSupportFsFileTruncate;
				table_[ driver_index ].fileStageToCache   = univMSSStageToCache;
				table_[ driver_index ].fileSyncToArch     = univMSSSyncToArch;    }
			// =-=-=-=-=-=-=-
			// init MSO_FILE_TYPE
			{   fileDriverType_t driver_index =	MSO_FILE_TYPE;
				table_[ driver_index ].fileCreate         = noSupportFsFileCreate;
				table_[ driver_index ].fileOpen           = noSupportFsFileOpen;
				table_[ driver_index ].fileRead           = noSupportFsFileRead;
				table_[ driver_index ].fileWrite          = noSupportFsFileWrite;
				table_[ driver_index ].fileClose          = noSupportFsFileClose;
				table_[ driver_index ].fileUnlink         = msoFileUnlink;
				table_[ driver_index ].fileStat           = msoFileStat;
				table_[ driver_index ].fileFstat          = noSupportFsFileFstat;
				table_[ driver_index ].fileLseek          = noSupportFsFileLseek;
				table_[ driver_index ].fileFsync          = noSupportFsFileFsync;
				table_[ driver_index ].fileMkdir          = noSupportFsFileMkdir;
				table_[ driver_index ].fileChmod          = noSupportFsFileChmod;
				table_[ driver_index ].fileRmdir          = noSupportFsFileRmdir;
				table_[ driver_index ].fileOpendir        = noSupportFsFileOpendir;
				table_[ driver_index ].fileClosedir       = noSupportFsFileClosedir;
				table_[ driver_index ].fileReaddir        = noSupportFsFileReaddir;
				table_[ driver_index ].fileStage          = noSupportFsFileStage;
				table_[ driver_index ].fileRename         = noSupportFsFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = noSupportFsFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = noSupportFsFileTruncate;
				table_[ driver_index ].fileStageToCache   = msoStageToCache;
				table_[ driver_index ].fileSyncToArch     = msoSyncToArch;    }
			// =-=-=-=-=-=-=-
			// init NON_BLOCKING_FILE_TYPE
			{   fileDriverType_t driver_index =	NON_BLOCKING_FILE_TYPE;
				table_[ driver_index ].fileCreate         = unixFileCreate;
				table_[ driver_index ].fileOpen           = unixFileOpen;
				table_[ driver_index ].fileRead           = nbFileRead;
				table_[ driver_index ].fileWrite          = nbFileWrite;
				table_[ driver_index ].fileClose          = unixFileClose;
				table_[ driver_index ].fileUnlink         = unixFileUnlink;
				table_[ driver_index ].fileStat           = unixFileStat;
				table_[ driver_index ].fileFstat          = unixFileFstat;
				table_[ driver_index ].fileLseek          = unixFileLseek;
				table_[ driver_index ].fileFsync          = unixFileFsync;
				table_[ driver_index ].fileMkdir          = unixFileMkdir;
				table_[ driver_index ].fileChmod          = unixFileChmod;
				table_[ driver_index ].fileRmdir          = unixFileRmdir;
				table_[ driver_index ].fileOpendir        = unixFileOpendir;
				table_[ driver_index ].fileClosedir       = unixFileClosedir;
				table_[ driver_index ].fileReaddir        = unixFileReaddir;
				table_[ driver_index ].fileStage          = unixFileStage;
				table_[ driver_index ].fileRename         = unixFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = unixFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = unixFileTruncate;
				table_[ driver_index ].fileStageToCache   = noSupportFsFileStageToCache;
				table_[ driver_index ].fileSyncToArch     = noSupportFsFileSyncToArch;    }
			// =-=-=-=-=-=-=-
			// init HPSS 
			#ifdef HPSS
			#else
			{   fileDriverType_t driver_index =	HPSS_FILE_TYPE;
				table_[ driver_index ].fileCreate         = noSupportFsFileCreate;
				table_[ driver_index ].fileRead           = noSupportFsFileRead;
				table_[ driver_index ].fileWrite          = noSupportFsFileWrite;
				table_[ driver_index ].fileClose          = noSupportFsFileClose;
				table_[ driver_index ].fileUnlink         = noSupportFsFileUnlink;
				table_[ driver_index ].fileStat           = noSupportFsFileStat;
				table_[ driver_index ].fileFstat          = noSupportFsFileFstat;
				table_[ driver_index ].fileLseek          = noSupportFsFileLseek;
				table_[ driver_index ].fileFsync          = noSupportFsFileFsync;
				table_[ driver_index ].fileMkdir          = noSupportFsFileMkdir;
				table_[ driver_index ].fileChmod          = noSupportFsFileChmod;
				table_[ driver_index ].fileRmdir          = noSupportFsFileRmdir;
				table_[ driver_index ].fileOpendir        = noSupportFsFileOpendir;
				table_[ driver_index ].fileClosedir       = noSupportFsFileClosedir;
				table_[ driver_index ].fileReaddir        = noSupportFsFileReaddir;
				table_[ driver_index ].fileStage          = noSupportFsFileStage;
				table_[ driver_index ].fileRename         = noSupportFsFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = noSupportFsFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = noSupportFsFileTruncate;
				table_[ driver_index ].fileStageToCache   = noSupportFsFileStageToCache;
				table_[ driver_index ].fileSyncToArch     = noSupportFsFileSyncToArch;    }
			#endif
			// =-=-=-=-=-=-=-
			// init AMAZON_S3 
			#ifdef AMAZON_S3
			#else
			{   fileDriverType_t driver_index =	S3_FILE_TYPE;
				table_[ driver_index ].fileCreate         = noSupportFsFileCreate;
				table_[ driver_index ].fileRead           = noSupportFsFileRead;
				table_[ driver_index ].fileWrite          = noSupportFsFileWrite;
				table_[ driver_index ].fileClose          = noSupportFsFileClose;
				table_[ driver_index ].fileUnlink         = noSupportFsFileUnlink;
				table_[ driver_index ].fileStat           = noSupportFsFileStat;
				table_[ driver_index ].fileFstat          = noSupportFsFileFstat;
				table_[ driver_index ].fileLseek          = noSupportFsFileLseek;
				table_[ driver_index ].fileFsync          = noSupportFsFileFsync;
				table_[ driver_index ].fileMkdir          = noSupportFsFileMkdir;
				table_[ driver_index ].fileChmod          = noSupportFsFileChmod;
				table_[ driver_index ].fileRmdir          = noSupportFsFileRmdir;
				table_[ driver_index ].fileOpendir        = noSupportFsFileOpendir;
				table_[ driver_index ].fileClosedir       = noSupportFsFileClosedir;
				table_[ driver_index ].fileReaddir        = noSupportFsFileReaddir;
				table_[ driver_index ].fileStage          = noSupportFsFileStage;
				table_[ driver_index ].fileRename         = noSupportFsFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = noSupportFsFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = noSupportFsFileTruncate;
				table_[ driver_index ].fileStageToCache   = noSupportFsFileStageToCache;
				table_[ driver_index ].fileSyncToArch     = noSupportFsFileSyncToArch;    }
			#endif
			// =-=-=-=-=-=-=-
			// init DDN_WOS 
			#ifdef DDN_WOS
			#else
			{   fileDriverType_t driver_index =	WOS_FILE_TYPE;
				table_[ driver_index ].fileCreate         = noSupportFsFileCreate;
				table_[ driver_index ].fileRead           = noSupportFsFileRead;
				table_[ driver_index ].fileWrite          = noSupportFsFileWrite;
				table_[ driver_index ].fileClose          = noSupportFsFileClose;
				table_[ driver_index ].fileUnlink         = noSupportFsFileUnlink;
				table_[ driver_index ].fileStat           = noSupportFsFileStat;
				table_[ driver_index ].fileFstat          = noSupportFsFileFstat;
				table_[ driver_index ].fileLseek          = noSupportFsFileLseek;
				table_[ driver_index ].fileFsync          = noSupportFsFileFsync;
				table_[ driver_index ].fileMkdir          = noSupportFsFileMkdir;
				table_[ driver_index ].fileChmod          = noSupportFsFileChmod;
				table_[ driver_index ].fileRmdir          = noSupportFsFileRmdir;
				table_[ driver_index ].fileOpendir        = noSupportFsFileOpendir;
				table_[ driver_index ].fileClosedir       = noSupportFsFileClosedir;
				table_[ driver_index ].fileReaddir        = noSupportFsFileReaddir;
				table_[ driver_index ].fileStage          = noSupportFsFileStage;
				table_[ driver_index ].fileRename         = noSupportFsFileRename;
				table_[ driver_index ].fileGetFsFreeSpace = noSupportFsFileGetFsFreeSpace;
				table_[ driver_index ].fileTruncate       = noSupportFsFileTruncate;
				table_[ driver_index ].fileStageToCache   = noSupportFsFileStageToCache;
				table_[ driver_index ].fileSyncToArch     = noSupportFsFileSyncToArch;    }
			#endif

        #endif // ifndef windows_platform 
        } // ctor
	}; // class fd_table

}; // namespace eirods

// =-=-=-=-=-=-=-
// Declare actual file driver table
eirods::fd_table FileDriverTable;

#else

fileDriver_t FileDriverTable[] = {

#ifndef windows_platform
    { UNIX_FILE_TYPE, unixFileCreate, unixFileOpen, unixFileRead, unixFileWrite,
      unixFileClose, unixFileUnlink, unixFileStat, unixFileFstat, unixFileLseek,
      unixFileFsync, unixFileMkdir, unixFileChmod, unixFileRmdir, unixFileOpendir,
      unixFileClosedir, unixFileReaddir, unixFileStage, unixFileRename,
      unixFileGetFsFreeSpace, unixFileTruncate, unixStageToCache,  // JMC - backport 4521
      unixSyncToArch},
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
     noSupportFsFileReaddir, noSupportFsFileStage, univMSSFileRename, // JMC - backport 4697 
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
#endif






#endif	/* FILE_DRIVER_TABLE_H */
