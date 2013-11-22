/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "rcConnect.h"

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
#include <iomanip>

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
#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#include <sys/stat.h>

#include <string.h>


// =-=-=-=-=-=-=-
// 2. Define utility functions that the operations might need

// =-=-=-=-=-=-=-
// NOTE: All storage resources must do this on the physical path stored in the file object and then update 
//       the file object's physical path with the full path

// =-=-=-=-=-=-=-
/// @brief Generates a full path name from the partial physical path and the specified resource's vault path
eirods::error unix_generate_full_path(
    eirods::plugin_property_map& _prop_map,
    const std::string&           _phy_path,
    std::string&                 _ret_string )
{
    eirods::error result = SUCCESS();
    eirods::error ret;
    std::string vault_path;
    
    // TODO - getting vault path by property will not likely work for coordinating nodes
    ret = _prop_map.get<std::string>( eirods::RESOURCE_PATH, vault_path);
    if((result = ASSERT_PASS(ret, "Resource has no vault path.")).ok()) {

        if(_phy_path.compare(0, 1, "/") != 0 &&
           _phy_path.compare(0, vault_path.size(), vault_path) != 0) {
            _ret_string  = vault_path;
            _ret_string += "/";
            _ret_string += _phy_path;
        } else {
            // The physical path already contains the vault path
            _ret_string = _phy_path;
        }
    }
    
    return result;

} // unix_generate_full_path

// =-=-=-=-=-=-=-
/// @brief update the physical path in the file object
eirods::error unix_check_path( 
    eirods::resource_plugin_context& _ctx )
{
    eirods::error result = SUCCESS();
    try {
        eirods::data_object_ptr data_obj = boost::dynamic_pointer_cast< eirods::data_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // NOTE: Must do this for all storage resources
        std::string full_path;
        eirods::error ret = unix_generate_full_path( _ctx.prop_map(), 
                                                     data_obj->physical_path(), 
                                                     full_path );
        if((result = ASSERT_PASS(ret, "Failed generating full path for object.")).ok()) {

            data_obj->physical_path( full_path );
        }

        return result;

    } catch( std::bad_cast ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "failed to cast fco to data_object" );

    }

} // unix_check_path

// =-=-=-=-=-=-=-
/// @brief Checks the basic operation parameters and updates the physical path in the file object
template< typename DEST_TYPE >
eirods::error unix_check_params_and_path(
    eirods::resource_plugin_context& _ctx )
{
    
    eirods::error result = SUCCESS();
    eirods::error ret;
  
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid 
    ret = _ctx.valid< DEST_TYPE >(); 
    if((result = ASSERT_PASS(ret, "Resource context is invalid.")).ok() ) { 

        result = unix_check_path( _ctx );
    }

    return result;

} // unix_check_params_and_path

extern "C" {
    // =-=-=-=-=-=-=-
    // 3. Define operations which will be called by the file*
    //    calls declared in server/driver/include/fileDriver.h
    // =-=-=-=-=-=-=-

    // =-=-=-=-=-=-=-
    // NOTE :: to access properties in the _prop_map do the 
    //      :: following :
    //      :: double my_var = 0.0;
    //      :: eirods::error ret = _prop_map.get< double >( "my_key", my_var ); 
    // =-=-=-=-=-=-=-

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error mock_archive_unlink_plugin( 
        eirods::resource_plugin_context& _ctx )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path< eirods::file_object >( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid plugin context.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // make the call to unlink      
            int status = unlink( fco->physical_path().c_str() );

            // =-=-=-=-=-=-=-
            // error handling
            int err_status = UNIX_FILE_UNLINK_ERR - errno;
            result = ASSERT_ERROR(status >= 0, err_status, "Unlink error for: \"%s\", errno = \"%s\", status = %d.",
                                  fco->physical_path().c_str(), strerror(errno), err_status);
        }
        
        return result;

    } // mock_archive_unlink_plugin

    int
    mockArchiveCopyPlugin(
        int         mode, 
        const char* srcFileName, 
        const char* destFileName )
    {
        int inFd, outFd;
        char myBuf[TRANS_BUF_SZ];
        rodsLong_t bytesCopied = 0;
        int bytesRead;
        int bytesWritten;
        int status;
        struct stat statbuf;

        status = stat (srcFileName, &statbuf);

        if (status < 0) {
            status = UNIX_FILE_STAT_ERR - errno;
            rodsLog (LOG_ERROR, 
                     "mockArchiveCopyPlugin: stat of %s error, status = %d",
                     srcFileName, status);
            return status;
        }

        inFd = open (srcFileName, O_RDONLY, 0);
        if (inFd < 0 || (statbuf.st_mode & S_IFREG) == 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "mockArchiveCopyPlugin: open error for srcFileName %s, status = %d",
                     srcFileName, status );
            close( inFd ); // JMC cppcheck - resource
            return status;
        }

        outFd = open (destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);
        if (outFd < 0) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLog (LOG_ERROR,
                     "mockArchiveCopyPlugin: open error for destFileName %s, status = %d",
                     destFileName, status);
            close (inFd);
            return status;
        }

        while ((bytesRead = read (inFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
            bytesWritten = write (outFd, (void *) myBuf, bytesRead);
            if (bytesWritten <= 0) {
                status = UNIX_FILE_WRITE_ERR - errno;
                rodsLog (LOG_ERROR,
                         "mockArchiveCopyPlugin: write error for srcFileName %s, status = %d",
                         destFileName, status);
                close (inFd);
                close (outFd);
                return status;
            }
            bytesCopied += bytesWritten;
        }

        close (inFd);
        close (outFd);

        if (bytesCopied != statbuf.st_size) {
            rodsLog ( LOG_ERROR,
                      "mockArchiveCopyPlugin: Copied size %lld does not match source \
                             size %lld of %s",
                      bytesCopied, statbuf.st_size, srcFileName );
            return SYS_COPY_LEN_ERR;
        } else {
            return 0;
        }

    } // mockArchiveCopyPlugin

    // =-=-=-=-=-=-=-
    // unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error mock_archive_stagetocache_plugin( 
        eirods::resource_plugin_context& _ctx,
        const char*                      _cache_file_name )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path< eirods::file_object >( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid plugin context.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
        
            // =-=-=-=-=-=-=-
            // get the vault path for the resource
            std::string path;
            ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_PATH, path ); 
            if((result = ASSERT_PASS(ret, "Failed to retrieve vault path for resource.")).ok() ) {
       
                // =-=-=-=-=-=-=-
                // append the hash to the path as the new 'cache file name'
                path += "/";
                path += fco->physical_path().c_str();
        
                int status = mockArchiveCopyPlugin( fco->mode(), fco->physical_path().c_str(), _cache_file_name );
                result = ASSERT_ERROR(status >= 0, status, "Failed copying archive file: \"%s\" to cache file: \"%s\".",
                                      fco->physical_path().c_str(), _cache_file_name);
            }
        }
        
        return result;
    } // mock_archive_stagetocache_plugin

    // =-=-=-=-=-=-=-
    // unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error mock_archive_synctoarch_plugin( 
        eirods::resource_plugin_context& _ctx,
        char*                            _cache_file_name )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // Check the operation parameters and update the physical path
        eirods::error ret = unix_check_params_and_path< eirods::file_object >( _ctx );
        if((result = ASSERT_PASS(ret, "Invalid plugin context.")).ok()) {
        
            // =-=-=-=-=-=-=-
            // get ref to fco
            eirods::file_object_ptr fco = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );
       
            // =-=-=-=-=-=-=-
            // hash the physical path to reflect object store behavior
            MD5_CTX context;
            char md5Buf[ MAX_NAME_LEN ];
            unsigned char hash  [ MAX_NAME_LEN ];

            strncpy( md5Buf, fco->physical_path().c_str(), fco->physical_path().size() );
            MD5Init( &context );
            MD5Update( &context, (unsigned char*)md5Buf, fco->physical_path().size() );
            MD5Final( (unsigned char*)hash, &context );
       

            std::stringstream ins;
            for(int i = 0; i < 16; ++i) {
                ins << std::setfill('0') << std::setw(2) << std::hex << (int)hash[i];
            }

            // =-=-=-=-=-=-=-
            // get the vault path for the resource
            std::string path;
            ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_PATH, path ); 
            if((result = ASSERT_PASS(ret, "Failed to get vault path for resource.")).ok() ) {
       
                // =-=-=-=-=-=-=-
                // append the hash to the path as the new 'cache file name'
                path += "/";
                path += ins.str();

                rodsLog( LOG_NOTICE, "mock archive :: cache file name [%s]", _cache_file_name );

                rodsLog( LOG_NOTICE, "mock archive :: new hashed file name for [%s] is [%s]",
                         fco->physical_path().c_str(), path.c_str() );
        
                // =-=-=-=-=-=-=-
                // make the copy to the 'archive'
                int status = mockArchiveCopyPlugin( fco->mode(), _cache_file_name, path.c_str() );
                if((result = ASSERT_ERROR( status >= 0, status, "Sync to arch failed.")).ok()) {
                    fco->physical_path( ins.str() );
                }
            }
        }

        return result;
    } // mock_archive_synctoarch_plugin

    // =-=-=-=-=-=-=-
    // redirect_create - code to determine redirection for get operation
    // Create never gets called on an archive.
    
    // =-=-=-=-=-=-=-
    // redirect_get - code to determine redirection for get operation
    eirods::error mock_archive_redirect_open( 
        eirods::plugin_property_map& _prop_map,
        eirods::file_object_ptr         _file_obj,
        const std::string&           _resc_name, 
        const std::string&           _curr_host, 
        float&                       _out_vote )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // initially set a good default
        _out_vote = 0.0;

        // =-=-=-=-=-=-=-
        // determine if the resource is down 
        int resc_status = 0;
        eirods::error get_ret = _prop_map.get< int >( eirods::RESOURCE_STATUS, resc_status );
        if((result = ASSERT_PASS(get_ret, "Failed to get \"status\" property.")).ok() ) {

            // =-=-=-=-=-=-=-
            // if the status is down, vote no.
            if( INT_RESC_STATUS_DOWN != resc_status ) {

                // =-=-=-=-=-=-=-
                // get the resource host for comparison to curr host
                std::string host_name;
                get_ret = _prop_map.get< std::string >( eirods::RESOURCE_LOCATION, host_name );
                if((result = ASSERT_PASS(get_ret, "Failed to get \"location\" property.")).ok() ) {

                    // =-=-=-=-=-=-=-
                    // set a flag to test if were at the curr host, if so we vote higher
                    bool curr_host = ( _curr_host == host_name );

                    // =-=-=-=-=-=-=-
                    // make some flags to clairify decision making
                    bool need_repl = ( _file_obj->repl_requested() > -1 );

                    // =-=-=-=-=-=-=-
                    // set up variables for iteration
                    bool          found     = false;
                    std::vector< eirods::physical_object > objs = _file_obj->replicas();
                    std::vector< eirods::physical_object >::iterator itr = objs.begin();
        
                    // =-=-=-=-=-=-=-
                    // check to see if the replica is in this resource, if one is requested
                    for( ; !found && itr != objs.end(); ++itr ) {

                        // =-=-=-=-=-=-=-
                        // run the hier string through the parser and get the last
                        // entry.
                        std::string last_resc;
                        eirods::hierarchy_parser parser;
                        parser.set_string( itr->resc_hier() );
                        parser.last_resc( last_resc ); 
          
                        // =-=-=-=-=-=-=-
                        // more flags to simplify decision making
                        bool repl_us = ( _file_obj->repl_requested() == itr->repl_num() ); 
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
                            }

                        } // if resc_us

                    } // for itr
                }
            }
        }
        
        return result;

    } // mock_archive_redirect_open

    // =-=-=-=-=-=-=-
    // used to allow the resource to determine which host
    // should provide the requested operation
    eirods::error mock_archive_redirect_plugin( 
        eirods::resource_plugin_context& _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        eirods::hierarchy_parser*           _out_parser,
        float*                              _out_vote )
    {
        eirods::error result = SUCCESS();
        
        // =-=-=-=-=-=-=-
        // check the context validity
        eirods::error ret = _ctx.valid< eirods::file_object >(); 
        if((result = ASSERT_PASS(ret, "Invalid plugin context.")).ok()) {

            if((result = ASSERT_ERROR(_opr && _curr_host && _out_parser && _out_vote, SYS_INVALID_INPUT_PARAM,
                                      "Invalid input parameters.")).ok()) {

                // =-=-=-=-=-=-=-
                // cast down the chain to our understood object type
                eirods::file_object_ptr file_obj = boost::dynamic_pointer_cast< eirods::file_object >( _ctx.fco() );

                // =-=-=-=-=-=-=-
                // get the name of this resource
                std::string resc_name;
                ret = _ctx.prop_map().get< std::string >( eirods::RESOURCE_NAME, resc_name );
                if((result = ASSERT_PASS(ret, "Failed to get property for resource name.")).ok() ) {

                    // =-=-=-=-=-=-=-
                    // add ourselves to the hierarchy parser by default
                    _out_parser->add_child( resc_name );

                    // =-=-=-=-=-=-=-
                    // test the operation to determine which choices to make
                    if( eirods::EIRODS_OPEN_OPERATION == (*_opr) ) {
                        // =-=-=-=-=-=-=-
                        // call redirect determination for 'get' operation
                        result = mock_archive_redirect_open( _ctx.prop_map(), file_obj, resc_name, (*_curr_host), (*_out_vote) );

                    } else if( eirods::EIRODS_CREATE_OPERATION == (*_opr) ) {
                        // =-=-=-=-=-=-=-
                        // call redirect determination for 'create' operation
                        result = ASSERT_ERROR(false, SYS_INVALID_INPUT_PARAM, "Create operation not supported for an archive");
                    }
                    else {
                        
                        // =-=-=-=-=-=-=-
                        // must have been passed a bad operation
                        result = ASSERT_ERROR(false, SYS_INVALID_INPUT_PARAM, "Operation not supported: \"%s\".",
                                              _opr->c_str());
                    }
                }
            }
        }

        return result;
    } // mock_archive_redirect_plugin

    // =-=-=-=-=-=-=-
    // mock_archive_file_rebalance - code which would rebalance the subtree
    eirods::error mock_archive_file_rebalance(
        eirods::resource_plugin_context& _ctx ) {
        return SUCCESS();

    } // mock_archive_file_rebalancec

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle mock_archive file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class mockarchive_resource : public eirods::resource {
        // =-=-=-=-=-=-=-
        // 3a. create a class to provide maintenance operations, this is only for example
        //     and will not be called.
        class maintenance_operation {
        public:
            maintenance_operation( const std::string& _n ) : name_(_n) {
            }

            maintenance_operation( const maintenance_operation& _rhs ) {
                name_ = _rhs.name_;    
            }

            maintenance_operation& operator=( const maintenance_operation& _rhs ) {
                name_ = _rhs.name_;    
                return *this;
            }

            eirods::error operator()( rcComm_t* ) {
                rodsLog( LOG_NOTICE, "mockarchive_resource::post_disconnect_maintenance_operation - [%s]", 
                         name_.c_str() );
                return SUCCESS();
            }

        private:
            std::string name_;

        }; // class maintenance_operation

    public:
        mockarchive_resource( const std::string& _inst_name, 
                              const std::string& _context ) : 
            eirods::resource( _inst_name, _context ) {
        } // ctor


        eirods::error need_post_disconnect_maintenance_operation( bool& _b ) {
            _b = false;
            return SUCCESS();
        }


        // =-=-=-=-=-=-=-
        // 3b. pass along a functor for maintenance work after
        //     the client disconnects, uncomment the first two lines for effect.
        eirods::error post_disconnect_maintenance_operation( eirods::pdmo_type& _op  ) {
#if 0
            std::string name;
            eirods::error err = get_property< std::string >( eirods::RESOURCE_NAME, name );
            if( !err.ok() ) {
                return PASSMSG( "mockarchive_resource::post_disconnect_maintenance_operation failed.", err );
            }

            _op = maintenance_operation( name );
            return SUCCESS();
#else
            return ERROR( -1, "nop" );
#endif
        }

    }; // class mockarchive_resource
  
    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create mockarchive_resource
        mockarchive_resource* resc = new mockarchive_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of 
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "mock_archive_unlink_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "mock_archive_stagetocache_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "mock_archive_synctoarch_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "mock_archive_redirect_plugin" );
        resc->add_operation( eirods::RESOURCE_OP_REBALANCE,             "mock_archive_file_rebalance" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( eirods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( eirods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



