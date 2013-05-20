


// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "generalAdmin.h"
#include "physPath.h"
#include "reIn2p3SysRule.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_physical_object.h"
#include "eirods_collection_object.h"
#include "eirods_string_tokenize.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_redirect.h"
#include "eirods_stacktrace.h"

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


extern "C" {

#define NB_READ_TOUT_SEC        60      /* 60 sec timeout */
#define NB_WRITE_TOUT_SEC       60      /* 60 sec timeout */

    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;

    /// =-=-=-=-=-=-=-
    /// @brief Check the general parameters passed in to most plugin functions
    inline eirods::error univ_mss_check_param(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // check the resource context
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "resource context is null" );
        }

        // =-=-=-=-=-=-=-
        // ask the context if it is valid
        eirods::error ret = _ctx->valid();
        if( !ret.ok() ) {
            return PASSMSG( "resource context is invalid", ret );

        }
       
        return SUCCESS();
 
    } // univ_mss_check_param

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX create
    eirods::error univ_mss_file_create( 
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
   
    } // univ_mss_file_create

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error univ_mss_file_open( 
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
 
    } // univ_mss_file_open

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Read
    eirods::error univ_mss_file_read(
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
 
    } // univ_mss_file_read

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Write
    eirods::error univ_mss_file_write( 
        eirods::resource_operation_context* _ctx,
        void*                               _buf, 
        int                                 _len ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
 
    } // univ_mss_file_write

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Close
    eirods::error univ_mss_file_close(
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
 
    } // univ_mss_file_close

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Unlink
    eirods::error univ_mss_file_unlink(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // check context
        eirods::error err = univ_mss_check_param( _ctx );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }
 
        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx->prop_map().get< std::string >( "script", script );
        if( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }
        
        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        eirods::first_class_object& fco = _ctx->fco();
        std::string filename = fco.physical_path();


        int status;
        execCmd_t execCmdInp;
        char cmdArgv[HUGE_NAME_LEN] = "";
        execCmdOut_t *execCmdOut = NULL;
        
        bzero (&execCmdInp, sizeof (execCmdInp));
        rstrcpy(execCmdInp.cmd, script.c_str(), LONG_NAME_LEN);
        strcat(cmdArgv, "rm");
        strcat(cmdArgv, " '");
        strcat(cmdArgv, filename.c_str());
        strcat(cmdArgv, "'");
        rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
        rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
        status = _rsExecCmd( _ctx->comm(), &execCmdInp, &execCmdOut);

        if (status < 0) {
            status = UNIV_MSS_UNLINK_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_unlink - failed for [";
            msg << filename;
            msg << "]";
            return ERROR( status, msg.str() );
        }

        return CODE(status);

    } // univ_mss_file_unlink

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Stat
    eirods::error univ_mss_file_stat(
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        // =-=-=-=-=-=-=-
        // check context
        eirods::error err = univ_mss_check_param( _ctx );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }
 
        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx->prop_map().get< std::string >( "script", script );
        if( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }
        
        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        eirods::first_class_object& fco = _ctx->fco();
        std::string filename = fco.physical_path();


        int i, status;
        execCmd_t execCmdInp;
        char cmdArgv[HUGE_NAME_LEN] = "";
        char splchain1[13][MAX_NAME_LEN], splchain2[4][MAX_NAME_LEN], splchain3[3][MAX_NAME_LEN];
        char *outputStr;
        const char *delim1 = ":\n";
        const char *delim2 = "-";
        const char *delim3 = ".";
        execCmdOut_t *execCmdOut = NULL;
        struct tm mytm;
        time_t myTime;
        
        bzero (&execCmdInp, sizeof (execCmdInp));
        rstrcpy(execCmdInp.cmd, script.c_str(), LONG_NAME_LEN);
        strcat(cmdArgv, "stat");
        strcat(cmdArgv, " '");
        strcat(cmdArgv, filename.c_str());
        strcat(cmdArgv, "' ");
        rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
        rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
        status = _rsExecCmd( _ctx->comm(), &execCmdInp, &execCmdOut );
        
        if (status == 0 && NULL != execCmdOut ) { // JMC cppcheck - nullptr
            if ( execCmdOut->stdoutBuf.buf != NULL) {
                outputStr = (char*)execCmdOut->stdoutBuf.buf;
                memset(&splchain1, 0, sizeof(splchain1));
                strSplit(outputStr, delim1, splchain1);
                _statbuf->st_dev = atoi(splchain1[0]);
                _statbuf->st_ino = atoi(splchain1[1]);
                _statbuf->st_mode = atoi(splchain1[2]);
                _statbuf->st_nlink = atoi(splchain1[3]);
                _statbuf->st_uid = atoi(splchain1[4]);
                _statbuf->st_gid = atoi(splchain1[5]);
                _statbuf->st_rdev = atoi(splchain1[6]);
                _statbuf->st_size = atoll(splchain1[7]);
                _statbuf->st_blksize = atoi(splchain1[8]);
                _statbuf->st_blocks = atoi(splchain1[9]);
                for (i = 0; i < 3; i++) {
                    memset(&splchain2, 0, sizeof(splchain2));
                    memset(&splchain3, 0, sizeof(splchain3));
                    strSplit(splchain1[10+i], delim2, splchain2);
                    mytm.tm_year = atoi(splchain2[0]) - 1900;
                    mytm.tm_mon = atoi(splchain2[1]) - 1;
                    mytm.tm_mday = atoi(splchain2[2]);
                    strSplit(splchain2[3], delim3, splchain3);
                    mytm.tm_hour = atoi(splchain3[0]);
                    mytm.tm_min = atoi(splchain3[1]);
                    mytm.tm_sec = atoi(splchain3[2]);
                    myTime = mktime(&mytm);
                    switch (i) {
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
            return ERROR( status, msg.str() ); 

        }
     
        return CODE(status);

    } // univ_mss_file_stat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX Fstat
    eirods::error univ_mss_file_fstat(
        eirods::resource_operation_context* _ctx,
        struct stat*                        _statbuf ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
 
    } // univ_mss_file_fstat

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX lseek
    eirods::error univ_mss_file_lseek(
        eirods::resource_operation_context* _ctx,
        size_t                              _offset, 
        int                                 _whence ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
 
    } // univ_mss_file_lseek

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX fsync
    eirods::error univ_mss_file_fsync(
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );
 
    } // univ_mss_file_fsync

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX chmod
    eirods::error univ_mss_file_chmod(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // check context
        eirods::error err = univ_mss_check_param( _ctx );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }
 
        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx->prop_map().get< std::string >( "script", script );
        if( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }
        
        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        eirods::first_class_object& fco = _ctx->fco();
        std::string filename = fco.physical_path();

        int mode = fco.mode(); 
        int status = 0;
        execCmd_t execCmdInp;
        char cmdArgv[HUGE_NAME_LEN] = "";
        char strmode[4];
        execCmdOut_t *execCmdOut = NULL;  
        
        if ( mode != getDefDirMode() ) {
            mode = getDefFileMode();
        }
        
        bzero (&execCmdInp, sizeof (execCmdInp));
        rstrcpy(execCmdInp.cmd, script.c_str(), LONG_NAME_LEN);
        strcat(cmdArgv, "chmod");
        strcat(cmdArgv, " '");
        strcat(cmdArgv, filename.c_str());
        strcat(cmdArgv, "' ");
        sprintf (strmode, "%o", mode);
        strcat(cmdArgv, strmode);
        rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
        rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
        status = _rsExecCmd( _ctx->comm(), &execCmdInp, &execCmdOut);
        
        if (status < 0) {
            status = UNIV_MSS_CHMOD_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_chmod - failed for [";
            msg << filename;
            msg << "]";
            return ERROR( status, msg.str() ); 

        }
        
        return CODE(status);

    } // univ_mss_file_chmod

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX mkdir
    eirods::error univ_mss_file_mkdir(
        eirods::resource_operation_context* _ctx ) { 
        // =-=-=-=-=-=-=-
        // check context
        eirods::error err = univ_mss_check_param( _ctx );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }
 
        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx->prop_map().get< std::string >( "script", script );
        if( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }
        
        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        eirods::first_class_object& fco = _ctx->fco();
        std::string dirname = fco.physical_path();

        int status = 0;
        execCmd_t execCmdInp;
        char cmdArgv[HUGE_NAME_LEN] = "";
        execCmdOut_t *execCmdOut = NULL;  

        bzero (&execCmdInp, sizeof (execCmdInp));
        rstrcpy( execCmdInp.cmd, script.c_str(), LONG_NAME_LEN );
        strcat(cmdArgv, "mkdir");
        strcat(cmdArgv, " '");
        strcat(cmdArgv, dirname.c_str() );
        strcat(cmdArgv, "'");
        rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
        rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
        status = _rsExecCmd( _ctx->comm(), &execCmdInp, &execCmdOut );
        if (status < 0) {
            status = UNIV_MSS_MKDIR_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_mkdir - mkdir failed for [";
            msg << dirname;
            msg << "]";
            return ERROR( status, msg.str() ); 
        }
        
        int mode = getDefDirMode(); 
        fco.mode( mode );
        err = univ_mss_file_chmod( _ctx );
         
        return err;

    } // univ_mss_file_mkdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rmdir
    eirods::error univ_mss_file_rmdir(
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_rmdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX opendir
    eirods::error univ_mss_file_opendir(
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_opendir

    // =-=-=-=-=-=-=-
    /// @brief interface for POSIX closedir
    eirods::error univ_mss_file_closedir(
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_closedir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX readdir
    eirods::error univ_mss_file_readdir(
        eirods::resource_operation_context* _ctx,
        struct rodsDirent**                 _dirent_ptr ) {
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_readdir

    /// =-=-=-=-=-=-=-
    /// @brief interface for POSIX rename
    eirods::error univ_mss_file_rename(
        eirods::resource_operation_context* _ctx,
        const char*                         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check context
        eirods::error err = univ_mss_check_param( _ctx );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }
 
        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx->prop_map().get< std::string >( "script", script );
        if( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }
        
        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        eirods::first_class_object& fco = _ctx->fco();
        std::string filename = fco.physical_path();

        // =-=-=-=-=-=-=-
        // first create the directory name
        char  dirname[MAX_NAME_LEN] = "";
        const char* lastpart = strrchr( _new_file_name, '/');
        int   lenDir   = strlen( _new_file_name ) - strlen( lastpart );
        strncpy( dirname, _new_file_name, lenDir );
        
        // =-=-=-=-=-=-=-
        // create a context to call the mkdir operation
        eirods::collection_object coll_obj( dirname, fco.resc_hier(), fco.mode(), 0 );
        eirods::resource_operation_context context( _ctx->comm(), 
                                                    _ctx->prop_map(), 
                                                    _ctx->child_map(), 
                                                    coll_obj, "" );
        // =-=-=-=-=-=-=-
        // create the directory on the MSS
        int status = 0;
        err = univ_mss_file_mkdir( &context );

        execCmd_t execCmdInp;
        char cmdArgv[HUGE_NAME_LEN] = "";
        execCmdOut_t *execCmdOut = NULL;
        
        bzero (&execCmdInp, sizeof (execCmdInp));
        rstrcpy(execCmdInp.cmd, script.c_str(), LONG_NAME_LEN);
        strcat(cmdArgv, "mv");
        strcat(cmdArgv, " '");
        strcat(cmdArgv, filename.c_str());
        strcat(cmdArgv, "' '");
        strcat(cmdArgv, _new_file_name );
        strcat(cmdArgv, "'");
        rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
        rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
        status = _rsExecCmd( _ctx->comm(), &execCmdInp, &execCmdOut);

        if (status < 0) {
            status = UNIV_MSS_RENAME_ERR - errno;
            std::stringstream msg;
            msg << "univ_mss_file_rename - failed for [";
            msg << filename;
            msg << "]";
            return ERROR( status, msg.str() ); 

        }

        return CODE(status);

    } // univ_mss_file_rename

    /// =-=-=-=-=-=-=-
    /// @brief interface to determine free space on a device given a path
    eirods::error univ_mss_file_getfs_freespace(
        eirods::resource_operation_context* _ctx ) { 
        return ERROR( SYS_NOT_SUPPORTED, __FUNCTION__ );

    } // univ_mss_file_getfs_freespace

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from filename to cacheFilename. optionalInfo info
    ///        is not used.
    eirods::error univ_mss_file_stage_to_cache(
        eirods::resource_operation_context* _ctx,
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // check context
        eirods::error err = univ_mss_check_param( _ctx );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        eirods::first_class_object& fco = _ctx->fco();
        std::string filename = fco.physical_path();

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx->prop_map().get< std::string >( "script", script );
        if( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        int status = 0;
        execCmd_t execCmdInp;
        char cmdArgv[HUGE_NAME_LEN] = "";
        execCmdOut_t *execCmdOut = NULL;
        bzero (&execCmdInp, sizeof (execCmdInp));

        rstrcpy(execCmdInp.cmd, script.c_str(), LONG_NAME_LEN);
        strcat(cmdArgv, "stageToCache");
        strcat(cmdArgv, " '");
        strcat(cmdArgv, filename.c_str() );
        strcat(cmdArgv, "' '");
        strcat(cmdArgv, _cache_file_name );
        strcat(cmdArgv, "'");
        rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
        rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
        status = _rsExecCmd( _ctx->comm(), &execCmdInp, &execCmdOut);
        
        if (status < 0) {
            status = UNIV_MSS_STAGETOCACHE_ERR - errno; 
            std::stringstream msg;
            msg << "univ_mss_file_stage_to_cache: staging from [";
            msg << _cache_file_name;
            msg << "] to [";
            msg << filename;
            msg << "] failed.";
            return ERROR( status, msg.str() );
        }
        
        return CODE(status);
	
    } // univ_mss_file_stage_to_cache

    /// =-=-=-=-=-=-=-
    /// @brief This routine is for testing the TEST_STAGE_FILE_TYPE.
    ///        Just copy the file from cacheFilename to filename. optionalInfo info
    ///        is not used.
    eirods::error univ_mss_file_sync_to_arch(
        eirods::resource_operation_context* _ctx, 
        const char*                         _cache_file_name ) { 
        // =-=-=-=-=-=-=-
        // check context
        eirods::error err = univ_mss_check_param( _ctx );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - invalid context";
            return PASSMSG( msg.str(), err );

        }

        // =-=-=-=-=-=-=-
        // snag a ref to the fco
        eirods::first_class_object& fco = _ctx->fco();
        std::string filename = fco.physical_path();

        // =-=-=-=-=-=-=-
        // first create the directory name
        char  dirname[MAX_NAME_LEN] = "";
        const char* lastpart = strrchr( filename.c_str(), '/');
        int   lenDir   = strlen( filename.c_str() ) - strlen( lastpart );
        strncpy( dirname, filename.c_str(), lenDir );
        
        // =-=-=-=-=-=-=-
        // create a context to call the mkdir operation
        eirods::collection_object coll_obj( dirname, fco.resc_hier(), fco.mode(), 0 );
        eirods::resource_operation_context context( _ctx->comm(), 
                                                    _ctx->prop_map(), 
                                                    _ctx->child_map(), 
                                                    coll_obj, "" );
        // =-=-=-=-=-=-=-
        // create the directory on the MSS
        int status = 0;
        err = univ_mss_file_mkdir( &context );
        
        execCmdOut_t* execCmdOut = NULL;
        char  cmdArgv[HUGE_NAME_LEN] = "";
    
        execCmd_t execCmdInp;
        bzero (&execCmdInp, sizeof (execCmdInp));

        // =-=-=-=-=-=-=-
        // get the script property
        std::string script;
        err = _ctx->prop_map().get< std::string >( "script", script );
        if( !err.ok() ) {
            return PASSMSG( __FUNCTION__, err );
        }

        rstrcpy( execCmdInp.cmd, script.c_str(), LONG_NAME_LEN );
        strcat(cmdArgv, "syncToArch");
        strcat(cmdArgv, " ");
        strcat(cmdArgv, _cache_file_name);
        strcat(cmdArgv, " ");
        strcat(cmdArgv, filename.c_str() );
        strcat(cmdArgv, "");

        rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
        rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
        status = _rsExecCmd( _ctx->comm(), &execCmdInp, &execCmdOut );
        if ( status == 0 ) {
            err = univ_mss_file_chmod( _ctx );
            if( !err.ok() ) {
                PASSMSG( "univ_mss_file_sync_to_arch - failed.", err );
            }
        } else {
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
            return ERROR( status, msg.str() );       
        }

        return CODE( status );

    } // univ_mss_file_sync_to_arch

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    eirods::error univ_mss_file_registered_plugin(
        eirods::resource_operation_context* _ctx) {
        // Check the operation parameters and update the physical path
        eirods::error ret = univ_mss_check_param(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    eirods::error univ_mss_file_unregistered_plugin(
        eirods::resource_operation_context* _ctx) {
        // Check the operation parameters and update the physical path
        eirods::error ret = univ_mss_check_param(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }
    
    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    eirods::error univ_mss_file_modified_plugin(
        eirods::resource_operation_context* _ctx) {
        // Check the operation parameters and update the physical path
        eirods::error ret = univ_mss_check_param(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "Invalid parameters or physical path.";
            return PASSMSG(msg.str(), ret);
        }
        // NOOP
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error univ_mss_file_redirect_create( 
                      eirods::resource_property_map& _prop_map,
                      eirods::file_object&           _file_obj,
                      const std::string&             _resc_name, 
                      const std::string&             _curr_host, 
                      float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map.get< int >( "status", resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_create - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            _out_vote = 0.0;
            return SUCCESS(); 
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( "location", host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_create - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // vote higher if we are on the same host
        if( _curr_host == host_name ) {
            _out_vote = 1.0;
        } else {
            _out_vote = 0.5;
        }

        return SUCCESS();

    } // univ_mss_file_redirect_create

    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error univ_mss_file_redirect_open( 
                      eirods::resource_property_map& _prop_map,
                      eirods::file_object&           _file_obj,
                      const std::string&             _resc_name, 
                      const std::string&             _curr_host, 
                      float&                         _out_vote ) {
        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map.get< int >( "status", resc_status );
        if( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_open - failed to get 'status' property", get_ret );
        }

        // =-=-=-=-=-=-=-
        // if the status is down, vote no.
        if( INT_RESC_STATUS_DOWN == resc_status ) {
            _out_vote = 0.0;
            return SUCCESS(); 
        }

        // =-=-=-=-=-=-=-
        // get the resource host for comparison to curr host
        std::string host_name;
        get_ret = _prop_map.get< std::string >( "location", host_name );
        if( !get_ret.ok() ) {
            return PASSMSG( "univ_mss_file_redirect_open - failed to get 'location' property", get_ret );
        }
        
        // =-=-=-=-=-=-=-
        // set a flag to test if were at the curr host, if so we vote higher
        bool curr_host = ( _curr_host == host_name );

        // =-=-=-=-=-=-=-
        // make some flags to clairify decision making
        bool need_repl = ( _file_obj.repl_requested() > -1 );

        // =-=-=-=-=-=-=-
        // set up variables for iteration
        bool          found     = false;
        eirods::error final_ret = SUCCESS();
        std::vector< eirods::physical_object > objs = _file_obj.replicas();
        std::vector< eirods::physical_object >::iterator itr = objs.begin();
        
        // =-=-=-=-=-=-=-
        // initially set vote to 0.0
        _out_vote = 0.0;

        // =-=-=-=-=-=-=-
        // check to see if the replica is in this resource, if one is requested
        for( ; itr != objs.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // run the hier string through the parser and get the last
            // entry.
            std::string last_resc;
            eirods::hierarchy_parser parser;
            parser.set_string( itr->resc_hier() );
            parser.last_resc( last_resc ); 
          
            // =-=-=-=-=-=-=-
            // more flags to simplify decision making
            bool repl_us = ( _file_obj.repl_requested() == itr->repl_num() ); 
            bool resc_us = ( _resc_name == last_resc );

            // =-=-=-=-=-=-=-
            // success - correct resource and dont need a specific
            //           replication, or the repl nums match
            if( resc_us ) {
                if( !need_repl || ( need_repl && repl_us ) ) {
                    found = true;
                    if( curr_host ) {
                        _out_vote = 1.0;
                    } else {
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
    eirods::error univ_mss_file_redirect_plugin( 
        eirods::resource_operation_context* _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {

        // =-=-=-=-=-=-=-
        // check the context pointer
        if( !_ctx ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "univ_mss_file_redirect_plugin - invalid resource context" );
        }
         
        // =-=-=-=-=-=-=-
        // check the context validity
        eirods::error ret = _ctx->valid< eirods::file_object >(); 
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - resource context is invalid";
            return PASSMSG( msg.str(), ret );
        }
 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_opr ) {
            return ERROR( -1, "univ_mss_file_redirect_plugin - null operation" );
        }
        if( !_curr_host ) {
            return ERROR( -1, "univ_mss_file_redirect_plugin - null operation" );
        }
        if( !_out_parser ) {
            return ERROR( -1, "univ_mss_file_redirect_plugin - null outgoing hier parser" );
        }
        if( !_out_vote ) {
            return ERROR( -1, "univ_mss_file_redirect_plugin - null outgoing vote" );
        }
        
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::file_object& file_obj = dynamic_cast< eirods::file_object& >( _ctx->fco() );

        // =-=-=-=-=-=-=-
        // get the name of this resource
        std::string resc_name;
        ret = _ctx->prop_map().get< std::string >( "name", resc_name );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "univ_mss_file_redirect_plugin - failed in get property for name";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add ourselves to the hierarchy parser by default
        _out_parser->add_child( resc_name );

        // =-=-=-=-=-=-=-
        // test the operation to determine which choices to make
        if( eirods::EIRODS_OPEN_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'get' operation
            return univ_mss_file_redirect_open( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );

        } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
            // =-=-=-=-=-=-=-
            // call redirect determination for 'create' operation
            return univ_mss_file_redirect_create( _ctx->prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote)  );
        }
      
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation 
        std::stringstream msg;
        msg << "univ_mss_file_redirect_plugin - operation not supported [";
        msg << (*_opr) << "]";
        return ERROR( -1, msg.str() );

    } // univ_mss_file_redirect_plugin

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle universal mss resources
    //    context string will hold the script to be called.
    class univ_mss_resource : public eirods::resource {
    public:
        univ_mss_resource( const std::string& _inst_name, 
                           const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {
            
            // =-=-=-=-=-=-=-
            // check the context string for inappropriate path behavior
            if( context_.find( "/" ) != std::string::npos ) {
                std::stringstream msg;
                msg << "univmss resource :: the path [";
                msg << context_;
                msg << "] should be a single file name which should reside in iRODS/server/bin/cmd/";
                rodsLog( LOG_ERROR, "[%s]", msg.str().c_str() );
            }

            // =-=-=-=-=-=-=-
            // assign context string as the univ mss script to call
            properties_.set< std::string >( "script", context_ );
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            _flg = false;
            return SUCCESS();
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _pdmo ) {
            return ERROR( -1, "nop" );
        }

    }; // class univ_mss_resource

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, 
                                      const std::string& _context  ) {
        // =-=-=-=-=-=-=-
        // 4a. create univ_mss_resource object
        univ_mss_resource* resc = new univ_mss_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "univ_mss_file_create" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "univ_mss_file_open" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "univ_mss_file_read" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "univ_mss_file_write" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "univ_mss_file_close" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "univ_mss_file_unlink" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "univ_mss_file_stat" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,        "univ_mss_file_fstat" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,        "univ_mss_file_fsync" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "univ_mss_file_mkdir" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "univ_mss_file_opendir" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "univ_mss_file_readdir" );
        resc->add_operation( eirods::RESOURCE_OP_STAGE,        "univ_mss_file_stage" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "univ_mss_file_rename" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "univ_mss_file_getfs_freespace" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "univ_mss_file_lseek" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "univ_mss_file_rmdir" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "univ_mss_file_closedir" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "univ_mss_file_stage_to_cache" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "univ_mss_file_sync_to_arch" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "univ_mss_file_registered_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "univ_mss_file_unregistered_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "univ_mss_file_modified_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "univ_mss_redirect" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( "check_path_perm", 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( "create_path",     1 );//CREATE_PATH );
        resc->set_property< int >( "category",        0 );//FILE_CAT );
        
        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



