/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.c - The general driver for all file types. */

#include "fileDriver.h"
#include "fileDriverTable.h"
#include "eirods_log.h"
#include "eirods_resource_plugin.h"
#include "eirods_first_class_object.h"

// =-=-=-=-=-=-=-
// Top Level Inteface for Resource Plugin POSIX Create
eirods::error fileCreate( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileCreate - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
      
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the object
    eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
        eirods::log( ret_err );
		return PASS( false, -1, "fileCreate - failed to resolve resource", ret_err );
	}
	   
	// =-=-=-=-=-=-=-
	// make the call to the "create" interface
	ret_err = resc->call< eirods::first_class_object* >( "create", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileCreate - failed to call 'create'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileCreate

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX Open
eirods::error fileOpen( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileOpen - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}

    // =-=-=-=-=-=-=-
	// retrieve the resource name given the object
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileOpen - failed to resolve resource", ret_err );
	}
    
	// =-=-=-=-=-=-=-
	// make the call to the "open" interface
	ret_err = resc->call< eirods::first_class_object* >( "open", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, _object.file_descriptor(), "fileOpen - failed to call 'open'", ret_err );
	} else {
        return CODE( _object.file_descriptor() );
	}

} // fileOpen

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX Write
eirods::error fileRead( eirods::first_class_object& _object, void* _buf, int _len ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileRead - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
     
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the object
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileRead - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "read" interface
	ret_err = resc->call< eirods::first_class_object*, void*, int >( "read", &_object, _buf, _len );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileRead - failed to call 'read'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileRead

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX Read
eirods::error fileWrite( eirods::first_class_object& _object, void* _buf, int  _len ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileWrite - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
   
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the object
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileWrite - failed to resolve resource", ret_err );
	}
    
	// =-=-=-=-=-=-=-
	// make the call to the "write" interface
	ret_err = resc->call< eirods::first_class_object*, void*, int >( "write", &_object, _buf, _len );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileWrite - failed to call 'write'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileWrite

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX close
eirods::error fileClose( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileClose - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileClose - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "close" interface
	ret_err = resc->call< eirods::first_class_object* >( "close", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileClose - failed to call 'close'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileClose

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX close
eirods::error fileUnlink( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileUnlink - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileUnlink - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "unlink" interface
	ret_err = resc->call< eirods::first_class_object* >( "unlink", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileUnlink - failed to call 'unlink'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileUnlink

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX stat
eirods::error fileStat( eirods::first_class_object& _object, struct stat* _statbuf ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileStat - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileStat - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "stat" interface
	ret_err = resc->call< eirods::first_class_object*, struct stat* >( "stat", &_object, _statbuf );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileStat - failed to call 'stat'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileStat

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX fstat
eirods::error fileFstat( eirods::first_class_object& _object, struct stat* _statbuf ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileFstat - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileFstat - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "fstat" interface
	ret_err = resc->call< eirods::first_class_object*, struct stat* >( "fstat", &_object, _statbuf );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileFstat - failed to call 'fstat'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileFstat

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX lseek
eirods::error fileLseek( eirods::first_class_object& _object, size_t _offset, int _whence ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileLseek - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileLseek - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "lseek" interface
	ret_err = resc->call< eirods::first_class_object*, size_t, int >( "lseek", &_object, _offset, _whence );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileLseek - failed to call 'lseek'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileLseek

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX fsync
eirods::error fileFsync( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileFsync - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileFsync - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "fsync" interface
	ret_err = resc->call< eirods::first_class_object* >( "fsync", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileFsync - failed to call 'fsync'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileFsync

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX mkdir
eirods::error fileMkdir( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileMkdir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileMkdir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "mkdir" interface
	ret_err = resc->call< eirods::first_class_object* >( "mkdir", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileMkdir - failed to call 'mkdir'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileMkdir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX mkdir
eirods::error fileChmod( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileChmod - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileChmod - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "chmod" interface
	ret_err = resc->call< eirods::first_class_object* >( "chmod", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileChmod - failed to call 'chmod'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileChmod

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX mkdir
eirods::error fileRmdir( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileRmdir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileRmdir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "rmdir" interface
	ret_err = resc->call< eirods::first_class_object* >( "rmdir", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileRmdir - failed to call 'rmdir'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileRmdir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX opendir
eirods::error fileOpendir( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileOpendir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileOpendir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "opendir" interface
	ret_err = resc->call< eirods::first_class_object* >( "opendir", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileOpendir - failed to call 'opendir'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}
    
} // fileOpendir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX close
eirods::error fileClosedir( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileClosedir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileClosedir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "closedir" interface
	ret_err = resc->call< eirods::first_class_object* >( "closedir", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileClosedir - failed to call 'closedir'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}
    
} // fileClosedir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX close
eirods::error fileReaddir( eirods::first_class_object& _object, struct dirent* _dirent_ptr ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileReaddir - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileReaddir - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "readdir" interface
	ret_err = resc->call< eirods::first_class_object*, struct dirent* >( "readdir", &_object, _dirent_ptr );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileReaddir - failed to call 'readdir'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileReaddir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin stage
eirods::error fileStage( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileStage - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileStage - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "stage" interface
	ret_err = resc->call< eirods::first_class_object* >( "stage", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileStage - failed to call 'stage'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileStage

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX rename
eirods::error fileRename( eirods::first_class_object& _object, std::string _new_file_name ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() || _new_file_name.empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileRename - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileRename - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "rename" interface
	ret_err = resc->call< eirods::first_class_object*, const char* >( "rename",  &_object, _new_file_name.c_str() ); 

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileRename - failed to call 'rename'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileRename

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin freespace
eirods::error fileGetFsFreeSpace( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileGetFsFreeSpace - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileGetFsFreeSpace - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "freespace" interface
	ret_err = resc->call< eirods::first_class_object* >( "freespace", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileGetFsFreeSpace - failed to call 'stage'", ret_err );
	} else {
        return CODE( ret_err.code() );
	}

} // fileGetFsFreeSpace

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin truncate
eirods::error fileTruncate( eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
	// trap empty file name
	if( _object.physical_path().empty() ) {
		eirods::error ret_err = ERROR( false, -1, "fileTruncate - File Name is Empty." );
		eirods::log( ret_err );
		return ret_err;
	}
    
    // =-=-=-=-=-=-=-
	// retrieve the resource name given the path
	eirods::resource_ptr resc;
    eirods::error ret_err = resc_mgr.resolve( _object, resc ); 
	if( !ret_err.ok() ) {
		return PASS( false, -1, "fileTruncate - failed to resolve resource", ret_err );
	}

	// =-=-=-=-=-=-=-
	// make the call to the "truncate" interface
	ret_err = resc->call< eirods::first_class_object* >( "truncate", &_object );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        return PASS( false, ret_err.code(), "fileTruncate - failed to call 'truncate'", ret_err );
	} else {
        return CODE( ret_err.code() );
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



