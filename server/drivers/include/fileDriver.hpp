/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.h - common header file for file driver
 */



#ifndef FILE_DRIVER_HPP
#define FILE_DRIVER_HPP

#ifndef windows_platform
#include <dirent.h>
#endif

#include "rods.h"
#include "rcConnect.h"
#include "objInfo.h"
#include "msParam.h"

// =-=-=-=-=-=-=-
#include "irods_error.hpp"
#include "irods_first_class_object.hpp"
irods::error fileCreate( rsComm_t*, irods::first_class_object_ptr );
irods::error fileOpen( rsComm_t*, irods::first_class_object_ptr );
irods::error fileRead( rsComm_t*, irods::first_class_object_ptr, void*, const int );
irods::error fileWrite( rsComm_t*, irods::first_class_object_ptr, const void*, const int );
irods::error fileClose( rsComm_t*, irods::first_class_object_ptr );
irods::error fileUnlink( rsComm_t*, irods::first_class_object_ptr );
irods::error fileStat( rsComm_t*, irods::first_class_object_ptr, struct stat* );
irods::error fileLseek( rsComm_t*, irods::first_class_object_ptr, const long long, const int );
irods::error fileMkdir( rsComm_t*, irods::first_class_object_ptr );
irods::error fileChmod( rsComm_t*, irods::first_class_object_ptr, int );
irods::error fileRmdir( rsComm_t*, irods::first_class_object_ptr );
irods::error fileOpendir( rsComm_t*, irods::first_class_object_ptr );
irods::error fileClosedir( rsComm_t*, irods::first_class_object_ptr );
irods::error fileReaddir( rsComm_t*, irods::first_class_object_ptr, struct rodsDirent** );
irods::error fileRename( rsComm_t*, irods::first_class_object_ptr, const std::string& );
irods::error fileGetFsFreeSpace( rsComm_t*, irods::first_class_object_ptr );
irods::error fileTruncate( rsComm_t*, irods::first_class_object_ptr );
irods::error fileStageToCache( rsComm_t*, irods::first_class_object_ptr, const std::string& );
irods::error fileSyncToArch( rsComm_t*, irods::first_class_object_ptr, const std::string& );
irods::error fileRegistered( rsComm_t* _comm, irods::first_class_object_ptr _object );
irods::error fileUnregistered( rsComm_t* _comm, irods::first_class_object_ptr _object );
irods::error fileModified( rsComm_t* _comm, irods::first_class_object_ptr _object );
irods::error fileNotify( rsComm_t* _comm, irods::first_class_object_ptr _object , const std::string& );
#endif	/* FILE_DRIVER_H */
