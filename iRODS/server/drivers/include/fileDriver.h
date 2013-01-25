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
eirods::error fileCreate  ( eirods::first_class_object& );
eirods::error fileOpen    ( eirods::first_class_object& );
eirods::error fileRead    ( eirods::first_class_object&, void*, int );
eirods::error fileWrite   ( eirods::first_class_object&, void*, int );
eirods::error fileClose   ( eirods::first_class_object& );
eirods::error fileUnlink  ( eirods::first_class_object& );
eirods::error fileStat    ( eirods::first_class_object&, struct stat* );
eirods::error fileFstat   ( eirods::first_class_object&, struct stat* );
eirods::error fileLseek   ( eirods::first_class_object&, size_t, int ); 
eirods::error fileFsync   ( eirods::first_class_object& );
eirods::error fileMkdir   ( eirods::first_class_object& );
eirods::error fileChmod   ( eirods::first_class_object& );
eirods::error fileRmdir   ( eirods::first_class_object& );
eirods::error fileOpendir ( eirods::first_class_object& );
eirods::error fileClosedir( eirods::first_class_object& );
eirods::error fileReaddir ( eirods::first_class_object&, struct rodsDirent** );
eirods::error fileRename  ( eirods::first_class_object&, std::string );
eirods::error fileGetFsFreeSpace( eirods::first_class_object& );
eirods::error fileTruncate( eirods::first_class_object& );
eirods::error fileStage   ( eirods::first_class_object& );


eirods::error fileStageToCache( std::string _file_name, std::string _cache_file_name, int _mode, int _flags, 
                                size_t _data_size, keyValPair_t* _cond_input, int& _status );
eirods::error fileSyncToArch( std::string _file_name, std::string _cache_file_name, int _mode, int _flags, 
					          size_t _data_size, keyValPair_t* _cond_input, int& _status );

#endif	/* FILE_DRIVER_H */
