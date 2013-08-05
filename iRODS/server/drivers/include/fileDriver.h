/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.h - common header file for file driver
 */



#ifndef FILE_DRIVER_H
#define FILE_DRIVER_H

#ifndef windows_platform
#include <dirent.h>
#endif

#include "rods.h"
#include "rcConnect.h"
#include "objInfo.h"
#include "msParam.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"
#include "eirods_first_class_object.h"
eirods::error fileCreate  ( rsComm_t*, eirods::first_class_object& );
eirods::error fileOpen    ( rsComm_t*, eirods::first_class_object& );
eirods::error fileRead    ( rsComm_t*, eirods::first_class_object&, void*, int );
eirods::error fileWrite   ( rsComm_t*, eirods::first_class_object&, void*, int );
eirods::error fileClose   ( rsComm_t*, eirods::first_class_object& );
eirods::error fileUnlink  ( rsComm_t*, eirods::first_class_object& );
eirods::error fileStat    ( rsComm_t*, eirods::first_class_object&, struct stat* );
eirods::error fileFstat   ( rsComm_t*, eirods::first_class_object&, struct stat* );
eirods::error fileLseek   ( rsComm_t*, eirods::first_class_object&, rodsLong_t, int ); 
eirods::error fileFsync   ( rsComm_t*, eirods::first_class_object& );
eirods::error fileMkdir   ( rsComm_t*, eirods::first_class_object& );
eirods::error fileRmdir   ( rsComm_t*, eirods::first_class_object& );
eirods::error fileOpendir ( rsComm_t*, eirods::first_class_object& );
eirods::error fileClosedir( rsComm_t*, eirods::first_class_object& );
eirods::error fileReaddir ( rsComm_t*, eirods::first_class_object&, struct rodsDirent** );
eirods::error fileRename  ( rsComm_t*, eirods::first_class_object&, const std::string& );
eirods::error fileGetFsFreeSpace( rsComm_t*, eirods::first_class_object& );
eirods::error fileStageToCache( rsComm_t*, eirods::first_class_object&, const std::string& );
eirods::error fileSyncToArch  ( rsComm_t*, eirods::first_class_object&, const std::string& );
eirods::error fileRegistered(rsComm_t* _comm, eirods::first_class_object& _object );
eirods::error fileUnregistered(rsComm_t* _comm, eirods::first_class_object& _object );
eirods::error fileModified(rsComm_t* _comm, eirods::first_class_object& _object );
#endif	/* FILE_DRIVER_H */
