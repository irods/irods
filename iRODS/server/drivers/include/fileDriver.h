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

#include "eirods_error.h"


typedef struct {
    fileDriverType_t	driverType; 
    int         	(*fileCreate)( rsComm_t*, char*, int, rodsLong_t ); /* JMC */
    int         	(*fileOpen)( rsComm_t*, char*, int, int ); /* JMC */
    int         	(*fileRead)( rsComm_t*, int, void*, int ); /* JMC */
    int         	(*fileWrite)( rsComm_t*, int, void*, int ); /* JMC */
    int         	(*fileClose)( rsComm_t*, int ); /* JMC */
    int         	(*fileUnlink)( rsComm_t*, char* ); /* JMC */
    int         	(*fileStat)( rsComm_t*, char*, struct stat* ); /* JMC */
    int         	(*fileFstat)( rsComm_t*, int, struct stat* ); /* JMC */
    rodsLong_t  	(*fileLseek)( rsComm_t*, int, rodsLong_t, int ); /* JMC */
    int         	(*fileFsync)( rsComm_t*, int ); /* JMC */
    int         	(*fileMkdir)( rsComm_t*, char *, int ); /* JMC */
    int         	(*fileChmod)( rsComm_t*, char*, int ); /* JMC */
    int         	(*fileRmdir)( rsComm_t*, char* ); /* JMC */
    int         	(*fileOpendir)( rsComm_t*, char*, void** ); /* JMC */
    int         	(*fileClosedir)( rsComm_t*, void* ); /* JMC */
    int         	(*fileReaddir)( rsComm_t*, void*, struct dirent* ); /* JMC */
    int         	(*fileStage)( rsComm_t*, char*, int ); /* JMC */
    int         	(*fileRename)( rsComm_t*, char*, char* ); /* JMC */
    rodsLong_t  	(*fileGetFsFreeSpace)( rsComm_t*, char*, int ); /* JMC */
    int         	(*fileTruncate)( rsComm_t*, char*, rodsLong_t ); /* JMC */
    int			(*fileStageToCache)( rsComm_t*, fileDriverType_t, int, int, char*, char*, rodsLong_t, keyValPair_t* ); /* JMC */
    int			(*fileSyncToArch)( rsComm_t*, fileDriverType_t, int, int, char*, char*, rodsLong_t, keyValPair_t*); /* JMC */
} fileDriver_t;

eirods::error fileCreate  ( std::string _file_name, int _mode, size_t _file_size, int& _file_desc );
eirods::error fileOpen    ( std::string _file_name, int _mode, int    _flags,     int& _file_desc );
eirods::error fileRead    ( std::string _file_name, int _fd,   void*  _buf,       int  _len,      int& _status );
eirods::error fileWrite   ( std::string _file_name, int _fd,   void*  _buf,       int  _len,      int& _status );
eirods::error fileClose   ( std::string _file_name, int _fd,   int&   _status );
eirods::error fileUnlink  ( std::string _file_name, int& _status );
eirods::error fileStat    ( std::string _file_name, struct stat* _statbuf, int& _status );
eirods::error fileFstat   ( std::string _file_name, int _fd, struct stat* _statbuf, int& _status );
eirods::error fileLseek   ( std::string _file_name, int _fd, size_t _offset, int _whence, size_t& _status ); 
eirods::error fileFsync   ( std::string _file_name, int _fd, int& _status );
eirods::error fileMkdir   ( std::string _file_name, int _mode, int& _status );
eirods::error fileChmod   ( std::string _file_name, int _mode, int& _status );
eirods::error fileRmdir   ( std::string _file_name, int& _status );
eirods::error fileOpendir ( std::string _file_name, void** _out_dir_ptr, int& _status ); 
eirods::error fileClosedir( std::string _file_name, void*  _dir_ptr,     int& _status );
eirods::error fileReaddir ( std::string _file_name, void* _dir_ptr, struct dirent* _dirent_ptr, int& _status );
eirods::error fileStage   ( std::string _file_name, int _flag, int& _status );
eirods::error fileRename  ( std::string _old_file_name, std::string _new_file_name, int& _status );
eirods::error fileGetFsFreeSpace( std::string _file_name, int _flag, size_t& _status );
eirods::error fileTruncate( std::string _file_name, size_t _size, int& _status );
eirods::error fileStageToCache( std::string _file_name, std::string _cache_file_name, int _mode, int _flags, 
                                size_t _data_size, keyValPair_t* _cond_input, int& _status );
eirods::error fileSyncToArch( std::string _file_name, std::string _cache_file_name, int _mode, int _flags, 
					          size_t _data_size, keyValPair_t* _cond_input, int& _status );

#endif	/* FILE_DRIVER_H */
