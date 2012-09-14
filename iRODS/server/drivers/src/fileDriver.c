/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.c - The general driver for all file types. */

#include "fileDriver.h"
#include "fileDriverTable.h"
#include "eirods_log.h"
#include "eirods_resource_plugin.h"

// =-=-=-=-=-=-=-
// Top Level Inteface for Resource Plugin POSIX Create
eirods::error fileCreate( std::string _file_name, int _mode, size_t _file_size, int& _file_desc ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileCreate - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
      
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
        eirods::log( ret_err );
		return PASS( false, -1, "fileCreate - failed to resolve resource", ret_err );
	}
	   
	// =-=-=-=-=-=-=-
	// make the call to the "create" interface
	ret_err = resc->call< const char*, int, size_t, int* >( "create", _file_name.c_str(), _mode, _file_size, &_file_desc );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _file_desc, "fileCreate - failed to call 'create'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileCreate

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX Open
eirods::error fileOpen( std::string _file_name, int _mode, int _flags, int& _file_desc ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileOpen - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}

    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileOpen - failed to resolve resource", ret_err );
	}
    
	// =-=-=-=-=-=-=-
	// make the call to the "create" interface
	ret_err = resc->call< const char*, int, int, int* >( "open", _file_name.c_str(), _flags, _mode, &_file_desc );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _file_desc, "fileOpen - failed to call 'create'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileOpen

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX read
eirods::error fileRead( std::string _file_name, int _fd, void* _buf, int _len, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileRead - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
     
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileRead - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "read" interface
	ret_err = resc->call< int, void*, int, int* >( "read", _fd, _buf, _len, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileRead - failed to call 'read'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileRead

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX write
eirods::error fileWrite( std::string _file_name, int _fd, void* _buf, int  _len, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileWrite - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}

    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileWrite - failed to resolve resource", ret_err );
	}
    
	// =-=-=-=-=-=-=-
	// make the call to the "write" interface
	ret_err = resc->call< int, void*, int, int* >( "write", _fd, _buf, _len, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileWrite - failed to call 'write'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileWrite

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX close
eirods::error fileClose( std::string _file_name, int _fd, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileClose - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileClose - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "close" interface
	ret_err = resc->call< int, int* >( "close", _fd, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileClose - failed to call 'close'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileClose

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX unlink
eirods::error fileUnlink( std::string _file_name, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileUnlink - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileUnlink - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "unlink" interface
	ret_err = resc->call< const char*, int* >( "unlink", _file_name.c_str(), &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileUnlink - failed to call 'unlink'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileUnlink

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX stat
eirods::error fileStat( std::string _file_name, struct stat* _statbuf, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileStat - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileStat - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "stat" interface
	ret_err = resc->call< const char*, struct stat*, int* >( "stat", _file_name.c_str(), _statbuf, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileStat - failed to call 'stat'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileStat

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX fstat
eirods::error fileFstat( std::string _file_name, int _fd, struct stat* _statbuf, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileFstat - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileFstat - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "fstat" interface
	ret_err = resc->call< int, struct stat*, int* >( "fstat", _fd, _statbuf, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileFstat - failed to call 'fstat'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileFstat

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX lseek
eirods::error fileLseek( std::string _file_name, int _fd, size_t _offset, int _whence, size_t& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileLseek - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileLseek - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "lseek" interface
	ret_err = resc->call< int, size_t, int, size_t* >( "lseek", _fd, _offset, _whence, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileLseek - failed to call 'lseek'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileLseek

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX fsync
eirods::error fileFsync( std::string _file_name, int _fd, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileFsync - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileFsync - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "fsync" interface
	ret_err = resc->call< int, int* >( "fsync", _fd, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileFsync - failed to call 'fsync'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileFsync

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX mkdir
eirods::error fileMkdir( std::string _file_name, int _mode, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileMkdir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileMkdir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "mkdir" interface
	ret_err = resc->call< const char*, int, int* >( "mkdir", _file_name.c_str(), _mode, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileMkdir - failed to call 'mkdir'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileMkdir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX chmod
eirods::error fileChmod( std::string _file_name, int _mode, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileChmod - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileChmod - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "chmod" interface
	ret_err = resc->call< const char*, int, int* >( "chmod", _file_name.c_str(), _mode, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileChmod - failed to call 'chmod'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileChmod

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX rmdir
eirods::error fileRmdir( std::string _file_name, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileRmdir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileRmdir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "rmdir" interface
	ret_err = resc->call< const char*, int* >( "rmdir", _file_name.c_str(), &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileRmdir - failed to call 'rmdir'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileRmdir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX opendir
eirods::error fileOpendir ( std::string _file_name, void ** _out_dir_ptr, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileOpendir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileOpendir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "opendir" interface
	ret_err = resc->call< const char*, void**, int* >( "opendir", _file_name.c_str(), _out_dir_ptr, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileOpendir - failed to call 'opendir'", ret_err );
	} else {
        return SUCCESS();
	}
    
} // fileOpendir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX closedir
eirods::error fileClosedir( std::string _file_name, void* _dir_ptr, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileClosedir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileClosedir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "closedir" interface
	ret_err = resc->call< const char*, void*, int* >( "closedir",  _file_name.c_str(), _dir_ptr, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileClosedir - failed to call 'closedir'", ret_err );
	} else {
        return SUCCESS();
	}
    
} // fileClosedir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX readdir
eirods::error fileReaddir( std::string _file_name, void* _dir_ptr, struct dirent* _dirent_ptr, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileReaddir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileReaddir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "readdir" interface
	ret_err = resc->call< void*, struct dirent*, int* >( "readdir", _dir_ptr, _dirent_ptr, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileReaddir - failed to call 'readdir'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileReaddir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin stage
eirods::error fileStage( std::string _file_name, int _flag, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileStage - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileStage - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "stage" interface
	ret_err = resc->call< const char*, int, int* >( "stage",  _file_name.c_str(), _flag, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileStage - failed to call 'stage'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileStage

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX rename
eirods::error fileRename( std::string _old_file_name, std::string _new_file_name, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _old_file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileRename - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _old_file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileRename - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "rename" interface
	ret_err = resc->call< const char*, const char*, int* >( "rename",  _old_file_name.c_str(), _new_file_name.c_str(), 
	                      &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileRename - failed to call 'rename'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileRename

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin freespace
eirods::error fileGetFsFreeSpace( std::string _file_name, int _flag, size_t& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileGetFsFreeSpace - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileGetFsFreeSpace - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "freespace" interface
	ret_err = resc->call< const char*, int, size_t* >( "freespace",  _file_name.c_str(), _flag, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileGetFsFreeSpace - failed to call 'stage'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileGetFsFreeSpace

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin truncate
eirods::error fileTruncate( std::string _file_name, size_t _size, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileTruncate - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileTruncate - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "truncate" interface
	ret_err = resc->call< const char*, size_t, int* >( "truncate",  _file_name.c_str(), _size, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileTruncate - failed to call 'truncate'", ret_err );
	} else {
        return SUCCESS();
	}
   
} // fileTruncate

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin StageToCache
eirods::error fileStageToCache( std::string _file_name, std::string _cache_file_name, int _mode, int _flags, 
                                size_t _data_size, keyValPair_t* _cond_input, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileStageToCache - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileStageToCache - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "stagetocache" interface
	ret_err = resc->call< const char*, const char*, int, int, size_t, keyValPair_t*, int*  >( 
	                      "stagetocache",  _file_name.c_str(), _cache_file_name.c_str(),
						  _mode, _flags, _data_size, _cond_input, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileStageToCache - failed to call 'stagetocache'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileStageToCache

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin SyncToArch
eirods::error fileSyncToArch( std::string _file_name, std::string _cache_file_name, int _mode, int _flags, 
					          size_t _data_size, keyValPair_t* _cond_input, int& _status ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileSyncToArch - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve_from_path( _file_name, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileSyncToArch - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "synctoarch" interface
	ret_err = resc->call< const char*, const char*, int, int, size_t, keyValPair_t*, int*  >( 
	                      "synctoarch",  _file_name.c_str(), _cache_file_name.c_str(),
						  _mode, _flags, _data_size, _cond_input, &_status );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _status, "fileSyncToArch - failed to call 'synctoarch'", ret_err );
	} else {
        return SUCCESS();
	}

} // fileSyncToArch



