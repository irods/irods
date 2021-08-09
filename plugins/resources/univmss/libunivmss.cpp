#include "msParam.h"
#include "generalAdmin.h"
#include "physPath.hpp"
#include "reIn2p3SysRule.hpp"
#include "miscServerFunct.hpp"
#include "rsExecCmd.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_physical_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"
#include "irods_re_structs.hpp"
#include "voting.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>
#include <boost/algorithm/string.hpp>

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

/// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
inline irods::error univ_mss_check_param(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if ( !ret.ok() ) {
        return PASSMSG( "resource context is invalid", ret );

    }

    return SUCCESS();

} // univ_mss_check_param


#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

// =-=-=-=-=-=-=-
/// @brief token to index the script property
const std::string SCRIPT_PROP( "script" );

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX create
irods::error univ_mss_file_create(
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_create

// =-=-=-=-=-=-=-
// interface for POSIX Open
irods::error univ_mss_file_open(
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_open

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Read
irods::error univ_mss_file_read(
    irods::plugin_context& ,
    void*,
    int ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_read

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Write
irods::error univ_mss_file_write(
    irods::plugin_context&,
    void*,
    int ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_write

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Close
irods::error univ_mss_file_close(
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_close

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Unlink
irods::error univ_mss_file_unlink(
    irods::plugin_context& _ctx ) {
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
    irods::plugin_context& _ctx,
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
    const char *delim1 = ":\n";
    const char *delim2 = "-";
    const char *delim3 = ".";
    execCmdOut_t *execCmdOut = NULL;
    struct tm mytm;
    time_t myTime;

    std::memset(&execCmdInp, 0, sizeof(execCmdInp));
    rstrcpy( execCmdInp.cmd, script.c_str(), LONG_NAME_LEN );
    snprintf( cmdArgv, sizeof( cmdArgv ), "stat '%s' ", filename.c_str() );
    rstrcpy( execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN );
    rstrcpy( execCmdInp.execAddr, "localhost", LONG_NAME_LEN );
    status = _rsExecCmd( &execCmdInp, &execCmdOut );

    if ( status == 0 && NULL != execCmdOut ) { // JMC cppcheck - nullptr
        if ( execCmdOut->stdoutBuf.buf != NULL ) {
            const std::string outputStr(static_cast<char*>(execCmdOut->stdoutBuf.buf), execCmdOut->stdoutBuf.len);
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
    irods::plugin_context&,
    long long,
    int ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_lseek

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX chmod
irods::error univ_mss_file_chmod(
    irods::plugin_context& _ctx ) {
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

    std::memset(&execCmdInp, 0, sizeof(execCmdInp));
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
    irods::plugin_context& _ctx ) {
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

    std::memset(&execCmdInp, 0, sizeof(execCmdInp));
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
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_rmdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX opendir
irods::error univ_mss_file_opendir(
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_opendir

// =-=-=-=-=-=-=-
/// @brief interface for POSIX closedir
irods::error univ_mss_file_closedir(
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_closedir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX readdir
irods::error univ_mss_file_readdir(
    irods::plugin_context& ,
    struct rodsDirent** ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_readdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX rename
irods::error univ_mss_file_rename(
    irods::plugin_context& _ctx,
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
    irods::plugin_context context(
        _ctx.prop_map(),
        coll_obj, "" );

    // =-=-=-=-=-=-=-
    // create the directory on the MSS
    int status = 0;
    err = univ_mss_file_mkdir( context );

    execCmd_t execCmdInp;

    std::memset(&execCmdInp, 0, sizeof(execCmdInp));
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

    // issue 4326 - plugins must set the physical path to the new path
    fco->physical_path(_new_file_name);

    return CODE( status );

} // univ_mss_file_rename

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX truncate
irods::error univ_mss_file_truncate(
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_truncate

/// =-=-=-=-=-=-=-
/// @brief interface to determine free space on a device given a path
irods::error univ_mss_file_getfs_freespace(
    irods::plugin_context& ) {
    return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

} // univ_mss_file_getfs_freespace

/// =-=-=-=-=-=-=-
/// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
///        Just copy the file from filename to cacheFilename. optionalInfo info
///        is not used.
irods::error univ_mss_file_stage_to_cache(
    irods::plugin_context& _ctx,
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
    std::memset(&execCmdInp, 0, sizeof(execCmdInp));
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
    irods::plugin_context& _ctx,
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
    irods::plugin_context context(
        _ctx.prop_map(),
        coll_obj, "" );

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
    std::memset(&execCmdInp, 0, sizeof(execCmdInp));
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
    irods::plugin_context& _ctx ) {
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
    irods::plugin_context& _ctx ) {
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
    irods::plugin_context& _ctx ) {
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
// used to allow the resource to determine which host
// should provide the requested operation
irods::error univ_mss_file_resolve_hierarchy(
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
        return irods::error{e};
    }
    return ERROR(SYS_UNKNOWN_ERROR, "An unknown error occurred while resolving hierarchy.");
} // univ_mss_file_resolve_hierarchy


// =-=-=-=-=-=-=-
// univ_mss__file_rebalance - code which would rebalance the subtree
irods::error univ_mss_file_rebalance(
    irods::plugin_context& _ctx ) {
    return SUCCESS();

} // univ_mss_file_rebalance

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
                msg << "] should be a single file name which should reside in msiExecCmd_bin";
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
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name,
                                 const std::string& _context ) {
    // =-=-=-=-=-=-=-
    // 4a. create univ_mss_resource object
    univ_mss_resource* resc = new univ_mss_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            univ_mss_file_unlink ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            univ_mss_file_stage_to_cache ) );

    resc->add_operation(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            univ_mss_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            univ_mss_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            univ_mss_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            univ_mss_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            univ_mss_file_truncate ) );

    resc->add_operation(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            univ_mss_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            univ_mss_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory
