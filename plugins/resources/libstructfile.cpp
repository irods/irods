/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_collection_object.h"
#include "eirods_structured_object.h"
#include "eirods_string_tokenize.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

// =-=-=-=-=-=-=-
// boost includes
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

// =-=-=-=-=-=-=-
// system includes
#include "archive.h"
#include "archive_entry.h"

// =-=-=-=-=-=-=-
// structures and defines
typedef struct structFileDesc {
    int inuseFlag;
    rsComm_t *rsComm;
    specColl_t *specColl;
    rescInfo_t *rescInfo;
    int openCnt;
	char dataType[NAME_LEN]; // JMC - backport 4634
} structFileDesc_t;

#define CACHE_DIR_STR "cacheDir"

typedef struct tarSubFileDesc {
    int inuseFlag;
    int structFileInx;
    int fd;                         /* the fd of the opened cached subFile */
    char cacheFilePath[MAX_NAME_LEN];   /* the phy path name of the cached
                                         * subFile */
} tarSubFileDesc_t;

#define NUM_TAR_SUB_FILE_DESC 20

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.h"
#include "modColl.h"
#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "collection.h"
#include "specColl.h"
#include "resource.h"
#include "miscServerFunct.h"
#include "physPath.h"
#include "fileOpen.h"


// =-=-=-=-=-=-=-=-
// file descriptor tables which hold references to struct files
// which are currently in flight
const int NUM_STRUCT_FILE_DESC = 16;
structFileDesc_t PluginStructFileDesc[ NUM_STRUCT_FILE_DESC  ];
tarSubFileDesc_t PluginTarSubFileDesc[ NUM_TAR_SUB_FILE_DESC ];

// =-=-=-=-=-=-=-=-
// manager of resource plugins which are resolved and cached
extern eirods::resource_manager resc_mgr;

extern "C" {

    // =-=-=-=-=-=-=-
    // start operation used to allocate the FileDesc tables
    void tarfilesystem_resource_start() {
        memset( PluginStructFileDesc, 0, sizeof(structFileDesc_t) * NUM_STRUCT_FILE_DESC  );
        memset( PluginTarSubFileDesc, 0, sizeof(tarSubFileDesc_t) * NUM_TAR_SUB_FILE_DESC );
    }

    // =-=-=-=-=-=-=-
    // stop operation used to free the FileDesc tables
    void tarfilesystem_resource_stop() {
        memset( PluginStructFileDesc, 0, sizeof(structFileDesc_t) * NUM_STRUCT_FILE_DESC  );
        memset( PluginTarSubFileDesc, 0, sizeof(tarSubFileDesc_t) * NUM_TAR_SUB_FILE_DESC );
    }

    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;

    // =-=-=-=-=-=-=-
    // 2. Define operations which will be called by the file*
    //    calls declared in server/driver/include/fileDriver.h
    // =-=-=-=-=-=-=-

    // =-=-=-=-=-=-=-
    // NOTE :: to access properties in the _prop_map do the 
    //      :: following :
    //      :: double my_var = 0.0;
    //      :: eirods::error ret = _prop_map.get< double >( "my_key", my_var ); 
    // =-=-=-=-=-=-=-
	
    // =-=-=-=-=-=-=-
    // helper function to check incoming parameters
    inline eirods::error param_check( eirods::resource_property_map* 
                                      _prop_map,
                                      eirods::resource_child_map* 
                                      _cmap, 
                                      eirods::first_class_object* 
                                      _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        bool status = true;
        std::string msg = "tarFileCreatePlugin - ";

        if( !_prop_map ) {
            status = false;
            msg += "null resource_property_map";
        }
        if( !_cmap ) {
            status = false;
            if( !status ) 
                msg += ", ";
            msg += "null resource_child_map";
        }
        if( !_object ) {
            status = false;
            if( !status ) 
                msg += ", ";
            msg += "null first_class_object";
        }

        if( status ) {
            return SUCCESS();
        } else {
            return ERROR( -1, msg );
        }

    } // param_check

    // =-=-=-=-=-=-=-
    // call archive file extraction for struct file 
    eirods::error extract_file( int _index ) {
        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        if( PluginStructFileDesc[ _index ].inuseFlag <= 0 ) {
            std::stringstream msg;
            msg << "extract_file - struct file index: ";
            msg << _index;
            msg << " is not in use";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        if( spec_coll == NULL                  || 
            strlen( spec_coll->cacheDir ) == 0 ||
            strlen( spec_coll->phyPath  ) == 0) {
            std::stringstream msg;
            msg << "extract_file - bad special collection for index: ";
            msg << _index;
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // select which attributes we want to restore
        int flags = ARCHIVE_EXTRACT_TIME;
        //flags |= ARCHIVE_EXTRACT_PERM;
        //flags |= ARCHIVE_EXTRACT_ACL;
        //flags |= ARCHIVE_EXTRACT_FFLAGS;

        // =-=-=-=-=-=-=-
        // initialize archive struct and set flags for format etc
        struct archive* arch = archive_read_new();
        archive_read_support_compression_all( arch );
        archive_read_support_format_all( arch );
        archive_read_support_filter_all( arch );

        // =-=-=-=-=-=-=-
        // open the archive and and prepare to read
        if( archive_read_open_filename( arch, spec_coll->phyPath, 16384 ) != ARCHIVE_OK ) {
            std::stringstream msg;
            msg << "extract_file - failed to open archive [";
            msg << spec_coll->phyPath;
            msg << "]";
            return ERROR( -1, msg.str() );
        }
 
        // =-=-=-=-=-=-=-
        // build a cache directory string
        std::string cache_dir( spec_coll->cacheDir );
        if( cache_dir[ cache_dir.size() - 1 ] != '/' ) {
            cache_dir += "/";
        }

        // =-=-=-=-=-=-=-
        // iterate over entries in the archive and write them do disk
        struct archive_entry* entry;
        while( ARCHIVE_OK == archive_read_next_header( arch, &entry ) ) {
             // =-=-=-=-=-=-=-
             // redirect the path to the cache directory
             std::string path = cache_dir + std::string( archive_entry_pathname( entry ) );
             archive_entry_set_pathname( entry, path.c_str() );

             // =-=-=-=-=-=-=-
             // read data from entry and write it to disk
             if( ARCHIVE_OK != archive_read_extract( arch, entry, flags ) ){
                 std::stringstream msg;
                 msg << "extract_file - failed to write [";
                 msg << path;
                 msg << "]"; 
                 rodsLog( LOG_NOTICE, msg.str().c_str() ); 
             } 

        } // while

        // =-=-=-=-=-=-=-
        // release the archive back into the wild
        archive_read_free( arch );

        return SUCCESS();

    } // extract_file
    
    // =-=-=-=-=-=-=-
    // recursively create a cache directory for a spec coll via irods api 
    eirods::error make_tar_cache_dir( int _index, std::string _host ) {
        // =-=-=-=-=-=-=-
        // extract and test comm pointer
        rsComm_t* rs_comm = PluginStructFileDesc[ _index ].rsComm;
        if( !rs_comm ) {
            std::stringstream msg;
            msg << "make_tar_cache_dir - null rsComm pointer for index: ";
            msg << _index;
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg.str() );
        }
        
        // =-=-=-=-=-=-=-
        // extract and test special collection pointer
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        if( !spec_coll ) {
            std::stringstream msg;
            msg << "make_tar_cache_dir - null specColl pointer for index: ";
            msg << _index;
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // construct a mkdir structure
        fileMkdirInp_t fileMkdirInp;
        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        fileMkdirInp.fileType = UNIX_FILE_TYPE;	  // the only type for cache
        fileMkdirInp.mode     = DEFAULT_DIR_MODE;
        strncpy( fileMkdirInp.addr.hostAddr, const_cast<char*>( _host.c_str() ), NAME_LEN ); 

        // =-=-=-=-=-=-=-
        // loop over a series of indicies for the directory suffix and
        // try to make the directory until it succeeds
        int dir_index = 0;
        while( true ) {
            // =-=-=-=-=-=-=-
            // build the cache directory name
            snprintf( fileMkdirInp.dirName, MAX_NAME_LEN, "%s.%s%d",
                      spec_coll->phyPath,    CACHE_DIR_STR, dir_index );

            // =-=-=-=-=-=-=-
            // system api call to create a directory
            int status = rsFileMkdir( rs_comm, &fileMkdirInp );
            if( status >= 0 ) {
                break;

            } else {
                if( getErrno( status ) == EEXIST ) {
                    dir_index ++;
                    continue;

                } else {
                    return ERROR( status, "make_tar_cache_dir - failed to create cache directory" );

                } // else

            } // else

        } // while
        
        // =-=-=-=-=-=-=-
        // copy successful cache dir out of mkdir struct
        strncpy( spec_coll->cacheDir, fileMkdirInp.dirName, MAX_NAME_LEN );

        // =-=-=-=-=-=-=-
        // Win!
        return SUCCESS();

    } // make_tar_cache_dir
        
    // =-=-=-=-=-=-=-
    // extract the tar file into a cache dir
    eirods::error stage_tar_struct_file( int _index, std::string _host ) {
        int status = -1;

        // =-=-=-=-=-=-=-
        // extract the special collection from the struct file table
        specColl_t* spec_coll = PluginStructFileDesc[_index].specColl;
        if( spec_coll == NULL ) {
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, "" );            
        }

        // =-=-=-=-=-=-=-
        // check to see if we have a cache dir and make one if not
        if( strlen( spec_coll->cacheDir ) == 0 ) {
            // =-=-=-=-=-=-=-
            // no cache. stage one. make the CacheDir first
            eirods::error mk_err = make_tar_cache_dir( _index, _host );
            if( !mk_err.ok() ) {
                return PASS( false, mk_err.code(), "stage_tar_struct_file - failed to create cachedir", mk_err );
            }

            // =-=-=-=-=-=-=-
            // expand tar file into cache dir
            eirods::error extract_err = extract_file( _index );
            if( !extract_err.ok() ) {
                std::stringstream msg;
                msg << "stage_tar_struct_file - extract_file failed for [";
                msg << spec_coll->objPath;
                msg << "] in cache directory [";
                msg << spec_coll->cacheDir;

                /* XXXX may need to remove the cacheDir too */
                return PASS( false, SYS_TAR_STRUCT_FILE_EXTRACT_ERR - errno, msg.str(), extract_err ); 

            } // if !ok

            // =-=-=-=-=-=-=-
            // register the CacheDir 
            status = modCollInfo2( PluginStructFileDesc[ _index ].rsComm, spec_coll, 0 );
            if( status < 0 ) {
                return ERROR( status, "stage_tar_struct_file - modCollInfo2 failed." ); 
            }

        } // if empty cachedir  

        return SUCCESS();

    } // stage_tar_struct_file
  
    // =-=-=-=-=-=-=-
    // find the next free PluginStructFileDesc slot, mark it in use and return the index
    int alloc_struct_file_desc() {
        for( int i = 1; i < NUM_STRUCT_FILE_DESC; i++ ) {
            if (PluginStructFileDesc[i].inuseFlag == FD_FREE) {
                PluginStructFileDesc[i].inuseFlag = FD_INUSE;
                return (i);
            };
        } // for i

        return (SYS_OUT_OF_FILE_DESC);

    } // alloc_struct_file_desc

    int free_struct_file_desc( int _idx ) {
        if( _idx  < 0 || _idx  >= NUM_STRUCT_FILE_DESC ) {
            rodsLog( LOG_NOTICE,
             "free_struct_file_desc: index %d out of range", _idx );
            return (SYS_FILE_DESC_OUT_OF_RANGE);
        }

        memset( &PluginStructFileDesc[ _idx ], 0, sizeof( structFileDesc_t ) );

        return (0);
    
    } // free_struct_file_desc

    // =-=-=-=-=-=-=-
    //  
    int match_struct_file_desc( specColl_t* _spec_coll ) {

        for( int i = 1; i < NUM_STRUCT_FILE_DESC; i++) {
            if( PluginStructFileDesc[i].inuseFlag == FD_INUSE &&
                PluginStructFileDesc[i].specColl  != NULL     &&
                strcmp( PluginStructFileDesc[i].specColl->collection, _spec_coll->collection ) == 0 &&
                strcmp( PluginStructFileDesc[i].specColl->objPath,    _spec_coll->objPath    ) == 0 ) {
                return (i);
            };
        } // for

        return (SYS_OUT_OF_FILE_DESC);

    } // match_struct_file_desc

    // =-=-=-=-=-=-=-
    // local function to manage the open of a tar file 
    eirods::error tar_struct_file_open( rsComm_t*    _comm, 
                                        specColl_t*  _spec_coll, 
                                        int&         _struct_desc_index,
                                        std::string& _resc_host ) {
        int status                  = 0;
        specCollCache_t* spec_cache = 0;

        // =-=-=-=-=-=-=-
        // trap null parameters
        if( 0 == _spec_coll ) {
            std::string msg( "tar_struct_file_open - null special collection parameter" );
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg );
        }

        if( 0 == _comm ) {
            std::string msg( "tar_struct_file_open - null rsComm_t parameter" );
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg );
        }

        // =-=-=-=-=-=-=-
        // look for opened PluginStructFileDesc
        _struct_desc_index = match_struct_file_desc( _spec_coll );
        if( _struct_desc_index > 0 ) {
            return SUCCESS();
        }
    
        // =-=-=-=-=-=-=-
        // alloc and trap bad alloc
        if( ( _struct_desc_index = alloc_struct_file_desc() ) < 0 ) {
            return ERROR( _struct_desc_index, "tar_struct_file_open - call to allocStructFileDesc failed." );
        }
                 
        // =-=-=-=-=-=-=-
        // [ mwan? :: Have to do this because  _spec_coll could come from a remote host ]
        // NOTE :: i dont see any remote server to server comms here
        if( ( status = getSpecCollCache( _comm,  _spec_coll->collection, 0, &spec_cache ) ) >= 0 ) {
            // =-=-=-=-=-=-=-
            // copy pointer to cached special collection
            PluginStructFileDesc[ _struct_desc_index ].specColl = &spec_cache->specColl;
            if( !PluginStructFileDesc[ _struct_desc_index ].specColl ) {
                
            }

            // =-=-=-=-=-=-=-
            // copy over physical path and resource name since getSpecCollCache 
            // does not give phyPath nor resource 
            if( strlen(  _spec_coll->phyPath ) > 0 ) { // JMC - backport 4517
                rstrcpy( spec_cache->specColl.phyPath,  _spec_coll->phyPath, MAX_NAME_LEN );
            }
            if( strlen( spec_cache->specColl.resource ) == 0 ) {
                rstrcpy( spec_cache->specColl.resource,  _spec_coll->resource, NAME_LEN );
            }
        } else {
            // =-=-=-=-=-=-=-
            // special collection is local to this server ??
            PluginStructFileDesc[ _struct_desc_index ].specColl =  _spec_coll;
        }

        // =-=-=-=-=-=-=-
        // cache pointer to comm struct
        PluginStructFileDesc[ _struct_desc_index ].rsComm = _comm;

        // =-=-=-=-=-=-=-
        // resolve resource by name
        eirods::resource_ptr resc;
        eirods::error resc_err = resc_mgr.resolve( PluginStructFileDesc[ _struct_desc_index ].specColl->resource, resc );
        if( !resc_err.ok() ) {
            std::stringstream msg;
            msg << "tar_struct_file_open - error returned from resolveResc for resource [";
            msg << _spec_coll->resource;
            msg << "], status: ";
            msg << resc_err.code();
            free_struct_file_desc( _struct_desc_index );
            return PASS( false, _struct_desc_index, msg.str(), resc_err );
        }

        // =-=-=-=-=-=-=-
        // extract the name of the host of the resource from the resource plugin
        rodsServerHost_t* rods_host; 
        eirods::error get_err = resc->get_property< rodsServerHost_t* >( "host", rods_host );
        if( !get_err.ok() ) {
            return PASS( false, -1, "failed to call get_property", get_err );
        }
        
        if( !rods_host->hostName ) {
            return ERROR( -1, "null rods server host" );
        }
        
        _resc_host = rods_host->hostName->name;

        // =-=-=-=-=-=-=-
        // TODO :: need to deal with remote open here 

        // =-=-=-=-=-=-=-
        // stage the tar file so we can get at its tasty innards
        eirods::error stage_err = stage_tar_struct_file( _struct_desc_index, _resc_host );
        if( !stage_err.ok() ) {
            free_struct_file_desc( _struct_desc_index );
            return PASS( false, _struct_desc_index, "tar_struct_file_open - stage_tar_struct_file failed.", stage_err );
        }

        // =-=-=-=-=-=-=-
        // Win!
        return CODE( _struct_desc_index );

    } // tar_struct_file_open

    // =-=-=-=-=-=-=-
    // create the phy path to the cache dir
    eirods::error compose_cache_dir_physical_path( char*       _phy_path, 
                                                   specColl_t* _spec_coll, 
                                                   const char* _sub_file_path ) {
        // =-=-=-=-=-=-=- 
        // subFilePath is composed by appending path to the objPath which is
        // the logical path of the tar file. So we need to substitude the
        // objPath with cacheDir 
        int len = strlen( _spec_coll->collection );
        if( strncmp( _spec_coll->collection, _sub_file_path, len ) != 0 ) {
            std::stringstream msg;
            msg << "compose_cache_dir_physical_path - collection [";
            msg << _spec_coll->collection;
            msg << "] sub file path [";
            msg << _sub_file_path;
            msg << "] mismatch";
            return ERROR( SYS_STRUCT_FILE_PATH_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=- 
        // compose the path
        snprintf( _phy_path, MAX_NAME_LEN, "%s%s", _spec_coll->cacheDir, _sub_file_path + len );
        
        // =-=-=-=-=-=-=- 
        // Win!
        return SUCCESS();

    } // compose_cache_dir_physical_path
    
	// =-=-=-=-=-=-=-
	// assign an new entry in the tar desc table
    int alloc_tar_sub_file_desc() {
        for( int i = 1; i < NUM_TAR_SUB_FILE_DESC; i++ ) {
            if (PluginTarSubFileDesc[i].inuseFlag == FD_FREE) {
                PluginTarSubFileDesc[i].inuseFlag = FD_INUSE;
                return (i);
            };
        }

        rodsLog (LOG_NOTICE,
         "alloc_tar_sub_file_desc: out of PluginTarSubFileDesc");

        return (SYS_OUT_OF_FILE_DESC);

    } // alloc_tar_sub_file_desc

	// =-=-=-=-=-=-=-
	// free an entry in the tar desc table
    int free_tar_sub_file_desc( int _idx ) {
        if( _idx < 0 || _idx >= NUM_TAR_SUB_FILE_DESC ) {
            rodsLog (LOG_NOTICE,
             "free_tar_sub_file_desc: index %d out of range", _idx );
            return (SYS_FILE_DESC_OUT_OF_RANGE);
        }

        memset( &PluginTarSubFileDesc[ _idx ], 0, sizeof( tarSubFileDesc_t ) );

        return (0);
    }

	// =-=-=-=-=-=-=-
	// interface for POSIX create
    eirods::error tarFileCreatePlugin( rsComm_t*                      _comm,
                                       const std::string&             _results,
                                       eirods::resource_property_map* 
                                       _prop_map,
                                       eirods::resource_child_map* 
                                       _cmap, 
                                       eirods::first_class_object* 
										                    _object ) {
		// =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileCreatePlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileCreatePlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileCreatePlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err = tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileCreatePlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( sub_index, "tarFileCreatePlugin - alloc_tar_sub_file_desc failed." );
        }

        // =-=-=-=-=-=-=-
        // cache struct file index into sub file index
        PluginTarSubFileDesc[ sub_index ].structFileInx = struct_file_index;
        
        // =-=-=-=-=-=-=-
        // build a file create structure to pass off to the server api call
        fileCreateInp_t fileCreateInp;
        memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileCreateInp.fileName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileCreatePlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        fileCreateInp.mode       = struct_obj->mode();
        fileCreateInp.flags      = struct_obj->flags();
        fileCreateInp.fileType   = UNIX_FILE_TYPE;	/* the only type for cache */
        fileCreateInp.otherFlags = NO_CHK_PERM_FLAG; // JMC - backport 4768
        strncpy( fileCreateInp.addr.hostAddr, resc_host.c_str(), NAME_LEN );

        // =-=-=-=-=-=-=-
        // make the call to create a file
        int status = rsFileCreate( comm, &fileCreateInp );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "tarFileCreatePlugin - rsFileCreate failed for [";
            msg << fileCreateInp.fileName;
            msg << "], status = ";
            msg << status;
            return ERROR( status, msg.str() );
        } else {
            PluginTarSubFileDesc[ sub_index ].fd = status;
            PluginStructFileDesc[ struct_file_index ].openCnt++;
            _object->file_descriptor( sub_index );
            return CODE(  sub_index  );
        }

    } // tarFileCreatePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error tarFileOpenPlugin( rsComm_t*                      _comm,
                                     const std::string&             _results,
                                     eirods::resource_property_map* 
                                     _prop_map, 
                                     eirods::resource_child_map* 
                                     _cmap, 
                                     eirods::first_class_object* 
                                     _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileOpenPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::first_class_object* hackPtr = _object;
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( hackPtr );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileOpenPlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileOpenPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileOpenPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( sub_index, "tarFileOpenPlugin - alloc_tar_sub_file_desc failed." );
        }

        // =-=-=-=-=-=-=-
        // cache struct file index into sub file index
        PluginTarSubFileDesc[ sub_index ].structFileInx = struct_file_index;
        
        // =-=-=-=-=-=-=-
        // build a file open structure to pass off to the server api call
        fileOpenInp_t fileOpenInp;
        memset( &fileOpenInp, 0, sizeof( fileOpenInp ) );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileOpenInp.fileName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileOpenPlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        fileOpenInp.mode       = struct_obj->mode();
        fileOpenInp.flags      = struct_obj->flags();
        fileOpenInp.fileType   = UNIX_FILE_TYPE;	/* the only type for cache */
        fileOpenInp.otherFlags = NO_CHK_PERM_FLAG; // JMC - backport 4768
        strncpy( fileOpenInp.addr.hostAddr,
                 resc_host.c_str(),
                 NAME_LEN );

        // =-=-=-=-=-=-=-
        // make the call to create a file
        int status = rsFileOpen( comm, &fileOpenInp );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "tarFileOpenPlugin - rsFileOpen failed for [";
            msg << fileOpenInp.fileName;
            msg << "], status = ";
            msg << status;
            return ERROR( status, msg.str() );
        } else {
            PluginTarSubFileDesc[ sub_index ].fd = status;
            PluginStructFileDesc[ struct_file_index ].openCnt++;
            _object->file_descriptor( sub_index );
            return CODE(  sub_index  );
        }

    } // tarFileOpenPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error tarFileReadPlugin( rsComm_t*                      _comm,
                                     const std::string&             _results,
                                     eirods::resource_property_map* 
                                     _prop_map, 
                                     eirods::resource_child_map* 
                                     _cmap,
                                     eirods::first_class_object* 
                                     _object,
                                     void*               _buf, 
                                     int                 _len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileReadPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if( _object->file_descriptor() < 1                      || 
            _object->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
            PluginTarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a read structure and make the rs call
        fileReadInp_t fileReadInp;
        bytesBuf_t fileReadOutBBuf;
        memset (&fileReadInp, 0, sizeof (fileReadInp));
        memset (&fileReadOutBBuf, 0, sizeof (fileReadOutBBuf));
        fileReadInp.fileInx = PluginTarSubFileDesc[ _object->file_descriptor() ].fd;
        fileReadInp.len     = _len;
        fileReadOutBBuf.buf = _buf;

        // =-=-=-=-=-=-=-
        // make the call to read a file
        int status = rsFileRead( _object->comm(), &fileReadInp, &fileReadOutBBuf );
        return CODE(status);

    } // tarFileReadPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error tarFileWritePlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map*
                                      _cmap,
                                      eirods::first_class_object* 
                                      _object,
                                      void*               _buf, 
                                      int                 _len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileWritePlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if( _object->file_descriptor() < 1                      || 
            _object->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
            PluginTarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileWritePlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a write structure and make the rs call
        fileWriteInp_t fileWriteInp;
        bytesBuf_t     fileWriteOutBBuf;
        memset( &fileWriteInp,     0, sizeof (fileWriteInp) );
        memset( &fileWriteOutBBuf, 0, sizeof (fileWriteOutBBuf) );
        fileWriteInp.len     = fileWriteOutBBuf.len = _len;
        fileWriteInp.fileInx = PluginTarSubFileDesc[ _object->file_descriptor() ].fd;
        fileWriteOutBBuf.buf = _buf;

        // =-=-=-=-=-=-=-
        // make the write api call
        int status = rsFileWrite( _object->comm(), &fileWriteInp, &fileWriteOutBBuf );
        if( status > 0 ) {
            // =-=-=-=-=-=-=-
            // cache has been written 
            int         struct_idx = PluginTarSubFileDesc[ _object->file_descriptor() ].structFileInx;
            specColl_t* spec_coll   = PluginStructFileDesc[ struct_idx ].specColl;
            if( spec_coll->cacheDirty == 0 ) {
                spec_coll->cacheDirty = 1;    
                int status1 = modCollInfo2( _object->comm(), spec_coll, 0 );
                if( status1 < 0 ) 
                    return CODE( status1 );
            }
        }

        return CODE( status );

    } // tarFileWritePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error tarFileClosePlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object* 
                                      _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileClosePlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if( _object->file_descriptor() < 1                      || 
            _object->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
            PluginTarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileClosePlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a close structure and make the rs call
        fileCloseInp_t fileCloseInp;
        fileCloseInp.fileInx = PluginTarSubFileDesc[ _object->file_descriptor() ].fd;
        int status = rsFileClose( _object->comm(), &fileCloseInp );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "tarFileClosePlugin - failed in rsFileClose for fd [ ";
            msg << _object->file_descriptor();
            msg << " ]";
            return ERROR( status, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // close out the sub file allocation and free the space
        int struct_file_index = PluginTarSubFileDesc[ _object->file_descriptor() ].structFileInx;
        PluginStructFileDesc[ struct_file_index ].openCnt++;
        free_tar_sub_file_desc( _object->file_descriptor() );
        _object->file_descriptor( 0 );

        return CODE( status );

    } // tarFileClosePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error tarFileUnlinkPlugin( rsComm_t*                      _comm,
                                       const std::string&             _results,
                                       eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object* 
                                       _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileUnlinkPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileUnlinkPlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileUnlinkPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileUnlinkPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // build a file unlink structure to pass off to the server api call
        fileUnlinkInp_t fileUnlinkInp;
        memset( &fileUnlinkInp, 0, sizeof( fileUnlinkInp ) );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileUnlinkInp.fileName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileUnlinkPlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        fileUnlinkInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        strncpy( fileUnlinkInp.addr.hostAddr, resc_host.c_str(), NAME_LEN );

        // =-=-=-=-=-=-=-
        // make the call to unlink a file
        int status = rsFileUnlink( comm, &fileUnlinkInp );
        if( status >= 0 ) {
            specColl_t* specColl;
            /* cache has been written */
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            if (loc_spec_coll->cacheDirty == 0) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2 ( comm, loc_spec_coll, 0 );
                if( status1 < 0 ) 
                    return CODE( status1 );
            }
        }

        return SUCCESS();

    } // tarFileUnlinkPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error tarFileStatPlugin( rsComm_t*                      _comm,
                                     const std::string&             _results,
                                     eirods::resource_property_map* 
                                     _prop_map, 
                                     eirods::resource_child_map* 
                                     _cmap,
                                     eirods::first_class_object* 
                                     _object,
                                     struct stat*        _statbuf ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileStatPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileStatPlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileStatPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileStatPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

    
        // =-=-=-=-=-=-=-
        // build a file stat structure to pass off to the server api call
        fileStatInp_t fileStatInp;
        memset( &fileStatInp, 0, sizeof ( fileStatInp ) );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileStatInp.fileName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileStatPlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        fileStatInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        strncpy( fileStatInp.addr.hostAddr, resc_host.c_str(), NAME_LEN );

        // =-=-=-=-=-=-=-
        // make the call to stat a file
        rodsStat_t* rods_stat;
        int status = rsFileStat( comm, &fileStatInp, &rods_stat );
        if( status >= 0 ) {
            rodsStatToStat( _statbuf, rods_stat );
        } else {
            return ERROR( status, "tarFileStatPlugin - rsFileStat failed." );
        }

        return CODE( status );

    } // tarFileStatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error tarFileFstatPlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map*
                                      _cmap,
                                      eirods::first_class_object* 
                                      _object,
                                      struct stat*        _statbuf ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileFstatPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if( _object->file_descriptor() < 1                      || 
            _object->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
            PluginTarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileFstatPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileFstatPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // build a fstat structure and make the rs call
        fileFstatInp_t fileFstatInp;
        memset( &fileFstatInp, 0, sizeof( fileFstatInp ) );
        fileFstatInp.fileInx = PluginTarSubFileDesc[ _object->file_descriptor() ].fd;
        
        rodsStat_t* rods_stat;
        int status = rsFileFstat( comm, &fileFstatInp, &rods_stat );
        if( status >= 0 ) {
            rodsStatToStat( _statbuf, rods_stat );
        } else {
            return ERROR( status, "tarFileFStatPlugin - rsFileFstat failed." );
        }

        return CODE( status );

    } // tarFileFstatPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error tarFileLseekPlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object* 
                                      _object,
                                      size_t              _offset, 
                                      int                 _whence ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileLseekPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if( _object->file_descriptor() < 1                      || 
            _object->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
            PluginTarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileLseekPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileLseekPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // build a lseek structure and make the rs call
        fileLseekInp_t fileLseekInp;
        memset( &fileLseekInp, 0, sizeof( fileLseekInp ) );
        fileLseekInp.fileInx = PluginTarSubFileDesc[ _object->file_descriptor() ].fd;
        fileLseekInp.offset  = _offset;
        fileLseekInp.whence  = _whence;
        
        fileLseekOut_t *fileLseekOut = NULL;
        int status = rsFileLseek( comm, &fileLseekInp, &fileLseekOut );

        if( status < 0 || NULL == fileLseekOut ) { // JMC cppcheck - nullptr
            return CODE( status );
        } else {
            rodsLong_t offset = fileLseekOut->offset;
            free( fileLseekOut );
            return CODE( offset );
        }

    } // tarFileLseekPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error tarFileFsyncPlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object*
                                      _object ) {
        // =-=-=-=-=-=-=-
        // Not Implemented for this plugin
        return ERROR( -1, "tarFileFsyncPlugin is not implemented." );

    } // tarFileFsyncPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error tarFileMkdirPlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map*
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object*
                                      _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileMkdirPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileMkdirPlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileMkdirPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileMkdirPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // build a file mkdir structure to pass off to the server api call
        fileMkdirInp_t fileMkdirInp;
        fileMkdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        strncpy( fileMkdirInp.addr.hostAddr, resc_host.c_str(), NAME_LEN );
        fileMkdirInp.mode = struct_obj->mode();

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileMkdirInp.dirName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileMkdirPlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        // =-=-=-=-=-=-=-
        // make the call to the mkdir api
        int status = rsFileMkdir( comm, &fileMkdirInp );
        if( status >= 0 ) {
            // use the specColl in PluginStructFileDesc 
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written 
            if( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if (status1 < 0) {
                    return ERROR( status1, "tarFileMkdirPlugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

    } // tarFileMkdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error tarFileChmodPlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object*
                                      _object,
                                      int                 _mode ) {
        // =-=-=-=-=-=-=-
        // Not Implemented for this plugin
        return ERROR( -1, "tarFileChmodPlugin is not implemented." );

    } // tarFileChmodPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error tarFileRmdirPlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object*
                                      _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileRmdirPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileRmdirPlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileRmdirPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileRmdirPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // build a file mkdir structure to pass off to the server api call
        fileRmdirInp_t fileRmdirInp;
        fileRmdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        strncpy( fileRmdirInp.addr.hostAddr, resc_host.c_str(), NAME_LEN );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileRmdirInp.dirName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileRmdirPlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        // =-=-=-=-=-=-=-
        // make the call to the mkdir api
        int status = rsFileRmdir( comm, &fileRmdirInp );
        if( status >= 0 ) {
            // use the specColl in PluginStructFileDesc 
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written 
            if( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if (status1 < 0) {
                    return ERROR( status1, "tarFileRmdirPlugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

    } // tarFileRmdirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error tarFileOpendirPlugin( rsComm_t*                      _comm,
                                        const std::string&             _results,
                                        eirods::resource_property_map* 
                                        _prop_map, 
                                        eirods::resource_child_map* 
                                        _cmap,
                                        eirods::first_class_object*
                                        _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileOpendirPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "tarFileOpendirPlugin - failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileOpendirPlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileOpendirPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileOpendirPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            eirods::error ret = PASS( false, -1, msg.str(), open_err );
            eirods::log( ret );
            return ret;
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
        if( !spec_coll ) {
            return ERROR( -1, "tarFileOpendirPlugin - null spec_coll pointer in PluginStructFileDesc" );
        }

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( sub_index, "tarFileOpenPlugin - alloc_tar_sub_file_desc failed." );
        }

        // =-=-=-=-=-=-=-
        // build a file open structure to pass off to the server api call
        fileOpendirInp_t fileOpendirInp;
        memset( &fileOpendirInp, 0, sizeof( fileOpendirInp ) );
        fileOpendirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        strncpy( fileOpendirInp.addr.hostAddr, resc_host.c_str(), NAME_LEN );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileOpendirInp.dirName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileRmdirPlugin - compose_cache_dir_physical_path failed.", comp_err );
        }
        
        // =-=-=-=-=-=-=-
        // make the api call to open a directory 
        int status = rsFileOpendir( comm, &fileOpendirInp );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "tarFileOpendirPlugin - error returned from rsFileOpendir for: [";
            msg << fileOpendirInp.dirName;
            msg << "], status: ";
            msg << status;
            eirods::error ret = ERROR( status, msg.str() );
            eirods::log( ret );
            return ret;
        } else {
            PluginTarSubFileDesc[ sub_index ].fd = status;
            PluginStructFileDesc[ struct_file_index ].openCnt++;
            _object->file_descriptor( sub_index );

            return CODE( sub_index );
        }

    } // tarFileOpendirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error tarFileClosedirPlugin( rsComm_t*                      _comm,
                                         const std::string&             _results,
                                         eirods::resource_property_map* 
                                         _prop_map, 
                                         eirods::resource_child_map* 
                                         _cmap,
                                         eirods::first_class_object*
                                         _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileClosedirPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if( _object->file_descriptor() < 1                      || 
            _object->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
            PluginTarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileClosedirPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileClosedirPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // build a file close dir structure to pass off to the server api call
        fileClosedirInp_t fileClosedirInp;
        fileClosedirInp.fileInx = PluginTarSubFileDesc[ _object->file_descriptor() ].fd;
        int status = rsFileClosedir( comm, &fileClosedirInp );
        if( status < 0 ) {
            eirods::log( LOG_ERROR, "tarFileClosedirPlugin - failed." );
            return ERROR( -1, "tarFileClosedirPlugin - failed on call to rsFileClosedir" );
        }

        // =-=-=-=-=-=-=-
        // close out the sub file index and free the allocation
        int struct_file_index = PluginTarSubFileDesc[ _object->file_descriptor() ].structFileInx;
        PluginStructFileDesc[ struct_file_index ].openCnt++;
        free_tar_sub_file_desc( _object->file_descriptor() );
        _object->file_descriptor( 0 );

        return CODE( status );

    } // tarFileClosedirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error tarFileReaddirPlugin( rsComm_t*                      _comm,
                                        const std::string&             _results,
                                        eirods::resource_property_map* 
                                        _prop_map, 
                                        eirods::resource_child_map* 
                                        _cmap,
                                        eirods::first_class_object*
                                        _object,
                                        struct rodsDirent** _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileReaddirPlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if( _object->file_descriptor() < 1                      || 
            _object->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
            PluginTarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReaddirPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileReaddirPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // build a file read dir structure to pass off to the server api call
        fileReaddirInp_t fileReaddirInp; 
        fileReaddirInp.fileInx = PluginTarSubFileDesc[ _object->file_descriptor() ].fd;

        // =-=-=-=-=-=-=-
        // make the api call to read the directory
        int status = rsFileReaddir( comm, &fileReaddirInp, _dirent_ptr );
        if( status < -1 ) {
            return ERROR( status, "tarFileReaddirPlugin - failed in call to rsFileReaddir" );
        }

        return CODE( status );

    } // tarFileReaddirPlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error tarFileStagePlugin( rsComm_t*                      _comm,
                                      const std::string&             _results,
                                      eirods::resource_property_map* 
                                      _prop_map, 
                                      eirods::resource_child_map* 
                                      _cmap,
                                      eirods::first_class_object*
                                      _object ) {
        // =-=-=-=-=-=-=-
        // this interface is not implemented in this plugin
        return ERROR( -1, "tarFileStagePlugin - not implemented." );

    } // tarFileStagePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX rename
    eirods::error tarFileRenamePlugin( rsComm_t*                      _comm,
                                       const std::string&             _results,
                                       eirods::resource_property_map* 
                                       _prop_map, 
                                       eirods::resource_child_map* 
                                       _cmap,
                                       eirods::first_class_object*
                                       _object, 
                                       const char*         _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileRenamePlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileRenamePlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileRenamePlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileRenamePlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
        
        // =-=-=-=-=-=-=-
        // build a file rename structure to pass off to the server api call
        fileRenameInp_t fileRenameInp;
        memset (&fileRenameInp, 0, sizeof (fileRenameInp));
        fileRenameInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        strncpy( fileRenameInp.addr.hostAddr, resc_host.c_str(), NAME_LEN );
         
        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err_old = compose_cache_dir_physical_path( fileRenameInp.oldFileName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err_old.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileRenamePlugin - compose_cache_dir_physical_path failed for old file name.", comp_err_old );
        }

        eirods::error comp_err_new = compose_cache_dir_physical_path( fileRenameInp.newFileName, spec_coll, _new_file_name );
        if( !comp_err_new.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileRenamePlugin - compose_cache_dir_physical_path failed for new file name.", comp_err_new );
        }

        // =-=-=-=-=-=-=-
        // make the api call for rename
        int status = rsFileRename( comm, &fileRenameInp );
        if( status >= 0 ) {
            // use the specColl in PluginStructFileDesc 
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written 
            if( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if (status1 < 0) {
                    return ERROR( status1, "tarFileRenamePlugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

    } // tarFileRenamePlugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error tarFileTruncatePlugin( rsComm_t*                      _comm,
                                         const std::string&             _results,
                                         eirods::resource_property_map* 
                                         _prop_map, 
                                         eirods::resource_child_map* 
                                         _cmap,
                                         eirods::first_class_object*
                                         _object ) { 
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileTruncatePlugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileTruncatePlugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileTruncatePlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileTruncatePlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( sub_index, "tarFileTruncatePlugin - alloc_tar_sub_file_desc failed." );
        }

        // =-=-=-=-=-=-=-
        // cache struct file index into sub file index
        PluginTarSubFileDesc[ sub_index ].structFileInx = struct_file_index;
        
        // =-=-=-=-=-=-=-
        // build a file create structure to pass off to the server api call
        fileOpenInp_t fileTruncateInp;
        memset( &fileTruncateInp, 0, sizeof( fileTruncateInp ) );
        strncpy( fileTruncateInp.addr.hostAddr,  resc_host.c_str(), NAME_LEN );
        fileTruncateInp.dataSize = struct_obj->offset();

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        eirods::error comp_err = compose_cache_dir_physical_path( fileTruncateInp.fileName, spec_coll, struct_obj->sub_file_path().c_str() );
        if( !comp_err.ok() ) {
            return PASS( false, SYS_STRUCT_FILE_PATH_ERR, 
                         "tarFileTruncatePlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        // =-=-=-=-=-=-=-
        // make the truncate api call
        int status = rsFileTruncate( comm, &fileTruncateInp );
        if( status > 0 ) {
            // =-=-=-=-=-=-=-
            // cache has been written 
            int         struct_idx = PluginTarSubFileDesc[ _object->file_descriptor() ].structFileInx;
            specColl_t* spec_coll   = PluginStructFileDesc[ struct_idx ].specColl;
            if( spec_coll->cacheDirty == 0 ) {
                spec_coll->cacheDirty = 1;    
                int status1 = modCollInfo2( _object->comm(), spec_coll, 0 );
                if( status1 < 0 ) 
                    return CODE( status1 );
            }
        }

        return CODE( status );

    } // tarFileTruncatePlugin

	
    // =-=-=-=-=-=-=-
	// interface to extract a tar file
	eirods::error tarFileExtractPlugin( rsComm_t*                      _comm,
                                        const std::string&             _results,
                                        eirods::resource_property_map* _prop_map, 
								        eirods::resource_child_map*    _cmap,
                                        eirods::first_class_object*    _object ) { 
		// =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileExtractPlugin", chk_err );
		}

		// =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileExtractPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileExtractPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // allocate a position in the struct table
        int struct_file_index = 0;
        if( ( struct_file_index = alloc_struct_file_desc() ) < 0 ) {
            return ERROR( struct_file_index, "tarFileExtractPlugin - failed to allocate struct file descriptor" );
        }

		// =-=-=-=-=-=-=-
        // populate the file descriptor table
        PluginStructFileDesc[ struct_file_index ].inuseFlag = 1;
        PluginStructFileDesc[ struct_file_index ].specColl  = spec_coll;
        PluginStructFileDesc[ struct_file_index ].rsComm    = comm;
        strncpy( PluginStructFileDesc[ struct_file_index ].dataType, 
                 struct_obj->data_type().c_str(), NAME_LEN );

		// =-=-=-=-=-=-=-
        // extract the file
        eirods::error ext_err = extract_file( struct_file_index );
        if( !ext_err.ok() ) {
            // NOTE:: may need to remove the cacheDir too 
            std::stringstream msg;
            msg << "tarFileExtractPlugin - failed to extact structure file for [";
            msg << spec_coll->objPath;
            msg << "] in directory [";
            msg << spec_coll->cacheDir;
            msg << "] with errno of ";
            msg << errno;
            return PASS( false, SYS_TAR_STRUCT_FILE_EXTRACT_ERR - errno, msg.str(), ext_err );
        }

        return CODE( ext_err.code() );

	} // tarFileExtractPlugin
    
    // =-=-=-=-=-=-=-
    // helper function to write an archive entry
    eirods::error write_file_to_archive( const boost::filesystem::path _path,
                                         const std::string&            _cache_dir, 
                                         struct archive*               _archive ) {
        namespace fs = boost::filesystem;
        struct archive_entry* entry = archive_entry_new();

        // =-=-=-=-=-=-=-
        // strip arch path from file name for header entry
        std::string path_name  = _path.string();
        std::string strip_file = path_name.substr( _cache_dir.size()+1 ); // add one for the last '/'
        archive_entry_set_pathname( entry, strip_file.c_str() );
        
        // =-=-=-=-=-=-=-
        // ask for the file size
        archive_entry_set_size( entry, fs::file_size( _path ) );
        archive_entry_set_filetype( entry, AE_IFREG );

        // =-=-=-=-=-=-=-
        // set the permissions
        //fs::perms pp;
        //fs::permissions( _path, pp );
        archive_entry_set_perm( entry, 0600 );

        // =-=-=-=-=-=-=-
        // set the time for the file
        std::time_t tt = last_write_time( _path );
        archive_entry_set_mtime( entry, tt, 0 );

        // =-=-=-=-=-=-=-
        // write out the header to the archive
        if( archive_write_header( _archive, entry ) != ARCHIVE_OK ) {
            std::stringstream msg;
            msg << "write_file_to_archive - failed to write entry header for [";
            msg << path_name;
            msg << "] with error string [";
            msg << archive_error_string( _archive );
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // JMC :: i didnt use ifstream as readsome() garbled the file
        //     :: some reason.  revisit this for windows
        // =-=-=-=-=-=-=-
        // open the file in question
        int fd = open( path_name.c_str(), O_RDONLY );
        if( -1 == fd )  {
            std::stringstream msg;
            msg << "write_file_to_archive - failed to open file for read [";
            msg << path_name;
            msg << "] with error [";
            msg << strerror( errno );
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // read in the file and add it to the archive
        char buff[ 16384 ];
        int len = read( fd, buff, sizeof( buff ) );        
        while( len > 0 ) {
            archive_write_data( _archive, buff, len );
            len = read( fd, buff, sizeof( buff ) );        
        }

        // =-=-=-=-=-=-=-
        // clean up
        close( fd );
        archive_entry_free( entry );

        return SUCCESS();

    } // write_file_to_archive
  
    // =-=-=-=-=-=-=-
    // helper function for recursive directory scanning
    eirods::error build_directory_listing( const boost::filesystem::path&          _path, 
                                           std::vector< boost::filesystem::path >& _listing ) {
        // =-=-=-=-=-=-=-
        // namespace alias for brevity 
        namespace fs = boost::filesystem;
        eirods::error final_error = SUCCESS();

        // =-=-=-=-=-=-=-
        // begin iterating over the cache dir
         try {
            if( fs::is_directory( _path ) ) {
                // =-=-=-=-=-=-=-
                // iterate over the directory and archive it
                fs::directory_iterator end_iter;
                fs::directory_iterator dir_itr( _path );
                for ( ; dir_itr != end_iter; ++dir_itr ) {
                    // =-=-=-=-=-=-=-
                    // recurse on this new directory
                    eirods::error ret = build_directory_listing( dir_itr->path(), _listing );
                    if( !ret.ok() ) {
                        std::stringstream msg;
                        msg << "build_directory_listing - failed on [";
                        msg << dir_itr->path().string();
                        msg << "]";
                        final_error = PASS( -1, false, msg.str(), final_error );
                    }

                } // for dir_itr
            
            } else if( fs::is_regular_file( _path ) ) {
                // =-=-=-=-=-=-=-
                // add file path to the vector
                _listing.push_back( _path );

            } else {
                std::stringstream msg;
                msg << "build_directory_listing - unhandled entry [";
                msg << _path.filename();
                msg << "]"; 
                rodsLog( LOG_NOTICE, msg.str().c_str() );
            }

        } catch ( const std::exception & ex ) {
            std::stringstream msg;
            msg << "build_directory_listing - caught exception [";
            msg << ex.what();
            msg << "] for directory entry [";
            msg << _path.filename();
            msg << "]";
            return ERROR( -1, msg.str() );

        }

        return final_error; 
  
    } // build_directory_listing 
   
    // =-=-=-=-=-=-=-
    // create an archive from the cache directory 
    eirods::error bundle_cache_dir( int         _index, 
                                    std::string _data_type ) {
        // =-=-=-=-=-=-=-
        // namespace alias for brevity 
        namespace fs = boost::filesystem;

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        if( PluginStructFileDesc[ _index ].inuseFlag <= 0 ) {
            std::stringstream msg;
            msg << "extract_file - struct file index: ";
            msg << _index;
            msg << " is not in use";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        if( spec_coll == NULL                   || 
            spec_coll->cacheDirty <= 0          ||
            strlen( spec_coll->cacheDir ) == 0  ||
            strlen( spec_coll->phyPath  ) == 0 ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - bad special collection for index: ";
            msg << _index;
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );

        }
  
        // =-=-=-=-=-=-=-
        // create a boost filesystem path for the cache directory
        fs::path full_path;//( fs::initial_path<fs::path>() );
        full_path = fs::system_complete( fs::path( spec_coll->cacheDir ) );

        // =-=-=-=-=-=-=-
        // check if the path is really there, just in case
        if( !fs::exists( full_path ) ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - cache directory does not exist [";
            msg << spec_coll->cacheDir;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // make sure it is a directory, just in case
        if( !fs::is_directory( full_path ) ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - cache directory is not actually a directory [";
            msg << spec_coll->cacheDir;
            msg << "]";
            return ERROR( -1, msg.str() );
        }
 
        // =-=-=-=-=-=-=-
        // build a listing of all of the files in all of the subdirs 
        // in the full_path
        std::vector< fs::path > listing;
        build_directory_listing( full_path, listing );
         
        // =-=-=-=-=-=-=-
        // create the archive
        struct archive* arch = archive_write_new();
        if( !arch ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - failed to create archive struct for [";
            msg << spec_coll->cacheDir;
            msg << "] into archive file [";
            msg << spec_coll->phyPath;
            msg << "]";
            return ERROR( -1, msg.str() );
            
        }
   
        // =-=-=-=-=-=-=-
        // set the compression flags given data_type
        if( _data_type == ZIP_DT_STR ) {
            if( archive_write_add_filter_lzip( arch ) != ARCHIVE_OK ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to set compression to lzip for archive [";
                msg << spec_coll->phyPath;
                msg << "] with error string [";
                msg << archive_error_string( arch );
                msg << "]";
                return ERROR( -1, msg.str() );

            }
             
        } else if( _data_type == GZIP_TAR_DT_STR ) {
            if( archive_write_add_filter_gzip( arch ) != ARCHIVE_OK ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to set compression to gzip for archive [";
                msg << spec_coll->phyPath;
                msg << "] with error string [";
                msg << archive_error_string( arch );
                msg << "]";
                return ERROR( -1, msg.str() );

            }

        } else if( _data_type == BZIP2_TAR_DT_STR ) {
            if( archive_write_add_filter_bzip2( arch ) != ARCHIVE_OK ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to set compression to bzip2 for archive ["; 
                msg << spec_coll->phyPath;
                msg << "] with error string [";
                msg << archive_error_string( arch );
                msg << "]";
                return ERROR( -1, msg.str() );

            }
        
        } else {
            if( archive_write_set_compression_none( arch ) != ARCHIVE_OK ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to set compression to none for archive ["; 
                msg << spec_coll->phyPath;
                msg << "] with error string [";
                msg << archive_error_string( arch );
                msg << "]";
                return ERROR( -1, msg.str() );

            }
        
        }
        
        // =-=-=-=-=-=-=-
        // set the format of the tar archive
        archive_write_set_format_ustar( arch );

        // =-=-=-=-=-=-=-
        // open the spec coll physical path for archival
        if( archive_write_open_filename( arch, spec_coll->phyPath ) < ARCHIVE_OK ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - failed to open archive file [";
            msg << spec_coll->phyPath;
            msg << "] with error string [";
            msg << archive_error_string( arch );
            msg << "]";
            return ERROR( -1, msg.str() );
         }

        // =-=-=-=-=-=-=-
        // iterate over the dir listing and archive the files
        std::string cache_dir( spec_coll->cacheDir );
        eirods::error arch_err = SUCCESS();
        for( size_t i = 0; i < listing.size(); ++i ) {
            // =-=-=-=-=-=-=-
            // strip off archive path from the filename
            eirods::error ret = write_file_to_archive( listing[ i ].string(), cache_dir, arch );

            if( !ret.ok() ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to archive file [";
                msg << listing[ i ].string();
                msg << "]";
                arch_err = PASS( -1, false, msg.str(), arch_err );
                eirods::log( PASS( -1, false, msg.str(), ret ) );
            } 

        } // for i

        // =-=-=-=-=-=-=-
        // close the archive and clean up
        archive_write_close( arch );
        archive_write_free( arch );
        
        // =-=-=-=-=-=-=-
        // handle errors
        if( !arch_err.ok() ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - failed to archive [";
            msg << spec_coll->cacheDir;
            msg << "] into archive file [";
            msg << spec_coll->phyPath;
            msg << "]";
            return PASS( false, -1, msg.str(), arch_err );

        } else {
            return SUCCESS();

        }

    } // bundle_cache_dir

    // =-=-=-=-=-=-=-
    // interface for tar / zip up of cache dir which also updates
    // the icat with the new file size
    eirods::error sync_cache_dir_to_tar_file( int         _index, 
                                              int         _opr_type,
                                              std::string _host ) {
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        rsComm_t*   comm      = PluginStructFileDesc[ _index ].rsComm;

        // =-=-=-=-=-=-=-
        // call bundle helper functions
        eirods::error bundle_err = bundle_cache_dir( _index, PluginStructFileDesc[ _index ].dataType );
        if( !bundle_err.ok() ) {
            return PASS( false, -1, "sync_cache_dir_to_tar_file - failed in bundle.", bundle_err );
        }

        // =-=-=-=-=-=-=-
        // create a file stat structure for the rs call
        fileStatInp_t file_stat_inp;
        memset( &file_stat_inp, 0, sizeof( file_stat_inp ) );
        rstrcpy( file_stat_inp.fileName, spec_coll->phyPath, MAX_NAME_LEN );
        strncpy( file_stat_inp.addr.hostAddr, _host.c_str(), NAME_LEN );

        // =-=-=-=-=-=-=-
        // call file stat api to get the size of the new file
        rodsStat_t* file_stat_out = NULL;
        int status = rsFileStat( comm, &file_stat_inp, &file_stat_out );
        if( status < 0 || NULL ==  file_stat_out ) { 
            std::stringstream msg;
            msg << "sync_cache_dir_to_tar_file - failed on call to rsFileStat for [";
            msg << spec_coll->phyPath;
            msg << "] with status of ";
            msg << status;
            return ERROR( status, msg.str() );
              
        }

        // =-=-=-=-=-=-=-
        // update icat with the new size of the file
        if( ( _opr_type & NO_REG_COLL_INFO ) == 0 ) {
            // NOTE :: for bundle opr, done at datObjClose 
            status = regNewObjSize( comm, spec_coll->objPath, spec_coll->replNum, file_stat_out->st_size );

        }

        free( file_stat_out );

        return CODE( status );

    } // sync_cache_dir_to_tar_file

    // =-=-=-=-=-=-=-
	// tarFileCopyPlugin
	eirods::error tarFileSyncPlugin( rsComm_t*                      _comm,
                                     const std::string&             _results,
                                     eirods::resource_property_map* _prop_map, 
                                     eirods::resource_child_map*    _cmap,
                                     eirods::first_class_object*    _object ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        eirods::error chk_err = param_check( _prop_map, _cmap, _object );
        if( !chk_err.ok() ) {
            return PASS( false, -1, "tarFileSyncPlugin", chk_err );
		}

		// =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( -1, "tarFileSyncPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( -1, "tarFileSyncPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        eirods::error open_err = tar_struct_file_open( comm, spec_coll, struct_file_index, resc_host );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileOpenPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // copy the data type requested by user for compression
        strncpy( PluginStructFileDesc[ struct_file_index ].dataType, struct_obj->data_type().c_str(), NAME_LEN );

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

		// =-=-=-=-=-=-=-
        // we cant sync if the structured collection is currently in use
        if( PluginStructFileDesc[ struct_file_index ].openCnt > 0 ) {
            return ERROR( SYS_STRUCT_FILE_BUSY_ERR, "tarFileSyncPlugin - spec coll in use" );
        }
        
		// =-=-=-=-=-=-=-
        // delete operation
        if( ( struct_obj->opr_type() & DELETE_STRUCT_FILE) != 0 ) {
            /* remove cache and the struct file */
            free_struct_file_desc( struct_file_index );
            return SUCCESS();
        }

		// =-=-=-=-=-=-=-
        // check if there is a specified cache directory,
        // if not then any other operation isn't possible
        if( strlen( spec_coll->cacheDir ) > 0 ) {
            if( spec_coll->cacheDirty > 0) {
		        // =-=-=-=-=-=-=-
                // write the tar file and register no dirty 
                eirods::error sync_err = sync_cache_dir_to_tar_file( struct_file_index, 
                                                                     struct_obj->opr_type(), 
                                                                     resc_host );
                if( !sync_err.ok() ) {
                    std::stringstream msg;
                    msg << "tarFileSyncPlugin - failed in sync_cache_dir_to_tar_file for [";
                    msg << spec_coll->cacheDir;
                    msg << "] with status of ";
                    msg << sync_err.code();

                    free_struct_file_desc ( struct_file_index );
                    return PASS( false, -1, msg.str(), sync_err );
                }

                spec_coll->cacheDirty = 0;
                if( ( struct_obj->opr_type() & NO_REG_COLL_INFO ) == 0 ) {
                    int status = modCollInfo2( comm,  spec_coll, 0 );
                    if( status < 0 ) {
                        free_struct_file_desc( struct_file_index );
                        return ERROR( status, "tarFileSyncPlugin - failed in modCollInfo2" );
                    
                    } // if status
                
                }

            } // if cache is dirty

            // =-=-=-=-=-=-=-
            // remove cache dir if necessary
            if( ( struct_obj->opr_type() & PURGE_STRUCT_FILE_CACHE ) != 0 ) {
                // =-=-=-=-=-=-=-
                // need to unregister the cache before removing it
                int status = modCollInfo2( comm,  spec_coll, 1 );
                if( status < 0 ) {
                    free_struct_file_desc( struct_file_index );
                    return ERROR( status, "tarFileSyncPlugin - failed in modCollInfo2" );
                }

                // =-=-=-=-=-=-=-
                // use the irods api to remove the directory
                fileRmdirInp_t rmdir_inp;
                memset( &rmdir_inp, 0, sizeof( rmdir_inp ) );
                rmdir_inp.flags    = RMDIR_RECUR;
                rmdir_inp.fileType = UNIX_FILE_TYPE;  
                rstrcpy( rmdir_inp.dirName,       spec_coll->cacheDir, MAX_NAME_LEN );
                strncpy( rmdir_inp.addr.hostAddr, resc_host.c_str(),   NAME_LEN );
                
                status = rsFileRmdir( comm, &rmdir_inp );
                if( status < 0 ) {
                    free_struct_file_desc( struct_file_index );
                    return ERROR( status, "tarFileSyncPlugin - failed in call to rsFileRmdir" );
                }

            } // if purge file cache
        
        } // if we have a cache dir

        free_struct_file_desc( struct_file_index );
        
        return SUCCESS();

	} // tarFileSyncPlugin

    // =-=-=-=-=-=-=-
	// tarFileCopyPlugin
	eirods::error tarFileGetFsFreeSpacePlugin( rsComm_t*                      _comm,
                                               const std::string&             _results,
                                               eirods::resource_property_map* _prop_map, 
                                               eirods::resource_child_map*    _cmap,
                                               eirods::first_class_object*    _object ) {
        return ERROR( -1, "tarFileGetFsFreeSpacePlugin is not implemented" );

    } // tarFileGetFsFreeSpacePlugin

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle tar file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class tarfilesystem_resource : public eirods::resource {
    public:
        tarfilesystem_resource( const std::string& _inst_name, const std::string& _context ) :
        eirods::resource( _inst_name, _context ) {
        } // ctor

    }; // class tarfilesystem_resource
  
    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the e-irods facing interface defined in 
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create tarfilesystem_resource
        tarfilesystem_resource* resc = new tarfilesystem_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
		// 4b1. set start and stop operations for alloc / free of tables
        resc->set_start_operation( "tarfilesystem_resource_start" );
        resc->set_stop_operation ( "tarfilesystem_resource_stop"  );

        // =-=-=-=-=-=-=-
		// 4b2. map function names to operations.  this map will be used to load
		//     the symbols from the shared object in the delay_load stage of 
		//     plugin loading.
        resc->add_operation( "create",       "tarFileCreatePlugin" );
        resc->add_operation( "open",         "tarFileOpenPlugin" );
        resc->add_operation( "read",         "tarFileReadPlugin" );
        resc->add_operation( "write",        "tarFileWritePlugin" );
        resc->add_operation( "close",        "tarFileClosePlugin" );
        resc->add_operation( "unlink",       "tarFileUnlinkPlugin" );
        resc->add_operation( "stat",         "tarFileStatPlugin" );
        resc->add_operation( "fstat",        "tarFileFstatPlugin" );
        resc->add_operation( "lseek",        "tarFileLseekPlugin" );
        resc->add_operation( "fsync",        "tarFileFsyncPlugin" );
        resc->add_operation( "mkdir",        "tarFileMkdirPlugin" );
        resc->add_operation( "chmod",        "tarFileChmodPlugin" );
        resc->add_operation( "opendir",      "tarFileOpendirPlugin" );
        resc->add_operation( "readdir",      "tarFileReaddirPlugin" );
        resc->add_operation( "stage",        "tarFileStagePlugin" );
        resc->add_operation( "rename",       "tarFileRenamePlugin" );
        resc->add_operation( "rmdir",        "tarFileRmdirPlugin" );
        resc->add_operation( "closedir",     "tarFileClosedirPlugin" );
        resc->add_operation( "truncate",     "tarFileTruncatePlugin" );
        resc->add_operation( "extract",      "tarFileExtractPlugin" );
        resc->add_operation( "sync",         "tarFileSyncPlugin" );
        resc->add_operation( "freespace",    "tarFileGetFsFreeSpacePlugin" );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



