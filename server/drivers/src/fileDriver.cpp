/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileDriver.c - The general driver for all file types. */

#include "fileDriver.hpp"
#include "irods_log.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_data_object.hpp"
#include "irods_stacktrace.hpp"

#include "irods_resource_constants.hpp"
#include "irods_resource_manager.hpp"

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX create
irods::error fileCreate(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        irods::log( ret_err );
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "create" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_CREATE, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'create'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }


} // fileCreate

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX open
irods::error fileOpen(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "open" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_OPEN, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'open'", ret_err );

    }
    else {
        return CODE( ret_err.code() );

    }

} // fileOpen

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX read
irods::error fileRead(
    rsComm_t*                     _comm,
    irods::first_class_object_ptr _object,
    void*                         _buf,
    const int                     _len ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call< void*, const int >( _comm, irods::RESOURCE_OP_READ, _object, _buf, _len );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'read'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileRead

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX write
irods::error fileWrite(
    rsComm_t*                     _comm,
    irods::first_class_object_ptr _object,
    const void*                   _buf,
    const int                     _len ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "write" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call< const void*, const int >( _comm, irods::RESOURCE_OP_WRITE, _object, _buf, _len );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'write'", ret_err );
    }
    else {
        std::stringstream msg;
        msg << "Write successful.";
        return PASSMSG( msg.str(), ret_err );
    }

} // fileWrite

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX close
irods::error fileClose(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "close" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_CLOSE, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'close'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileClose

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX unlink
irods::error fileUnlink(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "unlink" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_UNLINK, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'unlink'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileUnlink

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX stat
irods::error fileStat(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object,
    struct stat*                   _statbuf ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "stat" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call< struct stat* >( _comm, irods::RESOURCE_OP_STAT, _object, _statbuf );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'stat'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileStat

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX lseek
irods::error fileLseek(
    rsComm_t*                     _comm,
    irods::first_class_object_ptr _object,
    const long long               _offset,
    const int                     _whence ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "lseek" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call< const long long, const int >( _comm, irods::RESOURCE_OP_LSEEK, _object, _offset, _whence );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'lseek'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileLseek

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX mkdir
irods::error fileMkdir(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "mkdir" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_MKDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'mkdir'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileMkdir


// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX chmod
irods::error fileChmod(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object,
    int                            _mode ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "chmod" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_CHMOD, _object, _mode );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'chmod'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileChmod

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX rmdir
irods::error fileRmdir(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "rmdir" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_RMDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'rmdir'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileRmdir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX opendir
irods::error fileOpendir(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "opendir" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_OPENDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'opendir'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileOpendir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX closedir
irods::error fileClosedir(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "closedir" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_CLOSEDIR, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'closedir'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileClosedir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX readdir
irods::error fileReaddir(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object,
    struct rodsDirent**            _dirent_ptr ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "readdir" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call< struct rodsDirent** >( _comm, irods::RESOURCE_OP_READDIR, _object, _dirent_ptr );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'readdir'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileReaddir

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin POSIX rename
irods::error fileRename(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object,
    const std::string&             _new_file_name ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "rename" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call<  const char* >( _comm, irods::RESOURCE_OP_RENAME,  _object, _new_file_name.c_str() );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'rename'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileRename

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin freespace
irods::error fileGetFsFreeSpace(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "freespace" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_FREESPACE, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'stage'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileGetFsFreeSpace

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin truncate
irods::error fileTruncate(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "truncate" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call( _comm, irods::RESOURCE_OP_TRUNCATE, _object );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'truncate'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // fileTruncate

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin StageToCache
irods::error fileStageToCache(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object,
    const std::string&             _cache_file_name ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if ( _cache_file_name.empty() ) {
        irods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileStageToCache - File Name is Empty." );
        irods::log( ret_err );
        return ret_err;
    }

    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "stagetocache" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call< const char* >( _comm, irods::RESOURCE_OP_STAGETOCACHE, _object, _cache_file_name.c_str() );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'stagetocache'", ret_err );
    }
    else {
        return SUCCESS();
    }

} // fileStageToCache

// =-=-=-=-=-=-=-
// Top Level Interface for Resource Plugin SyncToArch
irods::error fileSyncToArch(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object,
    const std::string&             _cache_file_name ) {
    // =-=-=-=-=-=-=-
    // trap empty file name
    if ( _cache_file_name.empty() ) {
        irods::error ret_err = ERROR( SYS_INVALID_INPUT_PARAM, "fileSyncToArch - File Name is Empty." );
        irods::log( ret_err );
        return ret_err;
    }

    // =-=-=-=-=-=-=-
    // retrieve the resource name given the path
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    irods::error ret_err = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve resource", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "synctoarch" interface
    resc    = boost::dynamic_pointer_cast< irods::resource >( ptr );
    ret_err = resc->call< const char* >( _comm, irods::RESOURCE_OP_SYNCTOARCH, _object, _cache_file_name.c_str() );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'synctoarch'", ret_err );
    }
    else {
        return SUCCESS();
    }

} // fileSyncToArch

// =-=-=-=-=-=-=-
// File registered with the database
irods::error fileRegistered(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    ret = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to resolve resource.";
        result = PASSMSG( msg.str(), ret );

    }
    else {
        // =-=-=-=-=-=-=-
        // make the call to the "registered" interface
        resc = boost::dynamic_pointer_cast< irods::resource >( ptr );
        ret  = resc->call( _comm, irods::RESOURCE_OP_REGISTERED, _object );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to call registered interface.";
            result = PASSMSG( msg.str(), ret );
        }
    }

    return result;
} // fileRegistered

// =-=-=-=-=-=-=-
// File unregistered with the database
irods::error fileUnregistered(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    irods::plugin_ptr   ptr;
    irods::resource_ptr resc;
    ret = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to resolve resource.";
        result = PASSMSG( msg.str(), ret );
    }
    else {

        // =-=-=-=-=-=-=-
        // make the call to the "open" interface
        resc = boost::dynamic_pointer_cast< irods::resource >( ptr );
        ret  = resc->call( _comm, irods::RESOURCE_OP_UNREGISTERED, _object );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to call unregistered interface.";
            result = PASSMSG( msg.str(), ret );
        }
    }

    return result;
} // fileUnregistered

// =-=-=-=-=-=-=-
// File modified with the database
irods::error fileModified(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // =-=-=-=-=-=-=-
    // downcast - this must be called on a descendant of data object
    irods::data_object_ptr data_obj = boost::dynamic_pointer_cast< irods::data_object >( _object );
    std::string resc_hier = data_obj->resc_hier();
    if ( !resc_hier.empty() ) {
        // =-=-=-=-=-=-=-
        // retrieve the resource name given the object
        irods::plugin_ptr   ptr;
        irods::resource_ptr resc;
        ret = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to resolve resource.";
            result = PASSMSG( msg.str(), ret );
        }
        else {

            // =-=-=-=-=-=-=-
            // make the call to the "modified" interface
            resc = boost::dynamic_pointer_cast< irods::resource >( ptr );
            ret  = resc->call( _comm, irods::RESOURCE_OP_MODIFIED, _object );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to call modified interface.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    else {
        // NOOP okay for struct file objects
    }

    return result;

} // fileModified

// =-=-=-=-=-=-=-
// File modified with the database
irods::error fileNotify(
    rsComm_t*                      _comm,
    irods::first_class_object_ptr _object,
    const std::string&             _operation ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // =-=-=-=-=-=-=-
    // downcast - this must be called on a descendant of data object
    irods::data_object_ptr data_obj = boost::dynamic_pointer_cast< irods::data_object >( _object );
    std::string resc_hier = data_obj->resc_hier();
    if ( !resc_hier.empty() ) {
        // =-=-=-=-=-=-=-
        // retrieve the resource name given the object
        irods::plugin_ptr   ptr;
        irods::resource_ptr resc;
        ret = _object->resolve( irods::RESOURCE_INTERFACE, ptr );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "Failed to resolve resource.";
            result = PASSMSG( msg.str(), ret );
        }
        else {

            // =-=-=-=-=-=-=-
            // make the call to the "open" interface
            resc = boost::dynamic_pointer_cast< irods::resource >( ptr );
            ret  = resc->call< const std::string* >(
                       _comm,
                       irods::RESOURCE_OP_NOTIFY,
                       _object,
                       &_operation );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << "Failed to call notify interface.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    else {
        // NOOP okay for struct file objects
    }

    return result;

} // fileNotify
