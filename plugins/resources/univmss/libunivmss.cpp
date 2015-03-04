// =-=-=-=-=-=-=-
// irods includes
#include "msParam.hpp"
#include "rules.hpp"
#include "reGlobalsExtern.hpp"
#include "generalAdmin.hpp"
#include "physPath.hpp"
#include "reIn2p3SysRule.hpp"
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

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>
#include <boost/algorithm/string.hpp>

/// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
inline irods::error univ_mss_check_param(
    irods::resource_plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if ( !ret.ok() ) {
        return PASSMSG( "resource context is invalid", ret );

    }

    return SUCCESS();

} // univ_mss_check_param


extern "C" {

#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

    // =-=-=-=-=-=-=-
    /// @brief token to index the script property
    const std::string SCRIPT_PROP( "script" );

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    irods::error univ_mss_file_create(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error univ_mss_file_open(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    irods::error univ_mss_file_read(
        irods::resource_plugin_context& ,
        void*,
        int ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    irods::error univ_mss_file_write(
        irods::resource_plugin_context&,
        void*,
        int ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    irods::error univ_mss_file_close(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    irods::error univ_mss_file_unlink(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check context
        irods::error err = univ_mss_check_param< irods::data_object >( _ctx );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx.prop_map().get< std::string >( SCRIPT_PROP, script );
        if ( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        irods::data_object_ptr fco = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );
        std::string filename = fco->physical_path();

        execCmd_t execCmdInp;
        memset( &execCmdInp, 0, sizeof( execCmdInp ) );
        snprintf( execCmdInp.cmd, sizeof( execCmdInp.cmd ), "%s", script.c_str() );
        snprintf( execCmdInp.cmdArgv, sizeof( execCmdInp.cmdArgv ), "rm '%s'", filename.c_str() );
        snprintf( execCmdInp.execAddr, sizeof( execCmdInp.execAddr ), "localhost" );

        execCmdOut_t *execCmdOut = NULL;
        int status = _rsExecCmd( &execCmdInp, &execCmdOut );
        freeCmdExecOut( execCmdOut );

        if ( status < 0 ) {
            status = UNIV_MSS_UNLINK_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_unlink - failed for [";
            msg << filename;
            msg << "]";
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // univ_mss_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    irods::error univ_mss_file_stat(
        irods::resource_plugin_context& _ctx,
        struct stat*                     _statbuf ) {
        // =-=-=-=-=-=-=-
        // check context
        irods::error err = univ_mss_check_param< irods::data_object >( _ctx );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx.prop_map().get< std::string >( SCRIPT_PROP, script );
        if ( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        irods::data_object_ptr fco = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );
        std::string filename = fco->physical_path();


        int i, status;
        execCmd_t execCmdInp;
        char cmdArgv[HUGE_NAME_LEN] = "";
        char *outputStr;
        const char *delim1 = ":\n";
        const char *delim2 = "-";
        const char *delim3 = ".";
        execCmdOut_t *execCmdOut = NULL;
        struct tm mytm;
        time_t myTime;

        bzero( &execCmdInp, sizeof( execCmdInp ) );
        rstrcpy( execCmdInp.cmd, script.c_str(), LONG_NAME_LEN );
        snprintf( cmdArgv, sizeof( cmdArgv ), "stat '%s' ", filename.c_str() );
        rstrcpy( execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN );
        rstrcpy( execCmdInp.execAddr, "localhost", LONG_NAME_LEN );
        status = _rsExecCmd( &execCmdInp, &execCmdOut );

        if ( status == 0 && NULL != execCmdOut ) { // JMC cppcheck - nullptr
            if ( execCmdOut->stdoutBuf.buf != NULL ) {
                outputStr = ( char* )execCmdOut->stdoutBuf.buf;
                std::vector<std::string> output_tokens;
                boost::algorithm::split( output_tokens, outputStr, boost::is_any_of( delim1 ) );
                _statbuf->st_dev = atoi( output_tokens[0].c_str() );
                _statbuf->st_ino = atoi( output_tokens[1].c_str() );
                _statbuf->st_mode = atoi( output_tokens[2].c_str() );
                _statbuf->st_nlink = atoi( output_tokens[3].c_str() );
                _statbuf->st_uid = atoi( output_tokens[4].c_str() );
                _statbuf->st_gid = atoi( output_tokens[5].c_str() );
                _statbuf->st_rdev = atoi( output_tokens[6].c_str() );
                _statbuf->st_size = atoll( output_tokens[7].c_str() );
                _statbuf->st_blksize = atoi( output_tokens[8].c_str() );
                _statbuf->st_blocks = atoi( output_tokens[9].c_str() );
                for ( i = 0; i < 3; i++ ) {
                    std::vector<std::string> date_tokens;
                    boost::algorithm::split( date_tokens, output_tokens[10 + i], boost::is_any_of( delim2 ) );
                    mytm.tm_year = atoi( date_tokens[0].c_str() ) - 1900;
                    mytm.tm_mon = atoi( date_tokens[1].c_str() ) - 1;
                    mytm.tm_mday = atoi( date_tokens[2].c_str() );
                    std::vector<std::string> time_tokens;
                    boost::algorithm::split( time_tokens, date_tokens[3], boost::is_any_of( delim3 ) );
                    mytm.tm_hour = atoi( time_tokens[0].c_str() );
                    mytm.tm_min = atoi( time_tokens[1].c_str() );
                    mytm.tm_sec = atoi( time_tokens[2].c_str() );
                    myTime = mktime( &mytm );
                    switch ( i ) {
                    case 0:
                        _statbuf->st_atime = myTime;
                        break;
                    case 1:
                        _statbuf->st_mtime = myTime;
                        break;
                    case 2:
                        _statbuf->st_ctime = myTime;
                        break;
                    }
                }
            }
        }
        else {
            status = UNIV_MSS_STAT_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_stat - failed for [";
            msg << filename;
            msg << "]";
            freeCmdExecOut( execCmdOut );
            return ERROR( status, msg.str() );

        }

        freeCmdExecOut( execCmdOut );
        return CODE( status );

    } // univ_mss_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    irods::error univ_mss_file_lseek(
        irods::resource_plugin_context&,
        long long,
        int ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX chmod
    irods::error univ_mss_file_chmod(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check context
        irods::error err = univ_mss_check_param< irods::data_object >( _ctx );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx.prop_map().get< std::string >( SCRIPT_PROP, script );
        if ( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        irods::data_object_ptr fco = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );
        std::string filename = fco->physical_path();

        int mode = fco->mode();
        int status = 0;
        execCmd_t execCmdInp;

        if ( mode != getDefDirMode() ) {
            mode = getDefFileMode();
        }

        bzero( &execCmdInp, sizeof( execCmdInp ) );
        snprintf( execCmdInp.cmd, sizeof( execCmdInp.cmd ), "%s", script.c_str() );
        snprintf( execCmdInp.cmdArgv, sizeof( execCmdInp.cmdArgv ), "chmod '%s' %o", filename.c_str(), mode );
        snprintf( execCmdInp.execAddr, sizeof( execCmdInp.execAddr ), "%s", "localhost" );
        execCmdOut_t *execCmdOut = NULL;
        status = _rsExecCmd( &execCmdInp, &execCmdOut );
        freeCmdExecOut( execCmdOut );

        if ( status < 0 ) {
            status = UNIV_MSS_CHMOD_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_chmod - failed for [";
            msg << filename;
            msg << "]";
            return ERROR( status, msg.str() );

        }

        return CODE( status );

    } // univ_mss_file_chmod

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    irods::error univ_mss_file_mkdir(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check context
        irods::error err = univ_mss_check_param< irods::collection_object >( _ctx );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx.prop_map().get< std::string >( SCRIPT_PROP, script );
        if ( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        std::string dirname = fco->physical_path();

        int status = 0;
        execCmd_t execCmdInp;

        bzero( &execCmdInp, sizeof( execCmdInp ) );
        snprintf( execCmdInp.cmd, sizeof( execCmdInp.cmd ), "%s", script.c_str() );
        snprintf( execCmdInp.cmdArgv, sizeof( execCmdInp.cmdArgv ), "mkdir '%s'", dirname.c_str() );
        snprintf( execCmdInp.execAddr, sizeof( execCmdInp.execAddr ), "%s", "localhost" );
        execCmdOut_t *execCmdOut = NULL;
        status = _rsExecCmd( &execCmdInp, &execCmdOut );
        freeCmdExecOut( execCmdOut );
        if ( status < 0 ) {
            status = UNIV_MSS_MKDIR_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_mkdir - mkdir failed for [";
            msg << dirname;
            msg << "]";
            return ERROR( status, msg.str() );
        }

        int mode = getDefDirMode();
        fco->mode( mode );
        err = univ_mss_file_chmod( _ctx );

        return err;

    } // univ_mss_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    irods::error univ_mss_file_rmdir(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    irods::error univ_mss_file_opendir(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    irods::error univ_mss_file_closedir(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    irods::error univ_mss_file_readdir(
        irods::resource_plugin_context& ,
        struct rodsDirent** ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    irods::error univ_mss_file_rename(
        irods::resource_plugin_context& _ctx,
        const char*                      _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check context
        irods::error err = univ_mss_check_param< irods::file_object >( _ctx );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx.prop_map().get< std::string >( SCRIPT_PROP, script );
        if ( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        std::string filename = fco->physical_path();

        // =-=-=-=-=-=-=-
        // first create the directory name
        char  dirname[MAX_NAME_LEN] = "";
        const char* lastpart = strrchr( _new_file_name, '/' );
        int   lenDir   = strlen( _new_file_name ) - strlen( lastpart );
        strncpy( dirname, _new_file_name, lenDir );

        // =-=-=-=-=-=-=-
        // create a context to call the mkdir operation
        irods::collection_object_ptr coll_obj(
            new irods::collection_object(
                dirname,
                fco->resc_hier(),
                fco->mode(), 0 ) );
        irods::resource_plugin_context context(
            _ctx.prop_map(),
            coll_obj, "",
            _ctx.comm(),
            _ctx.child_map() );

        // =-=-=-=-=-=-=-
        // create the directory on the MSS
        int status = 0;
        err = univ_mss_file_mkdir( context );

        execCmd_t execCmdInp;

        bzero( &execCmdInp, sizeof( execCmdInp ) );
        snprintf( execCmdInp.cmd, sizeof( execCmdInp.cmd ), "%s", script.c_str() );
        snprintf( execCmdInp.cmdArgv, sizeof( execCmdInp.cmdArgv ), "mv '%s' '%s'", filename.c_str(), _new_file_name );
        snprintf( execCmdInp.execAddr, sizeof( execCmdInp.execAddr ), "%s", "localhost" );
        execCmdOut_t *execCmdOut = NULL;
        status = _rsExecCmd( &execCmdInp, &execCmdOut );
        freeCmdExecOut( execCmdOut );

        if ( status < 0 ) {
            status = UNIV_MSS_RENAME_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_rename - failed for [";
            msg << filename;
            msg << "]";
            return ERROR( status, msg.str() );

        }

        return CODE( status );

    } // univ_mss_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX truncate
    irods::error univ_mss_file_truncate(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_truncate

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    irods::error univ_mss_file_getfs_freespace(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_getfs_freespace

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from filename to cacheFilename. optionalInfo info
    ///        is not used.
    irods::error univ_mss_file_stage_to_cache(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        // =-=-=-=-=-=-=-
        // check context
        irods::error err = univ_mss_check_param< irods::file_object >( _ctx );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        std::string filename = fco->physical_path();

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx.prop_map().get< std::string >( SCRIPT_PROP, script );
        if ( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        int status = 0;

        std::stringstream cmdArgv;
        cmdArgv << "stageToCache '" << filename << "' '" << _cache_file_name << "'";

        execCmd_t execCmdInp;
        bzero( &execCmdInp, sizeof( execCmdInp ) );
        snprintf( execCmdInp.cmd, sizeof( execCmdInp.cmd ), "%s",  script.c_str() );
        snprintf( execCmdInp.cmdArgv, sizeof( execCmdInp.cmdArgv ), "%s", cmdArgv.str().c_str() );
        snprintf( execCmdInp.execAddr, sizeof( execCmdInp.execAddr ), "%s", "localhost" );

        execCmdOut_t *execCmdOut = NULL;
        status = _rsExecCmd( &execCmdInp, &execCmdOut );
        freeCmdExecOut( execCmdOut );

        if ( status < 0 ) {
            status = UNIV_MSS_STAGETOCACHE_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_stage_to_cache: staging from [";
            msg << _cache_file_name;
            msg << "] to [";
            msg << filename;
            msg << "] failed.";
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // univ_mss_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from cacheFilename to filename. optionalInfo info
    ///        is not used.
    irods::error univ_mss_file_sync_to_arch(
        irods::resource_plugin_context& _ctx,
        const char*                         _cache_file_name ) {
        // =-=-=-=-=-=-=-
        // check context
        irods::error err = univ_mss_check_param< irods::file_object >( _ctx );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        std::string filename = fco->physical_path();

        // =-=-=-=-=-=-=-
        // first create the directory name
        char  dirname[MAX_NAME_LEN] = "";
        const char* lastpart = strrchr( filename.c_str(), '/' );
        int   lenDir   = strlen( filename.c_str() ) - strlen( lastpart );
        strncpy( dirname, filename.c_str(), lenDir );

        // =-=-=-=-=-=-=-
        // create a context to call the mkdir operation
        irods::collection_object_ptr coll_obj(
            new irods::collection_object(
                dirname,
                fco->resc_hier(),
                fco->mode(), 0 ) );
        irods::resource_plugin_context context(
            _ctx.prop_map(),
            coll_obj, "",
            _ctx.comm(),
            _ctx.child_map() );

        // =-=-=-=-=-=-=-
        // create the directory on the MSS
        int status = 0;
        err = univ_mss_file_mkdir( context );

        execCmdOut_t* execCmdOut = NULL;

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx.prop_map().get< std::string >( SCRIPT_PROP, script );
        if ( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        execCmd_t execCmdInp;
        bzero( &execCmdInp, sizeof( execCmdInp ) );
        rstrcpy( execCmdInp.cmd, script.c_str(), LONG_NAME_LEN );
        snprintf( execCmdInp.cmdArgv, sizeof( execCmdInp.cmdArgv ), "syncToArch %s %s", _cache_file_name, filename.c_str() );
        rstrcpy( execCmdInp.execAddr, "localhost", LONG_NAME_LEN );
        status = _rsExecCmd( &execCmdInp, &execCmdOut );
        if ( status == 0 ) {
            err = univ_mss_file_chmod( _ctx );
            if ( !err.ok() ) {
                PASSMSG( "univ_mss_file_sync_to_arch - failed.", err );
            }
        }
        else {
            status = UNIV_MSS_SYNCTOARCH_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_sync_to_arch: copy of [";
            msg << _cache_file_name;
            msg << "] to [";
            msg << filename;
            msg << "] failed.";
            msg << "   stdout buff [";
            msg << execCmdOut->stdoutBuf.buf;
            msg << "]   stderr buff [";
            msg << execCmdOut->stderrBuf.buf;
            msg << "]  status [";
            msg << execCmdOut->status << "]";
            freeCmdExecOut( execCmdOut );
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // univ_mss_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    irods::error univ_mss_file_registered(
        irods::resource_plugin_context& _ctx ) {
        // Check the operation parameters and update the physical path
        irods::error ret = univ_mss_check_param< irods::file_object >( _ctx );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG( msg.str(), ret );
        }
        // NOOP
        return SUCCESS();
    } // univ_mss_file_registered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    irods::error univ_mss_file_unregistered(
        irods::resource_plugin_context& _ctx ) {
        // Check the operation parameters and update the physical path
        irods::error ret = univ_mss_check_param< irods::file_object >( _ctx );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG( msg.str(), ret );
        }
        // NOOP
        return SUCCESS();
    } // univ_mss_file_unregistered

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    irods::error univ_mss_file_modified(
        irods::resource_plugin_context& _ctx ) {
        // Check the operation parameters and update the physical path
        irods::error ret = univ_mss_check_param< irods::file_object >( _ctx );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG( msg.str(), ret );
        }
        // NOOP
        return SUCCESS();
    } // univ_mss_file_modified

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    irods::error univ_mss_file_redirect_create(
        irods::plugin_property_map& _prop_map,
        const std::string&           _curr_host,
        float&                       _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down
        int resc_status = 0;
        irods::error get_ret = _prop_map.get< int >( irods::RESOURCE_STATUS, resc_status );
        if ( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_create - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if ( INT_RESC_STATUS_DOWN == resc_status ) {
            _out_vote = 0.0;
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( irods::RESOURCE_LOCATION, host_name );
        if ( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_create - failed to get 'location' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // vote higher if we are on the same host
        if ( _curr_host == host_name ) {
            _out_vote = 1.0;
        }
        else {
            _out_vote = 0.5;
        }

        return SUCCESS();

    } // univ_mss_file_redirect_create

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    irods::error univ_mss_file_redirect_open(
        irods::plugin_property_map& _prop_map,
        irods::file_object_ptr         _file_obj,
        const std::string&           _resc_name,
        const std::string&           _curr_host,
        float&                       _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down
        int resc_status = 0;
        irods::error get_ret = _prop_map.get< int >( irods::RESOURCE_STATUS, resc_status );
        if ( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_open - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if ( INT_RESC_STATUS_DOWN == resc_status ) {
            _out_vote = 0.0;
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( irods::RESOURCE_LOCATION, host_name );
        if ( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_open - failed to get 'location' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // set a flag to test if were at the curr host, if so we vote higher
        bool curr_host = ( _curr_host == host_name );

        // =-=-=-=-=-=-=-
        // make some flags to clairify decision making
        bool need_repl = ( _file_obj->repl_requested() > -1 );

        // =-=-=-=-=-=-=-
        // set up variables for iteration
        irods::error final_ret = SUCCESS();
        std::vector< irods::physical_object > objs = _file_obj->replicas();
        std::vector< irods::physical_object >::iterator itr = objs.begin();

        // =-=-=-=-=-=-=-
        // initially set vote to 0.0
        _out_vote = 0.0;

        // =-=-=-=-=-=-=-
        // check to see if the replica is in this resource, if one is requested
        for ( ; itr != objs.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // run the hier string through the parser and get the last
            // entry.
            std::string last_resc;
            irods::hierarchy_parser parser;
            parser.set_string( itr->resc_hier() );
            parser.last_resc( last_resc );

            // =-=-=-=-=-=-=-
            // more flags to simplify decision making
            bool repl_us = ( _file_obj->repl_requested() == itr->repl_num() );
            bool resc_us = ( _resc_name == last_resc );

            // =-=-=-=-=-=-=-
            // success - correct resource and dont need a specific
            //           replication, or the repl nums match
            if ( resc_us ) {
                if ( !need_repl || ( need_repl && repl_us ) ) {
                    if ( curr_host ) {
                        _out_vote = 1.0;
                    }
                    else {
                        _out_vote = 0.5;
                    }
                    break;
                }

            } // if resc_us

        } // for itr

        return SUCCESS();

    } // redirect_get

    // =-=-=-=-=-=-=-
    // used to allow the resource to determine which host
    // should provide the requested operation
    irods::error univ_mss_file_redirect(
        irods::resource_plugin_context& _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        irods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {
        // =-=-=-=-=-=-=-
        // check the context validity
        irods::error ret = _ctx.valid< irods::file_object >();
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }

        // =-=-=-=-=-=-=-
        // check incoming parameters
        if ( !_opr ) {
            return ERROR( -1, "univ_mss_file_redirect- null operation" );
        }
        if ( !_curr_host ) {
            return ERROR( -1, "univ_mss_file_redirect- null operation" );
        }
        if ( !_out_parser ) {
            return ERROR( -1, "univ_mss_file_redirect- null outgoing hier parser" );
        }
        if ( !_out_vote ) {
            return ERROR( -1, "univ_mss_file_redirect- null outgoing vote" );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "univ_mss_file_redirect- failed in get property for name";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if ( irods::OPEN_OPERATION == ( *_opr ) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'get' operation
            return univ_mss_file_redirect_open( _ctx.prop_map(), file_obj, resc_name, ( *_curr_host ), ( *_out_vote ) );

        }
        else if ( irods::CREATE_OPERATION == ( *_opr ) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'create' operation
            return univ_mss_file_redirect_create( _ctx.prop_map(), ( *_curr_host ), ( *_out_vote ) );
        }

        // =-=-=-=-=-=-=-
        // must have been passed a bad operation
        std::stringstream msg;
        msg << "univ_mss_file_redirect- operation not supported [";
        msg << ( *_opr ) << "]";
        return ERROR( -1, msg.str() );

    } // univ_mss_file_redirect


    // =-=-=-=-=-=-=-
    // univ_mss__file_rebalance - code which would rebalance the subtree
    irods::error univ_mss__file_rebalance(
        irods::resource_plugin_context& _ctx ) {
        return update_resource_object_count(
                   _ctx.comm(),
                   _ctx.prop_map() );

    } // univ_mss__file_rebalancec

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle universal mss resources
    //    context string will hold the script to be called.
    class univ_mss_resource : public irods::resource {
        public:
            univ_mss_resource( const std::string& _inst_name,
                               const std::string& _context ) :
                irods::resource( _inst_name, _context ) {

                // =-=-=-=-=-=-=-
                // check the context string for inappropriate path behavior
                if ( context_.find( "/" ) != std::string::npos ) {
                    std::stringstream msg;
                    msg << "univmss resource :: the path [";
                    msg << context_;
                    msg << "] should be a single file name which should reside in iRODS/server/bin/cmd/";
                    rodsLog( LOG_ERROR, "[%s]", msg.str().c_str() );
                }

                // =-=-=-=-=-=-=-
                // assign context string as the univ mss script to call
                properties_.set< std::string >( SCRIPT_PROP, context_ );
            }

            // =-=-=-=-=-=-
            // override from plugin_base
            irods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
                _flg = false;
                return SUCCESS();
            }

            // =-=-=-=-=-=-
            // override from plugin_base
            irods::error post_disconnect_maintenance_operation( irods::pdmo_type& ) {
                return ERROR( -1, "nop" );
            }

    }; // class univ_mss_resource

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the irods facing interface defined in
    //    server/drivers/src/fileDriver.c
    irods::resource* plugin_factory( const std::string& _inst_name,
                                     const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // 4a. create univ_mss_resource object
        univ_mss_resource* resc = new univ_mss_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( irods::RESOURCE_OP_CREATE,            "univ_mss_file_create" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,              "univ_mss_file_open" );
        resc->add_operation( irods::RESOURCE_OP_READ,              "univ_mss_file_read" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,             "univ_mss_file_write" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,             "univ_mss_file_close" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,            "univ_mss_file_unlink" );
        resc->add_operation( irods::RESOURCE_OP_STAT,              "univ_mss_file_stat" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,             "univ_mss_file_mkdir" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,           "univ_mss_file_opendir" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,           "univ_mss_file_readdir" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,            "univ_mss_file_rename" );
        resc->add_operation( irods::RESOURCE_OP_TRUNCATE,          "univ_mss_file_truncate" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,         "univ_mss_file_getfs_freespace" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,             "univ_mss_file_lseek" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,             "univ_mss_file_rmdir" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,          "univ_mss_file_closedir" );
        resc->add_operation( irods::RESOURCE_OP_STAGETOCACHE,      "univ_mss_file_stage_to_cache" );
        resc->add_operation( irods::RESOURCE_OP_SYNCTOARCH,        "univ_mss_file_sync_to_arch" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,        "univ_mss_file_registered" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED,      "univ_mss_file_unregistered" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,          "univ_mss_file_modified" );

        resc->add_operation( irods::RESOURCE_OP_RESOLVE_RESC_HIER, "univ_mss_file_redirect" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,         "univ_mss__file_rebalance" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     irods::resource pointer
        return dynamic_cast<irods::resource*>( resc );

    } // plugin_factory

}; // extern "C"



