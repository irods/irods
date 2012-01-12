


#include "fileDriverNoOpFunctions.h"

// =-=-=-=-=-=-=-
// used within "server/drivers/include/structFileDriverTable.h"
int noSupportStructFileCreate( rsComm_t* a, subFile_t* b) { 
    return 0;
}
int noSupportStructFileTruncate( rsComm_t* a, subFile_t* b ) {
    return 0;
} 
int noSupportStructFileUnlink( rsComm_t* a, subFile_t* b ) {
    return 0;
} 
int noSupportStructFileOpen( rsComm_t* a, subFile_t* b ) {
    return 0;
} 
int noSupportStructFileMkdir( rsComm_t* a, subFile_t* b ) {
    return 0;
}
int noSupportStructFileRmdir( rsComm_t* a, subFile_t* b ) {
    return 0;
}
int noSupportStructFileClose( rsComm_t* a, int b ) {
    return 0;
}
int noSupportStructFileRead( rsComm_t* a, int b, void* c, int d ) {
    return 0;
} 
int noSupportStructFileWrite( rsComm_t* a, int b, void* c, int d ) {
    return 0;
}
int noSupportStructFileOpendir( rsComm_t* a, subFile_t* b ) {
    return 0;
}
int noSupportStructFileClosedir( rsComm_t* a, int b ) {
    return 0;
}
int noSupportStructFileStat( rsComm_t* a, subFile_t* b, rodsStat_t** c ) {
    return 0;
}
int noSupportStructFileFstat( rsComm_t* a, int b, rodsStat_t** c ) {
    return 0;
}
rodsLong_t noSupportStructFileLseek( rsComm_t* a, int b, rodsLong_t c, int d ) {
    return 0;
}
int noSupportStructFileRename( rsComm_t* a, subFile_t* b, char* c ) {
    return 0;
}
int noSupportStructFileReaddir( rsComm_t* a, int b, rodsDirent_t** c ) {
    return 0;
}
int noSupportStructeSync( rsComm_t* a, structFileOprInp_t* b ) {
    return 0;
}
int noSupportStructeExtract( rsComm_t* a, structFileOprInp_t* b ) {
    return 0;
}

// =-=-=-=-=-=-=-
// used within "server/drivers/include/fileDriverTable.h"
int        noSupportFsFileCreate( rsComm_t* a, char* b, int c, rodsLong_t d ) {
    return 0;
} 
int        noSupportFsFileOpen( rsComm_t* a, char* b, int c, int d ) {
    return 0;
}
int        noSupportFsFileRead( rsComm_t* a, int b, void* c, int d ) {
    return 0;
}
int        noSupportFsFileWrite( rsComm_t* a, int b, void* c, int d ) {
    return 0;
}
int        noSupportFsFileClose( rsComm_t* a, int b ) {
    return 0;
}
int        noSupportFsFileUnlink( rsComm_t* a, char* b ) {
    return 0;
}
int        noSupportFsFileStat( rsComm_t* a, char* b, struct stat* c ) {
    return 0;
}
int        noSupportFsFileFstat( rsComm_t* a, int b, struct stat* c ) {
    return 0;
} 
rodsLong_t noSupportFsFileLseek( rsComm_t* a, int b, rodsLong_t c, int d ) {
    return 0;
} 
int        noSupportFsFileFsync( rsComm_t* a, int b ) {
    return 0;
} 
int        noSupportFsFileMkdir( rsComm_t* a, char* b, int c ) {
    return 0;
}
int        noSupportFsFileChmod( rsComm_t* a, char* b, int c ) {
    return 0;
}
int        noSupportFsFileRmdir( rsComm_t* a, char* b ) {
    return 0;
} 
int        noSupportFsFileOpendir( rsComm_t* a, char* b, void** c ) {
    return 0;
} 
int        noSupportFsFileClosedir( rsComm_t* a, void* b ) {
    return 0;
} 
int        noSupportFsFileReaddir( rsComm_t* a, void* b, struct dirent* c ) {
    return 0;
}
int        noSupportFsFileStage( rsComm_t* a, char* b, int c ) {
    return 0;
}
int        noSupportFsFileRename( rsComm_t* a, char* b, char* c ) {
    return 0;
}
rodsLong_t noSupportFsFileGetFsFreeSpace( rsComm_t* a, char* b, int c ) {
    return 0;
} 
int        noSupportFsFileTruncate( rsComm_t* a, char* b, rodsLong_t c ) {
    return 0;
}
int        noSupportFsFileStageToCache( rsComm_t* a, fileDriverType_t b, int c, int d, char* e, 
                                        char* f, rodsLong_t g, keyValPair_t* h ) {
    return 0;
}
int        noSupportFsFileSyncToArch( rsComm_t* a, fileDriverType_t b, int c, int d, char* e, char* f, rodsLong_t g, keyValPair_t* h ) {
    return 0;
}
