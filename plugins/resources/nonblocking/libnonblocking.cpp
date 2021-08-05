// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "rcConnect.h"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_physical_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "voting.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/function.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// system includes
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>
#endif
#include <dirent.h>

#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif

#if defined(osx_platform)
#include <sys/param.h>
#include <sys/mount.h>
#endif

#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#include <sys/stat.h>

#include <string.h>

#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

// =-=-=-=-=-=-=-
// 1. Define utility functions that the operations might need

// =-=-=-=-=-=-=-
// NOTE: All storage resources must do this on the physical path stored in the file object and then update
//       the file object's physical path with the full path

// =-=-=-=-=-=-=-
/// @brief Generates a full path name from the partial physical path and the specified resource's vault path
irods::error non_blocking_generate_full_path(
    irods::plugin_property_map& _prop_map,
    const std::string&           _phy_path,
    std::string&                 _ret_string ) {
    irods::error result = SUCCESS();
    irods::error ret;
    std::string vault_path;
    // TODO - getting vault path by property will not likely work for coordinating nodes
    ret = _prop_map.get<std::string>( irods::RESOURCE_PATH, vault_path );
    if ( ( result = ASSERT_ERROR( ret.ok(), SYS_INVALID_INPUT_PARAM, "resource has no vault path." ) ).ok() ) {
        if ( _phy_path.compare( 0, 1, "/" ) != 0 &&
                _phy_path.compare( 0, vault_path.size(), vault_path ) != 0 ) {
            _ret_string  = vault_path;
            _ret_string += "/";
            _ret_string += _phy_path;
        }
        else {
            // The physical path already contains the vault path
            _ret_string = _phy_path;
        }
    }

    return result;

} // non_blocking_generate_full_path

// =-=-=-=-=-=-=-
/// @brief update the physical path in the file object
irods::error non_blocking_check_path(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // try dynamic cast on ptr, throw error otherwise
    irods::data_object_ptr data_obj = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );
    if ( ( result = ASSERT_ERROR( data_obj.get(), SYS_INVALID_INPUT_PARAM, "Failed to cast fco to data_object." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // NOTE: Must do this for all storage resources
        std::string full_path;
        irods::error ret = non_blocking_generate_full_path( _ctx.prop_map(),
                           data_obj->physical_path(),
                           full_path );
        if ( ( result = ASSERT_PASS( ret, "Failed generating full path for object." ) ).ok() ) {
            data_obj->physical_path( full_path );
        }
    }

    return result;

} // non_blocking_check_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
template< typename DEST_TYPE >
irods::error non_blocking_check_params_and_path(
    irods::plugin_context& _ctx ) {

    irods::error result = SUCCESS();
    irods::error ret;

    // =-=-=-=-=-=-=-
    // verify that the resc context is valid
    ret = _ctx.valid< DEST_TYPE >();
    if ( ( result = ASSERT_PASS( ret, "resource context is invalid." ) ).ok() ) {
        result = non_blocking_check_path( _ctx );
    }

    return result;

} // non_blocking_check_params_and_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
irods::error non_blocking_check_params_and_path(
    irods::plugin_context& _ctx ) {

    irods::error result = SUCCESS();
    irods::error ret;

    // =-=-=-=-=-=-=-
    // verify that the resc context is valid
    ret = _ctx.valid();
    if ( ( result = ASSERT_PASS( ret, "non_blocking_check_params_and_path - resource context is invalid" ) ).ok() ) {
        result = non_blocking_check_path( _ctx );
    }

    return result;

} // non_blocking_check_params_and_path

// =-=-=-=-=-=-=-
//@brief Recursively make all of the dirs in the path
irods::error non_blocking_file_mkdir_r(
    const std::string&      path,
    mode_t mode ) {
    irods::error result = SUCCESS();
    std::string subdir;
    std::size_t pos = 0;
    bool done = false;
    while ( !done && result.ok() ) {
        pos = path.find_first_of( '/', pos + 1 );
        if ( pos > 0 ) {
            subdir = path.substr( 0, pos );
            int status = mkdir( subdir.c_str(), mode );

            // =-=-=-=-=-=-=-
            // handle error cases
            result = ASSERT_ERROR( status >= 0 || errno == EEXIST, UNIX_FILE_RENAME_ERR - errno, "mkdir error for \"%s\", errno = \"%s\", status = %d.",
                                   subdir.c_str(), strerror( errno ), status );
        }
        if ( pos == std::string::npos ) {
            done = true;
        }
    }

    return result;

} // non_blocking_file_mkdir_r

// =-=-=-=-=-=-=-
// 2. Define operations which will be called by the file*
//    calls declared in server/driver/include/fileDriver.h
// =-=-=-=-=-=-=-

// =-=-=-=-=-=-=-
// NOTE :: to access properties in the _prop_map do the
//      :: following :
//      :: double my_var = 0.0;
//      :: irods::error ret = _prop_map.get< double >( "my_key", my_var );
// =-=-=-=-=-=-=-

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file registration
irods::error non_blocking_file_registered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    result = ASSERT_PASS( ret, "Invalid parameters or physical path." );

    // NOOP
    return result;
}

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file unregistration
irods::error non_blocking_file_unregistered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    result = ASSERT_PASS( ret, "Invalid parameters or physical path." );

    // NOOP
    return result;
}

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file modification
irods::error non_blocking_file_modified(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    result = ASSERT_PASS( ret, "Invalid parameters or physical path." );

    // NOOP
    return result;
}

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file operation
irods::error non_blocking_file_notify(
    irods::plugin_context& _ctx,
    const std::string* ) {
    irods::error result = SUCCESS();
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    result = ASSERT_PASS( ret, "Invalid parameters or physical path." );

    // NOOP
    return result;
}

// =-=-=-=-=-=-=-
// interface to determine free space on a device given a path
irods::error non_blocking_file_getfs_freespace(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the hierarchy to the desired object
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        size_t found = fco->physical_path().find_last_of( "/" );
        std::string path = fco->physical_path().substr( 0, found + 1 );
        int status = -1;
        rodsLong_t fssize = USER_NO_SUPPORT_ERR;
#if defined(solaris_platform)
        struct statvfs statbuf;
#else
        struct statfs statbuf;
#endif

#if defined(solaris_platform) || defined(sgi_platform)   ||     \
defined(aix_platform)     || defined(linux_platform) ||     \
defined(osx_platform)
#if defined(solaris_platform)
        status = statvfs( path.c_str(), &statbuf );
#else
#if defined(sgi_platform)
        status = statfs( path.c_str(), &statbuf, sizeof( struct statfs ), 0 );
#else
        status = statfs( path.c_str(), &statbuf );
#endif
#endif

        // =-=-=-=-=-=-=-
        // handle error, if any
        int err_status = UNIX_FILE_GET_FS_FREESPACE_ERR - errno;
        if ( ( result = ASSERT_ERROR( status >= 0, err_status, "Statfs error for \"%s\", status = %d.",
                                      path.c_str(), err_status ) ).ok() ) {

#if defined(sgi_platform)
            if ( statbuf.f_frsize > 0 ) {
                fssize = statbuf.f_frsize;
            }
            else {
                fssize = statbuf.f_bsize;
            }
            fssize *= statbuf.f_bavail;
#endif

#if defined(aix_platform) || defined(osx_platform) ||   \
defined(linux_platform)
            fssize = statbuf.f_bavail * statbuf.f_bsize;
#endif

#if defined(sgi_platform)
            fssize = statbuf.f_bfree * statbuf.f_bsize;
#endif
            result.code( fssize );
        }

#endif /* solaris_platform, sgi_platform .... */
    }

    return result;

} // non_blocking_file_get_fsfreespace

// =-=-=-=-=-=-=-
// interface for POSIX create
irods::error non_blocking_file_create(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        ret = non_blocking_file_getfs_freespace( _ctx );
        if ( ( result = ASSERT_PASS( ret, "Error determining freespace on system." ) ).ok() ) {
            rodsLong_t file_size = fco->size();
            if ( ( result = ASSERT_ERROR( file_size < 0 || ret.code() >= file_size, USER_FILE_TOO_LARGE, "File size: %ld is greater than space left on device: %ld",
                                          file_size, ret.code() ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // make call to umask & open for create
                mode_t myMask = umask( ( mode_t ) 0000 );
                int    fd     = open( fco->physical_path().c_str(), O_RDWR | O_CREAT | O_EXCL, fco->mode() );
                int errsav = errno;

                // =-=-=-=-=-=-=-
                // reset the old mask
                ( void ) umask( ( mode_t ) myMask );

                // =-=-=-=-=-=-=-
                // if we got a 0 descriptor, try again
                if ( fd == 0 ) {

                    close( fd );
                    int null_fd = open( "/dev/null", O_RDWR, 0 );
                    fd = open( fco->physical_path().c_str(), O_RDWR | O_CREAT | O_EXCL, fco->mode() );
                    errsav = errno;
                    if ( null_fd >= 0 ) {
                        close( null_fd );
                    }
                    rodsLog( LOG_NOTICE, "non_blocking_file_create: 0 descriptor" );
                }

                // =-=-=-=-=-=-=-
                // trap error case with bad fd
                if ( fd < 0 ) {
                    int status = UNIX_FILE_CREATE_ERR - errsav;
                    std::stringstream msg;
                    msg << "create error for \"";
                    msg << fco->physical_path();
                    msg << "\", errno = \"";
                    msg << strerror( errsav );
                    msg << "\".";
                    // =-=-=-=-=-=-=-
                    // WARNING :: Major Assumptions are made upstream and use the FD also as a
                    //         :: Status, if this is not done EVERYTHING BREAKS!!!!111one
                    fco->file_descriptor( status );
                    result = ERROR( status, msg.str() );
                }
                else {
                    // =-=-=-=-=-=-=-
                    // cache file descriptor in out-variable
                    fco->file_descriptor( fd );
                    result.code( fd );
                }
            }
        }
    }
    // =-=-=-=-=-=-=-
    // declare victory!
    return result;

} // non_blocking_file_create

// =-=-=-=-=-=-=-
// interface for POSIX Open
irods::error non_blocking_file_open(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // handle OSX weirdness...
        int flags = fco->flags();

#if defined(osx_platform)
        // For osx, O_TRUNC = 0x0400, O_TRUNC = 0x200 for other system
        if ( flags & 0x200 ) {
            flags = flags ^ 0x200;
            flags = flags | O_TRUNC;
        }
#endif
        // =-=-=-=-=-=-=-
        // make call to open
        errno = 0;
        int fd = open( fco->physical_path().c_str(), flags, fco->mode() );
        int errsav = errno;

        // =-=-=-=-=-=-=-
        // if we got a 0 descriptor, try again
        if ( fd == 0 ) {
            close( fd );
            int null_fd = open( "/dev/null", O_RDWR, 0 );
            fd = open( fco->physical_path().c_str(), flags, fco->mode() );
            errsav = errno;
            if ( null_fd >= 0 ) {
                close( null_fd );
            }
            rodsLog( LOG_NOTICE, "non_blocking_file_open: 0 descriptor" );
        }

        // =-=-=-=-=-=-=-
        // cache status in the file object
        fco->file_descriptor( fd );

        // =-=-=-=-=-=-=-
        // did we still get an error?
        int status = UNIX_FILE_OPEN_ERR - errsav;
        if ( fd < 0 ) {
            std::stringstream msg;
            msg << "Open error for \"";
            msg << fco->physical_path();
            msg << "\", errno = \"";
            msg << strerror( errsav );
            msg << "\", status = ";
            msg << status;
            msg << ", flags = ";
            msg << flags;
            msg << ".";
            result = ERROR( status, msg.str() );
        }
        else {
            result.code( fd );
        }
    }

    // =-=-=-=-=-=-=-
    // declare victory!
    return result;

} // non_blocking_file_open

// =-=-=-=-=-=-=-
// interface for POSIX Read
irods::error non_blocking_file_read(
    irods::plugin_context& _ctx,
    void*                  _buf,
    const int              _len ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        int status;
        struct timeval tv;
        int nbytes;
        int toRead;
        char *tmpPtr;
        fd_set set;
        int fd = fco->file_descriptor();

        bzero( &tv, sizeof( tv ) );
        tv.tv_sec = NB_READ_TOUT_SEC;

        /* Initialize the file descriptor set. */
        FD_ZERO( &set );
        FD_SET( fd, &set );

        toRead = _len;
        tmpPtr = ( char * ) _buf;

        while ( toRead > 0 ) {
#ifndef _WIN32
            status = select( fd + 1, &set, NULL, NULL, &tv );
            if ( status == 0 ) {
                /* timedout */
                if ( _len - toRead > 0 ) {
                    return CODE( _len - toRead );
                }
                else {
                    std::stringstream msg;
                    msg << "timeout error.";
                    return ERROR( UNIX_FILE_OPR_TIMEOUT_ERR - errno, msg.str() );
                }
            }
            else if ( status < 0 ) {
                if ( errno == EINTR ) {
                    errno = 0;
                    continue;
                }
                else {
                    std::stringstream msg;
                    msg << "file read error.";
                    return ERROR( UNIX_FILE_READ_ERR - errno, msg.str() );
                }
            }
#endif

            nbytes = read( fco->file_descriptor(), ( void * ) tmpPtr, toRead );
            if ( nbytes < 0 ) {
                if ( errno == EINTR ) {
                    /* interrupted */
                    errno = 0;
                    nbytes = 0;
                }
                else if ( toRead == _len ) {
                    return ERROR( UNIX_FILE_READ_ERR - errno, "file read error" );
                }
                else {
                    break;
                }
            }
            else if ( nbytes == 0 ) {
                break;
            }

            toRead -= nbytes;
            tmpPtr += nbytes;

        } // while

        result.code( _len - toRead );
    }

    // =-=-=-=-=-=-=-
    // win!
    return result;

} // non_blocking_file_read

// =-=-=-=-=-=-=-
// interface for POSIX Write
irods::error non_blocking_file_write(
    irods::plugin_context& _ctx,
    const void*            _buf,
    const int              _len ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {
        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        int fd = fco->file_descriptor();
        int nbytes   = 0;
        int toWrite  = 0;
        int status   = 0;
        char* tmpPtr = 0;

        struct timeval tv;
        bzero( &tv, sizeof( tv ) );
        tv.tv_sec = NB_WRITE_TOUT_SEC;

        /* Initialize the file descriptor set. */
        fd_set set;
        FD_ZERO( &set );
        FD_SET( fd, &set );

        toWrite = _len;
        tmpPtr = ( char * ) _buf;

        while ( toWrite > 0 ) {
#ifndef _WIN32
            status = select( fd + 1, NULL, &set, NULL, &tv );
            if ( status == 0 ) {
                /* timedout */
                return ERROR( UNIX_FILE_OPR_TIMEOUT_ERR - errno, "time out error" );;
            }
            else if ( status < 0 ) {
                if ( errno == EINTR ) {
                    errno = 0;
                    continue;
                }
                else {
                    return ERROR( UNIX_FILE_WRITE_ERR - errno, "file write error" );
                }
            }
#endif

            nbytes = write( fco->file_descriptor(), ( void * ) tmpPtr, _len );
            if ( nbytes < 0 ) {
                if ( errno == EINTR ) {
                    /* interrupted */
                    errno = 0;
                    nbytes = 0;
                }
                else  {
                    return ERROR( UNIX_FILE_WRITE_ERR - errno, "file write error" );
                }
            }

            toWrite -= nbytes;
            tmpPtr  += nbytes;

        } // while

        result.code( _len );

    }

    // =-=-=-=-=-=-=-
    // win!
    return result;

} // non_blocking_file_write

// =-=-=-=-=-=-=-
// interface for POSIX Close
irods::error non_blocking_file_close(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to close
        int status = close( fco->file_descriptor() );

        // =-=-=-=-=-=-=-
        // log any error
        int err_status = UNIX_FILE_CLOSE_ERR - errno;
        if ( !( result = ASSERT_ERROR( status >= 0, err_status, "Close error for file: \"%s\", errno = \"%s\", status = %d.",
                                       fco->physical_path().c_str(), strerror( errno ), err_status ) ).ok() ) {
            result.code( err_status );
        }
        else {
            result.code( status );
        }
    }

    return result;

} // non_blocking_file_close

// =-=-=-=-=-=-=-
// interface for POSIX Unlink
irods::error non_blocking_file_unlink(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::data_object_ptr fco = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to unlink
        int status = unlink( fco->physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // error handling
        int err_status = UNIX_FILE_UNLINK_ERR - errno;
        if ( !( result = ASSERT_ERROR( status >= 0, err_status, "Unlink error for \"%s\", errno = \"%s\", status = %d.",
                                       fco->physical_path().c_str(), strerror( errno ), err_status ) ).ok() ) {

            result.code( err_status );
        }
        else {
            result.code( status );
        }
    }

    return result;

} // non_blocking_file_unlink

// =-=-=-=-=-=-=-
// interface for POSIX Stat
irods::error non_blocking_file_stat(
    irods::plugin_context& _ctx,
    struct stat*                        _statbuf ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // NOTE:: this function assumes the object's physical path is
    //        correct and should not have the vault path
    //        prepended - hcj

    irods::error ret = _ctx.valid();
    if ( ( result = ASSERT_PASS( ret, "resource context is invalid." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::data_object_ptr fco = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to stat
        int status = stat( fco->physical_path().c_str(), _statbuf );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        int err_status = UNIX_FILE_STAT_ERR - errno;
        if ( ( result = ASSERT_ERROR( status >= 0, err_status, "Stat error for \"%s\", errno = \"%s\", status = %d.",
                                      fco->physical_path().c_str(), strerror( errno ), err_status ) ).ok() ) {
            result.code( status );
        }
    }

    return result;

} // non_blocking_file_stat

// =-=-=-=-=-=-=-
// interface for POSIX lseek
irods::error non_blocking_file_lseek(
    irods::plugin_context& _ctx,
    const long long                           _offset,
    const int                                 _whence ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // get ref to fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to lseek
        long long status = lseek( fco->file_descriptor(),  _offset, _whence );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        long long err_status = UNIX_FILE_LSEEK_ERR - errno;
        if ( ( result = ASSERT_ERROR( status >= 0, err_status, "Lseek error for \"%s\", errno = \"%s\", status = %ld.",
                                      fco->physical_path().c_str(), strerror( errno ), err_status ) ).ok() ) {
            result.code( status );
        }
    }

    return result;

} // non_blocking_file_lseek

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error non_blocking_file_mkdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // NOTE :: this function assumes the object's physical path is correct and
    //         should not have the vault path prepended - hcj

    irods::error ret = _ctx.valid< irods::collection_object >();
    if ( ( result = ASSERT_PASS( ret, "resource context is invalid." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to mkdir & umask
        mode_t myMask = umask( ( mode_t ) 0000 );
        int    status = mkdir( fco->physical_path().c_str(), fco->mode() );

        // =-=-=-=-=-=-=-
        // reset the old mask
        umask( ( mode_t ) myMask );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        result.code( status );
        int err_status = UNIX_FILE_MKDIR_ERR - errno;
        if ( ( result = ASSERT_ERROR( status >= 0, err_status, "Mkdir error for \"%s\", errno = \"%s\", status = %d.",
                                      fco->physical_path().c_str(), strerror( errno ), err_status ) ).ok() ) {
            result.code( status );
        }
    }
    return result;

} // non_blocking_file_mkdir

// =-=-=-=-=-=-=-
// interface for POSIX rmdir
irods::error non_blocking_file_rmdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to chmod
        int status = rmdir( fco->physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        int err_status = UNIX_FILE_RMDIR_ERR - errno;
        result = ASSERT_ERROR( status >= 0, err_status, "Rmdir error for \"%s\", errno = \"%s\", status = %d.",
                               fco->physical_path().c_str(), strerror( errno ), err_status );
    }

    return result;

} // non_blocking_file_rmdir

// =-=-=-=-=-=-=-
// interface for POSIX opendir
irods::error non_blocking_file_opendir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path< irods::collection_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to opendir
        DIR* dir_ptr = opendir( fco->physical_path().c_str() );

        // =-=-=-=-=-=-=-
        // cache status in out variable
        int err_status = UNIX_FILE_OPENDIR_ERR - errno;

        // =-=-=-=-=-=-=-
        // return an error if necessary
        if ( NULL != dir_ptr ) {
            // =-=-=-=-=-=-=-
            // cache dir_ptr in out variables
            fco->directory_pointer( dir_ptr );
        }
        else {
            std::stringstream msg;
            msg << "Opendir error for \"";
            msg << fco->physical_path();
            msg << "\", errno = \"";
            msg << strerror( errno );
            msg << "\", status = ";
            msg << err_status;
            msg << ".";
            result = ERROR( err_status, msg.str() );
        }
    }

    return result;

} // non_blocking_file_opendir

// =-=-=-=-=-=-=-
// interface for POSIX closedir
irods::error non_blocking_file_closedir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path< irods::collection_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // make the call to opendir
        int status = closedir( fco->directory_pointer() );

        // =-=-=-=-=-=-=-
        // return an error if necessary
        int err_status = UNIX_FILE_CLOSEDIR_ERR - errno;
        result = ASSERT_ERROR( status >= 0, err_status, "Closedir error for \"%s\", errno = \"%s\", status = %d.",
                               fco->physical_path().c_str(), strerror( errno ), err_status );
    }

    return result;

} // non_blocking_file_closedir

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error non_blocking_file_readdir(
    irods::plugin_context& _ctx,
    struct rodsDirent**                 _dirent_ptr ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path< irods::collection_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // zero out errno?
        errno = 0;

        // =-=-=-=-=-=-=-
        // make the call to readdir
        struct dirent * tmp_dirent = readdir( fco->directory_pointer() );

        // =-=-=-=-=-=-=-
        // handle error cases
        if ( ( result = ASSERT_ERROR( tmp_dirent != NULL, -1, "End of directory list reached." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // alloc dirent as necessary
            if ( !( *_dirent_ptr ) ) {
                ( *_dirent_ptr ) = ( rodsDirent_t* )malloc( sizeof( rodsDirent_t ) );
            }

            // =-=-=-=-=-=-=-
            // convert standard dirent to rods dirent struct
            int status = direntToRodsDirent( ( *_dirent_ptr ), tmp_dirent );
            if ( status < 0 ) {
                irods::log( ERROR( status, "direntToRodsDirent failed." ) );
            }

#if defined(solaris_platform)
            rstrcpy( ( *_dirent_ptr )->d_name, tmp_dirent->d_name, MAX_NAME_LEN );
#endif
        }
        else {
            // =-=-=-=-=-=-=-
            // cache status in out variable
            int status = UNIX_FILE_READDIR_ERR - errno;
            if ( ( result = ASSERT_ERROR( errno == 0, status, "Readdir error, status = %d, errno= \"%s\".",
                                          status, strerror( errno ) ) ).ok() ) {
                result.code( -1 );
            }
        }
    }

    return result;

} // non_blocking_file_readdir

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error non_blocking_file_rename(
    irods::plugin_context& _ctx,
    const char*                         _new_file_name ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // manufacture a new path from the new file name
        std::string new_full_path;
        ret = non_blocking_generate_full_path( _ctx.prop_map(), _new_file_name, new_full_path );
        if ( ( result = ASSERT_PASS( ret, "Unable to generate full path for destination file: \"%s\".",
                                     _new_file_name ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // cast down the hierarchy to the desired object
            irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // make the directories in the path to the new file
            std::string new_path = new_full_path;
            std::size_t last_slash = new_path.find_last_of( '/' );
            new_path.erase( last_slash );
            ret = non_blocking_file_mkdir_r( new_path.c_str(), 0750 );
            if ( ( result = ASSERT_PASS( ret, "Mkdir error for \"%s\".", new_path.c_str() ) ).ok() ) {

            }

            // =-=-=-=-=-=-=-
            // make the call to rename
            int status = rename( fco->physical_path().c_str(), new_full_path.c_str() );

            // =-=-=-=-=-=-=-
            // handle error cases
            if ( status < 0 ) {
                status = UNIX_FILE_RENAME_ERR - errno;

                std::stringstream msg;
                msg << "non_blocking_file_rename: rename error for ";
                msg <<  fco->physical_path();
                msg << " to ";
                msg << new_full_path;
                msg << ", errno = ";
                msg << strerror( errno );
                msg << ", status = ";
                msg << status;

                return ERROR( status, msg.str() );
            }
        }
    }

    return result;

} // non_blocking_file_rename

// =-=-=-=-=-=-=-
// interface for POSIX truncate
irods::error non_blocking_file_truncate(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - Invalid parameters or physical path.";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // cast down the chain to our understood object type
    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // =-=-=-=-=-=-=-
    // make the call to rename
    int status = truncate( file_obj->physical_path().c_str(),
                           file_obj->size() );

    // =-=-=-=-=-=-=-
    // handle any error cases
    if ( status < 0 ) {
        // =-=-=-=-=-=-=-
        // cache status in out variable
        status = UNIX_FILE_TRUNCATE_ERR - errno;

        std::stringstream msg;
        msg << "non_blocking_file_truncate: rename error for ";
        msg << file_obj->physical_path();
        msg << ", errno = '";
        msg << strerror( errno );
        msg << "', status = ";
        msg << status;

        return ERROR( status, msg.str() );
    }

    return CODE( status );

} // non_blocking_file_truncate


irods::error
non_blockingFileCopyPlugin( int         mode,
                            const char* srcFileName,
                            const char* destFileName ) {
    irods::error result = SUCCESS();

    int inFd = open( srcFileName, O_RDONLY, 0 );
    int open_err_status = UNIX_FILE_OPEN_ERR - errno;
    struct stat statbuf;
    int status = stat( srcFileName, &statbuf );
    int stat_err_status = UNIX_FILE_STAT_ERR - errno;
    if ( status < 0 ) {
        if ( inFd >= 0 ) {
            close( inFd );
        }
        std::stringstream msg_stream;
        msg_stream << "Stat of \"" << srcFileName << "\" error, status = " << stat_err_status;
        result = ERROR( stat_err_status, msg_stream.str() );
    }
    else if ( inFd < 0 ) {
        std::stringstream msg_stream;
        msg_stream << "Open error for srcFileName \"" << srcFileName << "\", status = " << status;
        result = ERROR( open_err_status, msg_stream.str() );
    }
    else if ( ( statbuf.st_mode & S_IFREG ) == 0 ) {
        close( inFd ); // JMC cppcheck - resource
        std::stringstream msg_stream;
        msg_stream << "srcFileName \"" << srcFileName << "\" is not a regular file.";
        result = ERROR( UNIX_FILE_STAT_ERR, msg_stream.str() );
    }
    else {
        int outFd = open( destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode );
        int err_status = UNIX_FILE_OPEN_ERR - errno;
        if ( outFd < 0 ) {
            std::stringstream msg_stream;
            msg_stream << "Open error for destFileName \"" << destFileName << "\", status = " << status;
            result = ERROR( err_status, msg_stream.str() );
        }
        else if ( ( statbuf.st_mode & S_IFREG ) == 0 ) {
            close( outFd ); // JMC cppcheck - resource
            std::stringstream msg_stream;
            msg_stream << "destFileName \"" << destFileName << "\" is not a regular file.";
            result = ERROR( UNIX_FILE_STAT_ERR, msg_stream.str() );
        }
        else {
            size_t trans_buff_size;
            try {
                trans_buff_size = irods::get_advanced_setting<const int>(irods::CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS) * 1024 * 1024;
            } catch ( const irods::exception& e ) {
                close( outFd );
                close( inFd );
                return irods::error(e);
            }

            std::vector<char> myBuf( trans_buff_size );

            int bytesRead;
            rodsLong_t bytesCopied = 0;
            while ( result.ok() && ( bytesRead = read( inFd, ( void * ) myBuf.data(), trans_buff_size ) ) > 0 ) {
                int bytesWritten = write( outFd, ( void * ) myBuf.data(), bytesRead );
                err_status = UNIX_FILE_WRITE_ERR - errno;
                if ( ( result = ASSERT_ERROR( bytesWritten > 0, err_status, "Write error for srcFileName %s, status = %d",
                                              destFileName, status ) ).ok() ) {
                    bytesCopied += bytesWritten;
                }
            }

            close( outFd );

            if ( result.ok() ) {
                result = ASSERT_ERROR( bytesCopied == statbuf.st_size, SYS_COPY_LEN_ERR, "Copied size %lld does not match source size %lld of %s",
                                       bytesCopied, statbuf.st_size, srcFileName );
            }
        }
        close( inFd );
    }
    return result;
}

// =-=-=-=-=-=-=-
// non_blockingStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from filename to cacheFilename. optionalInfo info
// is not used.
irods::error non_blocking_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char*                      _cache_file_name ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the hierarchy to the desired object
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        ret = non_blockingFileCopyPlugin( fco->mode(), fco->physical_path().c_str(), _cache_file_name );
        result = ASSERT_PASS( ret, "Failed" );
    }
    return result;
} // non_blocking_file_stage_to_cache

// =-=-=-=-=-=-=-
// non_blockingSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from cacheFilename to filename. optionalInfo info
// is not used.
irods::error non_blocking_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char*                     _cache_file_name ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = non_blocking_check_params_and_path( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Invalid parameters or physical path." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // cast down the hierarchy to the desired object
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        ret = non_blockingFileCopyPlugin( fco->mode(), _cache_file_name, fco->physical_path().c_str() );
        result = ASSERT_PASS( ret, "Failed" );
    }

    return result;

} // non_blocking_file_sync_to_arch

// =-=-=-=-=-=-=-
// used to allow the resource to determine which host
// should provide the requested operation
irods::error non_blocking_file_resolve_hierarchy(
    irods::plugin_context&   _ctx,
    const std::string*       _opr,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote)
{
    namespace irv = irods::experimental::resource::voting;

    if (irods::error ret = _ctx.valid<irods::file_object>(); !ret.ok()) {
        return PASSMSG("Invalid resource context.", ret);
    }

    if (!_opr || !_curr_host || !_out_parser || !_out_vote) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Invalid input parameter.");
    }

    _out_parser->add_child(irods::get_resource_name(_ctx));
    *_out_vote = irv::vote::zero;
    try {
        *_out_vote = irv::calculate(*_opr, _ctx, *_curr_host, *_out_parser);
        return SUCCESS();
    }
    catch(const std::out_of_range& e) {
        return ERROR(INVALID_OPERATION, e.what());
    }
    catch (const irods::exception& e) {
        return irods::error(e);
    }
    return ERROR(SYS_UNKNOWN_ERROR, "An unknown error occurred while resolving hierarchy.");
} // non_blocking_file_resolve_hierarchy

// =-=-=-=-=-=-=-
// non_blocking_file_rebalance - code which would rebalance the subtree
irods::error non_blocking_file_rebalance(
    irods::plugin_context& _ctx ) {
    return SUCCESS();

} // non_blocking_file_rebalance

// =-=-=-=-=-=-=-
// 3. create derived class to handle non_blocking file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class non_blocking_resource : public irods::resource {
        // =-=-=-=-=-=-=-
        // 3a. create a class to provide maintenance operations, this is only for example
        //     and will not be called.
        class maintenance_operation {
            public:
                maintenance_operation( const std::string& _n ) : name_( _n ) {
                }

                maintenance_operation( const maintenance_operation& _rhs ) {
                    name_ = _rhs.name_;
                }

                maintenance_operation& operator=( const maintenance_operation& _rhs ) {
                    name_ = _rhs.name_;
                    return *this;
                }

                irods::error operator()( rcComm_t* ) {
                    rodsLog( LOG_NOTICE, "non_blocking_resource::post_disconnect_maintenance_operation - [%s]",
                             name_.c_str() );
                    return SUCCESS();
                }

            private:
                std::string name_;

        }; // class maintenance_operation

    public:
        non_blocking_resource(
            const std::string& _inst_name,
            const std::string& _context ) :
            irods::resource(
                _inst_name,
                _context ) {
        } // ctor


        irods::error need_post_disconnect_maintenance_operation( bool& _b ) {
            _b = false;
            return SUCCESS();
        }


        // =-=-=-=-=-=-=-
        // 3b. pass along a functor for maintenance work after
        //     the client disconnects, uncomment the first two lines for effect.
        irods::error post_disconnect_maintenance_operation( irods::pdmo_type& ) {
            irods::error result = SUCCESS();
            return ERROR( -1, "nop" );
        }
}; // class non_blocking_resource

// =-=-=-=-=-=-=-
// 4. create the plugin factory function which will return a dynamically
//    instantiated object of the previously defined derived resource.  use
//    the add_operation member to associate a 'call name' to the interfaces
//    defined above.  for resource plugins these call names are standardized
//    as used by the irods facing interface defined in
//    server/drivers/src/fileDriver.c
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

    // =-=-=-=-=-=-=-
    // 4a. create non_blocking_resource
    non_blocking_resource* resc = new non_blocking_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    //

    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            non_blocking_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            non_blocking_file_open ) );

    resc->add_operation(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,const int)>(
                non_blocking_file_read ) );

    resc->add_operation(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,const void*,const int)>(
            non_blocking_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            non_blocking_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            non_blocking_file_unlink ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            non_blocking_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            non_blocking_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            non_blocking_file_opendir ) );

    resc->add_operation(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            non_blocking_file_readdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            non_blocking_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            non_blocking_file_getfs_freespace ) );

    resc->add_operation(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, const long long, const int)>(
            non_blocking_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            non_blocking_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            non_blocking_file_closedir ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            non_blocking_file_stage_to_cache ) );

    resc->add_operation(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            non_blocking_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            non_blocking_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            non_blocking_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            non_blocking_file_modified ) );

    resc->add_operation(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            non_blocking_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            non_blocking_file_truncate ) );

    resc->add_operation(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            non_blocking_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            non_blocking_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory

