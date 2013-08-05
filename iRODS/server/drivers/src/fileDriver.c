/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.c - The general driver for all file types. */

#include "fileDriver.h"
#include "eirods_log.h"
#include "eirods_resource_plugin.h"
#include "eirods_first_class_object.h"
#include "eirods_stacktrace.h"

#include "eirods_resource_constants.h"
#include "eirods_resource_manager.h"
extern eirods::resource_manager resc_mgr;

// =-=-=-=-=-=-=-
// Top Level Inteface for Resource Plugin POSIX create
eirods::error fileCreate( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "file name is empty." );
        eirods::log( ret_err );
        return ret_err;
    }
      
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        eirods::log( ret_err );
        return PASSMSG( "failed to resolve resource", ret_err );
    }
           
    // =-=-=-=-=-=-=-
    // make the call to the "create" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_CREATE, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'create'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }


} // fileCreate

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX open
eirods::error fileOpen( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "file name is empty." );
        eirods::log( ret_err );
        return ret_err;
    }

    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }
    
    // =-=-=-=-=-=-=-
    // make the call to the "open" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_OPEN, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'open'", ret_err );
    
    } else {
        return CODE( _object.file_descriptor() );

    }

} // fileOpen

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX read
eirods::error fileRead( rsComm_t* _comm, eirods::first_class_object& _object, void* _buf, int _len ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    ret_err = resc->call< void*, int >( _comm, eirods::RESOURCE_OP_READ, _object, _buf, _len );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'read'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileRead

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX write
eirods::error fileWrite( rsComm_t* _comm, eirods::first_class_object& _object, void* _buf, int  _len ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }
    
    // =-=-=-=-=-=-=-
    // make the call to the "write" interface
    ret_err = resc->call< void*, int >( _comm, eirods::RESOURCE_OP_WRITE, _object, _buf, _len );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'write'", ret_err );
    } else {
        std::stringstream msg;
        msg << "Write successful.";
        return PASSMSG(msg.str(), ret_err);
    }

} // fileWrite

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX close
eirods::error fileClose( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "file name is empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "close" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_CLOSE, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'close'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileClose

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX unlink
eirods::error fileUnlink( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "file name is empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "unlink" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_UNLINK, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'unlink'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileUnlink

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX stat
eirods::error fileStat( rsComm_t* _comm, eirods::first_class_object& _object, struct stat* _statbuf ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "file name is empty." );
        eirods::log( ret_err );
        return ret_err;
    }
   
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "stat" interface
    ret_err = resc->call< struct stat* >( _comm, eirods::RESOURCE_OP_STAT, _object, _statbuf );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'stat'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileStat

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX fstat
eirods::error fileFstat( rsComm_t* _comm, eirods::first_class_object& _object, struct stat* _statbuf ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "file name is empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "fstat" interface
    ret_err = resc->call< struct stat* >( _comm, eirods::RESOURCE_OP_FSTAT, _object, _statbuf );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'fstat'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileFstat

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX lseek
eirods::error fileLseek( rsComm_t* _comm, eirods::first_class_object& _object, long long _offset, int _whence ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "file name is empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "lseek" interface
    ret_err = resc->call< long long, int >( _comm, eirods::RESOURCE_OP_LSEEK, _object, _offset, _whence );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'lseek'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileLseek

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX fsync
eirods::error fileFsync( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileFsync - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "fsync" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_FSYNC, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'fsync'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileFsync

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX mkdir
eirods::error fileMkdir( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileMkdir - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "mkdir" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_MKDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'mkdir'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileMkdir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX rmdir
eirods::error fileRmdir( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileRmdir - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "rmdir" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_RMDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'rmdir'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileRmdir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX opendir
eirods::error fileOpendir( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileOpendir - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "opendir" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_OPENDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'opendir'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }
    
} // fileOpendir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX closedir
eirods::error fileClosedir( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    //if( _object.physical_path().empty() ) {
    //      eirods::error ret_err = ERROR( false, SYS_INVALID_INPUT_PARAM, "fileClosedir - File Name is Empty." );
    //      eirods::log( ret_err );
    //      return ret_err;
    //}
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "closedir" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_CLOSEDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'closedir'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }
    
} // fileClosedir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX readdir
eirods::error fileReaddir( rsComm_t* _comm, eirods::first_class_object& _object, struct rodsDirent** _dirent_ptr ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "readdir" interface
    ret_err = resc->call< struct rodsDirent** >( _comm, eirods::RESOURCE_OP_READDIR, _object, _dirent_ptr );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'readdir'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileReaddir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX rename
eirods::error fileRename( rsComm_t*                   _comm, 
                          eirods::first_class_object& _object, 
                          const std::string&          _new_file_name ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() || _new_file_name.empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileRename - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "rename" interface
    ret_err = resc->call<  const char* >( _comm, eirods::RESOURCE_OP_RENAME,  _object, _new_file_name.c_str() );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'rename'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileRename

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin freespace
eirods::error fileGetFsFreeSpace( rsComm_t* _comm, eirods::first_class_object& _object ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileGetFsFreeSpace - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc ); 
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "freespace" interface
    ret_err = resc->call( _comm, eirods::RESOURCE_OP_FREESPACE, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'stage'", ret_err );
    } else {
        return CODE( ret_err.code() );
    }

} // fileGetFsFreeSpace

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin StageToCache
eirods::error fileStageToCache( rsComm_t*                   _comm, 
                                eirods::first_class_object& _object, 
                                const std::string&          _cache_file_name ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _cache_file_name.empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileStageToCache - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc );
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "stagetocache" interface
    ret_err = resc->call< const char* >( _comm, eirods::RESOURCE_OP_STAGETOCACHE, _object, _cache_file_name.c_str() );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'stagetocache'", ret_err );
    } else {
        return SUCCESS();
    }

} // fileStageToCache

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin SyncToArch
eirods::error fileSyncToArch( rsComm_t*                   _comm, 
                              eirods::first_class_object& _object, 
                              const std::string&          _cache_file_name ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _cache_file_name.empty() ) {
        eirods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileSyncToArch - File Name is Empty." );
        eirods::log( ret_err );
        return ret_err;
    }
    
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    eirods::resource_ptr resc;
    eirods::error ret_err = _object.resolve( resc_mgr, resc );
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "synctoarch" interface
    ret_err = resc->call< const char* >( _comm, eirods::RESOURCE_OP_SYNCTOARCH, _object, _cache_file_name.c_str() );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'synctoarch'", ret_err );
    } else {
        return SUCCESS();
    }

} // fileSyncToArch

// File registered with the database
eirods::error fileRegistered(
    rsComm_t* _comm,
    eirods::first_class_object& _object ) {
    eirods::error result = SUCCESS();
    eirods::error ret;
    
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - File name is empty.";
        result = ERROR(-1, msg.str());

    } else {
        // =-=-=-=-=-=-=-
        // retrieve the resource name given the object
        eirods::resource_ptr resc;
        ret = _object.resolve( resc_mgr, resc ); 
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to resolve resource.";
            result = PASSMSG(msg.str(), ret);

        } else {
            // =-=-=-=-=-=-=-
            // make the call to the "registered" interface
            ret = resc->call( _comm, eirods::RESOURCE_OP_REGISTERED, _object );
            if( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to call registered interface.";
                result = PASSMSG(msg.str(), ret);
            }
        }
    }
    return result;
} // fileRegistered

// File unregistered with the database
eirods::error fileUnregistered(
    rsComm_t* _comm,
    eirods::first_class_object& _object )
{
    eirods::error result = SUCCESS();
    eirods::error ret;
    
    // =-=-=-=-=-=-=-
    // trap empty file name
    if( _object.physical_path().empty() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - File name is empty.";
        result = ERROR(-1, msg.str());
    } else {

        // =-=-=-=-=-=-=-
        // retrieve the resource name given the object
        eirods::resource_ptr resc;
        ret = _object.resolve( resc_mgr, resc ); 
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to resolve resource.";
            result = PASSMSG(msg.str(), ret);
        } else {
        
            // =-=-=-=-=-=-=-
            // make the call to the "open" interface
            ret = resc->call( _comm, eirods::RESOURCE_OP_UNREGISTERED, _object );
            if( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to call unregistered interface.";
                result = PASSMSG(msg.str(), ret);
            }
        }
    }
    return result;
} // fileUnregistered

// File modified with the database
eirods::error fileModified(
    rsComm_t* _comm,
    eirods::first_class_object& _object )
{
    eirods::error result = SUCCESS();
    eirods::error ret;

    std::string resc_hier = _object.resc_hier();
    if(!resc_hier.empty()) {
        
        // =-=-=-=-=-=-=-
        // retrieve the resource name given the object
        eirods::resource_ptr resc;
        ret = _object.resolve( resc_mgr, resc ); 
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to resolve resource.";
            result = PASSMSG(msg.str(), ret);
        } else {
        
            // =-=-=-=-=-=-=-
            // make the call to the "open" interface
            ret = resc->call( _comm, eirods::RESOURCE_OP_MODIFIED, _object );
            if( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to call modified interface.";
                result = PASSMSG(msg.str(), ret);
            }
        }
    } else {
        // NOOP okay for struct file objects
    }
    return result;
} // fileModified
