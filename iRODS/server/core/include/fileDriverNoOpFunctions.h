
// This file holds declarations for no-op functions used to satisfy the file driver tables
// for both the structured file and file-system drivers.  this was necessary as the g++ 
// compiler will not allow function pointer declarations with no parameters within its signature 
// to be bound to functions with parameters within its signature. -JMC

#ifndef __FILE_DRIVER_NO_OP_FUNCTIONS_H__
#define __FILE_DRIVER_NO_OP_FUNCTIONS_H__

#include "rcConnect.h"
#include "structFileSync.h"

// =-=-=-=-=-=-=-
// used within "server/drivers/include/structFileDriverTable.h"
int noSupportStructFileCreate( rsComm_t*, subFile_t* ); 
int noSupportStructFileTruncate( rsComm_t*, subFile_t* ); 
int noSupportStructFileUnlink( rsComm_t*, subFile_t* ); 
int noSupportStructFileOpen( rsComm_t*, subFile_t* ); 
int noSupportStructFileMkdir( rsComm_t*, subFile_t* );
int noSupportStructFileRmdir( rsComm_t*, subFile_t* );
int noSupportStructFileClose( rsComm_t*, int );
int noSupportStructFileRead( rsComm_t*, int, void*, int ); 
int noSupportStructFileWrite( rsComm_t*, int, void*, int );
int noSupportStructFileOpendir( rsComm_t*, subFile_t* );
int noSupportStructFileClosedir( rsComm_t*, int );
int noSupportStructFileStat( rsComm_t*, subFile_t*, rodsStat_t** );
int noSupportStructFileFstat( rsComm_t*, int, rodsStat_t** );
rodsLong_t noSupportStructFileLseek( rsComm_t*, int, rodsLong_t, int );
int noSupportStructFileRename( rsComm_t*, subFile_t*, char* );
int noSupportStructFileReaddir( rsComm_t*, int, rodsDirent_t** );
int noSupportStructeSync( rsComm_t*, structFileOprInp_t*);
int noSupportStructeExtract( rsComm_t*, structFileOprInp_t* );


// =-=-=-=-=-=-=-
// used within "server/drivers/include/fileDriverTable.h"
int        noSupportFsFileCreate( rsComm_t*, char*, int, rodsLong_t ); 
int        noSupportFsFileOpen( rsComm_t*, char*, int, int );
int        noSupportFsFileRead( rsComm_t*, int, void*, int );
int        noSupportFsFileWrite( rsComm_t*, int, void*, int );
int        noSupportFsFileClose( rsComm_t*, int );
int        noSupportFsFileUnlink( rsComm_t*, char* );
int        noSupportFsFileStat( rsComm_t*, char*, struct stat* );
int        noSupportFsFileFstat( rsComm_t*, int, struct stat* ); 
rodsLong_t noSupportFsFileLseek( rsComm_t*, int, rodsLong_t, int ); 
int        noSupportFsFileFsync( rsComm_t*, int ); 
int        noSupportFsFileMkdir( rsComm_t*, char *, int );
int        noSupportFsFileChmod( rsComm_t*, char*, int );
int        noSupportFsFileRmdir( rsComm_t*, char* ); 
int        noSupportFsFileOpendir( rsComm_t*, char*, void** ); 
int        noSupportFsFileClosedir( rsComm_t*, void* ); 
int        noSupportFsFileReaddir( rsComm_t*, void*, struct dirent* );
int        noSupportFsFileStage( rsComm_t*, char*, int );
int        noSupportFsFileRename( rsComm_t*, char*, char* );
rodsLong_t noSupportFsFileGetFsFreeSpace( rsComm_t*, char*, int ); 
int        noSupportFsFileTruncate( rsComm_t*, char*, rodsLong_t );
int        noSupportFsFileStageToCache( rsComm_t*, fileDriverType_t, int, int, char*, 
                                        char*, rodsLong_t, keyValPair_t* );
int        noSupportFsFileSyncToArch( rsComm_t*, fileDriverType_t, int, int, char*, char*, 
                              rodsLong_t, keyValPair_t*);
#endif // __FILE_DRIVER_NO_OP_FUNCTIONS_H__ 



