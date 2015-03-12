// =-=-=-=-=-=-=-
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_structured_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_resource_manager.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "apiHeaderAll.hpp"

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
#include "rsGlobalExtern.hpp"
#include "modColl.hpp"
#include "apiHeaderAll.hpp"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "resource.hpp"
#include "miscServerFunct.hpp"
#include "physPath.hpp"
#include "fileOpen.hpp"


// =-=-=-=-=-=-=-=-
// file descriptor tables which hold references to struct files
// which are currently in flight
const int NUM_STRUCT_FILE_DESC = 16;
structFileDesc_t PluginStructFileDesc[ NUM_STRUCT_FILE_DESC  ];
tarSubFileDesc_t PluginTarSubFileDesc[ NUM_TAR_SUB_FILE_DESC ];

// =-=-=-=-=-=-=-=-
// manager of resource plugins which are resolved and cached
extern irods::resource_manager resc_mgr;

extern "C" {

    // =-=-=-=-=-=-=-
    // start operation used to allocate the FileDesc tables
    void tarfilesystem_resource_start() {
        memset( PluginStructFileDesc, 0, sizeof( structFileDesc_t ) * NUM_STRUCT_FILE_DESC );
        memset( PluginTarSubFileDesc, 0, sizeof( tarSubFileDesc_t ) * NUM_TAR_SUB_FILE_DESC );
    }

    // =-=-=-=-=-=-=-
    // stop operation used to free the FileDesc tables
    void tarfilesystem_resource_stop() {
        memset( PluginStructFileDesc, 0, sizeof( structFileDesc_t ) * NUM_STRUCT_FILE_DESC );
        memset( PluginTarSubFileDesc, 0, sizeof( tarSubFileDesc_t ) * NUM_TAR_SUB_FILE_DESC );
    }

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

    // =-=-=-=-=-=-=-
    // helper function to check incoming parameters
    inline irods::error tar_check_params(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // ask the context if it is valid
        irods::error ret = _ctx.valid< irods::structured_object >();
        if ( !ret.ok() ) {
            return PASSMSG( "resource context is invalid", ret );

        }

        return SUCCESS();

    } // tar_check_params

    // =-=-=-=-=-=-=-
    // @brief simple struct to pass into libarchive callbacks
    struct cb_ctx_t {
        int               idx_;
        char              loc_[ NAME_LEN ];
        structFileDesc_t* desc_;
        bytesBuf_t        read_buf;
    };

    // =-=-=-=-=-=-=-
    // OPEN callback for use by libarchive which makes use of the
    // irods rsFile API for file access
    int irods_file_open(
        struct archive* _arch,
        void*           _data,
        int             mode ) {
        if ( !_arch ||
                !_data ) {
            rodsLog( LOG_ERROR, "irods_file_open - null input" );
            return ARCHIVE_FATAL;
        }

        // =-=-=-=-=-=-=-
        // cast data pointer to the cb_struct
        cb_ctx_t* cb_ctx = static_cast< cb_ctx_t* >( _data );

        // =-=-=-=-=-=-=-
        // handy pointer to special collection
        specColl_t* spec_coll = cb_ctx->desc_->specColl;

        // =-=-=-=-=-=-=-
        // prepare a file inp to call a rsFileOpen
        fileOpenInp_t f_inp;
        memset( &f_inp, 0, sizeof( f_inp ) );
        rstrcpy( f_inp.resc_name_,	spec_coll->resource, MAX_NAME_LEN );
        rstrcpy( f_inp.resc_hier_,	spec_coll->rescHier, MAX_NAME_LEN );
        rstrcpy( f_inp.objPath,		spec_coll->objPath,  MAX_NAME_LEN );
        rstrcpy( f_inp.addr.hostAddr,	cb_ctx->loc_,        NAME_LEN );
        rstrcpy( f_inp.fileName,	spec_coll->phyPath,  MAX_NAME_LEN );
        f_inp.mode  = getDefFileMode();
        f_inp.flags = mode;
        //rstrcpy( f_inp.in_pdmo, dataObjInfo->in_pdmo, MAX_NAME_LEN );
        cb_ctx->idx_ = rsFileOpen( cb_ctx->desc_->rsComm, &f_inp );

        return ARCHIVE_OK;

    } // irods_file_open

    int irods_file_open_for_read(
        struct archive* _arch,
        void*           _data ) {
        return irods_file_open( _arch, _data, O_RDONLY );
    }

    int irods_file_open_for_write(
        struct archive* _arch,
        void*           _data ) {
        return irods_file_open( _arch, _data, O_WRONLY | O_CREAT );
    }

    // =-=-=-=-=-=-=-
    // READ callback for use by libarchive which makes use of the
    // irods rsFile API for file access
    ssize_t irods_file_read(
        struct archive* _arch,
        void*           _data,
        const void**    _buff ) {
        if ( !_arch ||
                !_data ||
                !_buff ) {
            rodsLog( LOG_ERROR, "irods_file_read - null input" );
            return ARCHIVE_FATAL;
        }

        // =-=-=-=-=-=-=-
        // cast data pointer to the cb_struct
        cb_ctx_t* cb_ctx = static_cast< cb_ctx_t* >( _data );

        // =-=-=-=-=-=-=-
        // stat the file to get its size
        rodsStat_t*   stbuf = 0;
        fileStatInp_t f_inp;
        memset( &f_inp, 0, sizeof( f_inp ) );
        rstrcpy( f_inp.fileName, FileDesc[ cb_ctx->idx_ ].fileName, MAX_NAME_LEN );
        rstrcpy( f_inp.rescHier, FileDesc[ cb_ctx->idx_ ].rescHier, MAX_NAME_LEN );
        rstrcpy( f_inp.objPath,  FileDesc[ cb_ctx->idx_ ].objPath,  MAX_NAME_LEN );
        rstrcpy( f_inp.addr.hostAddr, cb_ctx->loc_, NAME_LEN );
        int status = rsFileStat( cb_ctx->desc_->rsComm, &f_inp, &stbuf );
        if ( status != 0 ) {
            if ( status != UNIX_FILE_STAT_ERR - ENOENT ) {
                rodsLog( LOG_DEBUG, "irods_file_read: can't stat %s. status = %d",
                         f_inp.fileName, status );
            }
            return -1;
        }

        size_t buf_len = stbuf->st_size;
        free( stbuf );

        // =-=-=-=-=-=-=-
        // build a read inp and read the buffer
        if ( cb_ctx->read_buf.buf ) {
            free( cb_ctx->read_buf.buf );
        }

        memset( &cb_ctx->read_buf, 0, sizeof( cb_ctx->read_buf ) );
        cb_ctx->read_buf.buf = malloc( buf_len );
        cb_ctx->read_buf.len = buf_len;

        fileReadInp_t r_inp;
        memset( &r_inp, 0, sizeof( r_inp ) );
        r_inp.fileInx = cb_ctx->idx_;
        r_inp.len     = buf_len;
        status = rsFileRead( cb_ctx->desc_->rsComm, &r_inp, &cb_ctx->read_buf );
        if ( status < 0 ) {
            return -1;
        }
        else {
            ( *_buff ) = cb_ctx->read_buf.buf;
            return status;
        }

    } // irods_file_read

    // =-=-=-=-=-=-=-
    // CLOSE callback for use by libarchive which makes use of the
    // irods rsFile API for file access
    int irods_file_close(
        struct archive* _arch,
        void*           _data ) {
        // =-=-=-=-=-=-=-
        // parameter check
        if ( !_arch ||
                !_data ) {
            rodsLog( LOG_ERROR, "irods_file_close - null input" );
            return ARCHIVE_FATAL;
        }
        // =-=-=-=-=-=-=-
        // cast data pointer to the cb_struct
        cb_ctx_t* cb_ctx = static_cast< cb_ctx_t* >( _data );

        fileCloseInp_t fileCloseInp;
        memset( &fileCloseInp, 0, sizeof( fileCloseInp ) );
        fileCloseInp.fileInx = cb_ctx->idx_;
        return rsFileClose( cb_ctx->desc_->rsComm, &fileCloseInp );

    } // irods_file_close

    // =-=-=-=-=-=-=-
    //
    ssize_t irods_file_write(
        struct archive* _arch,
        void*           _data,
        const void*     _buff,
        size_t          _len ) {
        // =-=-=-=-=-=-=-
        // parameter check
        if ( !_arch ||
                !_data ||
                !_buff ) {
            rodsLog( LOG_ERROR, "irods_file_write - null input" );
            return ARCHIVE_FATAL;
        }

        // =-=-=-=-=-=-=-
        // cast data pointer to the cb_struct
        cb_ctx_t* cb_ctx = static_cast< cb_ctx_t* >( _data );

        fileWriteInp_t fileWriteInp;
        memset( &fileWriteInp, 0, sizeof( fileWriteInp ) );
        fileWriteInp.fileInx = cb_ctx->idx_;
        fileWriteInp.len     = _len;

        bytesBuf_t write_buf;
        write_buf.buf = const_cast< void* >( _buff );
        write_buf.len = _len;
        int sz = rsFileWrite(
                     cb_ctx->desc_->rsComm,
                     &fileWriteInp,
                     &write_buf );
        if ( sz < 0 ) {
            return -1;
        }
        else {
            return sz;
        }

    } // irods_file_write

    // =-=-=-=-=-=-=-
    // call archive file extraction for struct file
    irods::error extract_file( int _index ) {
        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        if ( PluginStructFileDesc[ _index ].inuseFlag <= 0 ) {
            std::stringstream msg;
            msg << "extract_file - struct file index: ";
            msg << _index;
            msg << " is not in use";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        if ( spec_coll == NULL                  ||
                strlen( spec_coll->cacheDir ) == 0 ||
                strlen( spec_coll->phyPath ) == 0 ) {
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
        archive_read_support_filter_all( arch );
        archive_read_support_format_all( arch );

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( spec_coll->rescHier, location );
        if ( !ret.ok() ) {
            return PASSMSG( "extract_file - failed in get_loc_for_hier_string", ret );
        }

        cb_ctx_t cb_ctx;
        memset( &cb_ctx, 0, sizeof( cb_ctx_t ) );
        cb_ctx.desc_ = &PluginStructFileDesc[ _index ];
        snprintf( cb_ctx.loc_, sizeof( cb_ctx.loc_ ), "%s", location.c_str() );

        // =-=-=-=-=-=-=-
        // open the archive and and prepare to read
        if ( archive_read_open(
                    arch,
                    &cb_ctx,
                    irods_file_open_for_read,
                    irods_file_read,
                    irods_file_close ) != ARCHIVE_OK ) {
            std::stringstream msg;
            msg << "extract_file - failed to open archive [";
            msg << spec_coll->phyPath;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a cache directory string
        std::string cache_dir( spec_coll->cacheDir );
        if ( cache_dir[ cache_dir.size() - 1 ] != '/' ) {
            cache_dir += "/";
        }

        // =-=-=-=-=-=-=-
        // iterate over entries in the archive and write them do disk
        struct archive_entry* entry;
        while ( ARCHIVE_OK == archive_read_next_header( arch, &entry ) ) {
            // =-=-=-=-=-=-=-
            // redirect the path to the cache directory
            std::string path = cache_dir + std::string( archive_entry_pathname( entry ) );
            archive_entry_set_pathname( entry, path.c_str() );

            // =-=-=-=-=-=-=-
            // read data from entry and write it to disk
            if ( ARCHIVE_OK != archive_read_extract( arch, entry, flags ) ) {
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

        // =-=-=-=-=-=-=-
        // release the last read buffer
        if ( cb_ctx.read_buf.buf ) {
            free( cb_ctx.read_buf.buf );
        }

        return SUCCESS();

    } // extract_file

    // =-=-=-=-=-=-=-
    // recursively create a cache directory for a spec coll via irods api
    irods::error make_tar_cache_dir( int _index, std::string _host ) {
        // =-=-=-=-=-=-=-
        // extract and test comm pointer
        rsComm_t* rs_comm = PluginStructFileDesc[ _index ].rsComm;
        if ( !rs_comm ) {
            std::stringstream msg;
            msg << "make_tar_cache_dir - null rsComm pointer for index: ";
            msg << _index;
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // extract and test special collection pointer
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        if ( !spec_coll ) {
            std::stringstream msg;
            msg << "make_tar_cache_dir - null specColl pointer for index: ";
            msg << _index;
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // construct a mkdir structure
        fileMkdirInp_t fileMkdirInp;
        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        fileMkdirInp.mode     = DEFAULT_DIR_MODE;
        snprintf( fileMkdirInp.addr.hostAddr, sizeof( fileMkdirInp.addr.hostAddr ), "%s", _host.c_str() );
        snprintf( fileMkdirInp.rescHier, sizeof( fileMkdirInp.rescHier ), "%s", spec_coll->rescHier );

        // =-=-=-=-=-=-=-
        // loop over a series of indicies for the directory suffix and
        // try to make the directory until it succeeds
        int dir_index = 0;
        while ( true ) {
            // =-=-=-=-=-=-=-
            // build the cache directory name
            snprintf( fileMkdirInp.dirName, MAX_NAME_LEN, "%s.%s%d",
                      spec_coll->phyPath,    CACHE_DIR_STR, dir_index );

            // =-=-=-=-=-=-=-
            // system api call to create a directory
            int status = rsFileMkdir( rs_comm, &fileMkdirInp );
            if ( status >= 0 ) {
                break;

            }
            else {
                if ( getErrno( status ) == EEXIST ) {
                    dir_index ++;
                    continue;

                }
                else {
                    return ERROR( status, "make_tar_cache_dir - failed to create cache directory" );

                } // else

            } // else

        } // while

        // =-=-=-=-=-=-=-
        // copy successful cache dir out of mkdir struct
        snprintf( spec_coll->cacheDir, sizeof( spec_coll->cacheDir ), "%s", fileMkdirInp.dirName );

        // =-=-=-=-=-=-=-
        // Win!
        return SUCCESS();

    } // make_tar_cache_dir

    // =-=-=-=-=-=-=-
    // extract the tar file into a cache dir
    irods::error stage_tar_struct_file( int _index, std::string _host ) {
        int status = -1;

        // =-=-=-=-=-=-=-
        // extract the special collection from the struct file table
        specColl_t* spec_coll = PluginStructFileDesc[_index].specColl;
        if ( spec_coll == NULL ) {
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, "stage_tar_struct_file - null spec coll" );
        }

        rsComm_t* comm = PluginStructFileDesc[ _index ].rsComm;
        if ( comm == NULL ) {
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, "stage_tar_struct_file - null comm" );
        }


        // =-=-=-=-=-=-=-
        // check to see if we have a cache dir and make one if not
        if ( strlen( spec_coll->cacheDir ) == 0 ) {
            // =-=-=-=-=-=-=-
            // no cache. stage one. make the CacheDir first
            irods::error mk_err = make_tar_cache_dir( _index, _host );
            if ( !mk_err.ok() ) {
                return PASSMSG( "failed to create cachedir", mk_err );
            }

            // =-=-=-=-=-=-=-
            // expand tar file into cache dir
            irods::error extract_err = extract_file( _index );
            if ( !extract_err.ok() ) {
                std::stringstream msg;
                msg << "stage_tar_struct_file - extract_file failed for [";
                msg << spec_coll->objPath;
                msg << "] in cache directory [";
                msg << spec_coll->cacheDir << "]";

                /* XXXX may need to remove the cacheDir too */
                return PASSMSG( msg.str(), extract_err );

            } // if !ok

            // =-=-=-=-=-=-=-
            // register the CacheDir
            status = modCollInfo2( comm, spec_coll, 0 );
            if ( status < 0 ) {
                return ERROR( status, "stage_tar_struct_file - modCollInfo2 failed." );
            }

            // =-=-=-=-=-=-=-
            // if the left over cache dir has a symlink in it, remove the
            // directory ( addresses Wisc Security Issue r4906 )
            if ( hasSymlinkInDir( spec_coll->cacheDir ) ) {
                rodsLog( LOG_ERROR, "extractTarFile: cacheDir %s has symlink in it",
                         spec_coll->cacheDir );

                /* remove cache */
                fileRmdirInp_t fileRmdirInp;
                memset( &fileRmdirInp, 0, sizeof( fileRmdirInp ) );
                rstrcpy( fileRmdirInp.dirName,       spec_coll->cacheDir,                MAX_NAME_LEN );
                rstrcpy( fileRmdirInp.addr.hostAddr, const_cast<char*>( _host.c_str() ), NAME_LEN );
                rstrcpy( fileRmdirInp.rescHier,      spec_coll->rescHier,                MAX_NAME_LEN );
                fileRmdirInp.flags = RMDIR_RECUR;
                int status = rsFileRmdir( comm, &fileRmdirInp );
                if ( status < 0 ) {
                    std::stringstream msg;
                    msg << "stage_tar_struct_file - rmdir error for [";
                    msg << spec_coll->cacheDir << "]";
                    return ERROR( status, msg.str() );
                }

            } // if hasSymlinkInDir

        } // if empty cachedir

        return SUCCESS();

    } // stage_tar_struct_file

    // =-=-=-=-=-=-=-
    // find the next free PluginStructFileDesc slot, mark it in use and return the index
    int alloc_struct_file_desc() {
        for ( int i = 1; i < NUM_STRUCT_FILE_DESC; i++ ) {
            if ( PluginStructFileDesc[i].inuseFlag == FD_FREE ) {
                PluginStructFileDesc[i].inuseFlag = FD_INUSE;
                return i;
            };
        } // for i

        return SYS_OUT_OF_FILE_DESC;

    } // alloc_struct_file_desc

    int free_struct_file_desc( int _idx ) {
        if ( _idx  < 0 || _idx  >= NUM_STRUCT_FILE_DESC ) {
            rodsLog( LOG_NOTICE,
                     "free_struct_file_desc: index %d out of range", _idx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        memset( &PluginStructFileDesc[ _idx ], 0, sizeof( structFileDesc_t ) );

        return 0;

    } // free_struct_file_desc

    // =-=-=-=-=-=-=-
    //
    int match_struct_file_desc( specColl_t* _spec_coll ) {

        for ( int i = 1; i < NUM_STRUCT_FILE_DESC; i++ ) {
            if ( PluginStructFileDesc[i].inuseFlag == FD_INUSE &&
                    PluginStructFileDesc[i].specColl  != NULL     &&
                    strcmp( PluginStructFileDesc[i].specColl->collection, _spec_coll->collection ) == 0 &&
                    strcmp( PluginStructFileDesc[i].specColl->objPath,    _spec_coll->objPath ) == 0 ) {
                return i;
            };
        } // for

        return SYS_OUT_OF_FILE_DESC;

    } // match_struct_file_desc

    // =-=-=-=-=-=-=-
    // local function to manage the open of a tar file
    irods::error tar_struct_file_open(
        rsComm_t*          _comm,
        specColl_t*        _spec_coll,
        int&               _struct_desc_index,
        const std::string& _resc_hier,
        std::string&       _resc_host ) {
        int status                  = 0;
        specCollCache_t* spec_cache = 0;

        // =-=-=-=-=-=-=-
        // trap null parameters
        if ( 0 == _spec_coll ) {
            std::string msg( "tar_struct_file_open - null special collection parameter" );
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg );
        }

        if ( 0 == _comm ) {
            std::string msg( "tar_struct_file_open - null rsComm_t parameter" );
            return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, msg );
        }

        // =-=-=-=-=-=-=-
        // look for opened PluginStructFileDesc
        _struct_desc_index = match_struct_file_desc( _spec_coll );
        if ( _struct_desc_index > 0 ) {
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // alloc and trap bad alloc
        if ( ( _struct_desc_index = alloc_struct_file_desc() ) < 0 ) {
            return ERROR( _struct_desc_index, "tar_struct_file_open - call to allocStructFileDesc failed." );
        }

        // =-=-=-=-=-=-=-
        // [ mwan? :: Have to do this because  _spec_coll could come from a remote host ]
        // NOTE :: i dont see any remote server to server comms here
        if ( ( status = getSpecCollCache( _comm,  _spec_coll->collection, 0, &spec_cache ) ) >= 0 ) {
            // =-=-=-=-=-=-=-
            // copy pointer to cached special collection
            PluginStructFileDesc[ _struct_desc_index ].specColl = &spec_cache->specColl;
            if ( !PluginStructFileDesc[ _struct_desc_index ].specColl ) {

            }

            // =-=-=-=-=-=-=-
            // copy over physical path and resource name since getSpecCollCache
            // does not give phyPath nor resource
            if ( strlen( _spec_coll->phyPath ) > 0 ) { // JMC - backport 4517
                rstrcpy( spec_cache->specColl.phyPath,  _spec_coll->phyPath, MAX_NAME_LEN );
            }
            if ( strlen( spec_cache->specColl.resource ) == 0 ) {
                rstrcpy( spec_cache->specColl.resource,  _spec_coll->resource, NAME_LEN );
            }
        }
        else {
            // =-=-=-=-=-=-=-
            // special collection is local to this server ??
            PluginStructFileDesc[ _struct_desc_index ].specColl =  _spec_coll;
        }

        // =-=-=-=-=-=-=-
        // cache pointer to comm struct
        PluginStructFileDesc[ _struct_desc_index ].rsComm = _comm;

        // =-=-=-=-=-=-=-
        // resolve the child resource by name
        irods::resource_ptr resc;
        std::string last_resc;
        irods::hierarchy_parser parser;
        parser.set_string( _resc_hier );
        parser.last_resc( last_resc );
        irods::error resc_err = resc_mgr.resolve( last_resc, resc );
        if ( !resc_err.ok() ) {
            std::stringstream msg;
            msg << "tar_struct_file_open - error returned from resolveResc for resource [";
            msg << _spec_coll->resource;
            msg << "], status: ";
            msg << resc_err.code();
            free_struct_file_desc( _struct_desc_index );
            return PASSMSG( msg.str(), resc_err );
        }

        // =-=-=-=-=-=-=-
        // extract the name of the host of the resource from the resource plugin
        rodsServerHost_t* rods_host = 0;
        irods::error get_err = resc->get_property< rodsServerHost_t* >( irods::RESOURCE_HOST, rods_host );
        if ( !get_err.ok() ) {
            return PASSMSG( "failed to call get_property", get_err );
        }

        if ( !rods_host->hostName ) {
            return ERROR( -1, "null rods server host" );
        }

        _resc_host = rods_host->hostName->name;

        // =-=-=-=-=-=-=-
        // TODO :: need to deal with remote open here

        // =-=-=-=-=-=-=-
        // stage the tar file so we can get at its tasty innards
        irods::error stage_err = stage_tar_struct_file( _struct_desc_index, _resc_host );
        if ( !stage_err.ok() ) {
            free_struct_file_desc( _struct_desc_index );
            return PASSMSG( "stage_tar_struct_file failed.", stage_err );
        }

        // =-=-=-=-=-=-=-
        // Win!
        return CODE( _struct_desc_index );

    } // tar_struct_file_open

    // =-=-=-=-=-=-=-
    // create the phy path to the cache dir
    irods::error compose_cache_dir_physical_path( char*       _phy_path,
            specColl_t* _spec_coll,
            const char* _sub_file_path ) {
        // =-=-=-=-=-=-=-
        // subFilePath is composed by appending path to the objPath which is
        // the logical path of the tar file. So we need to substitude the
        // objPath with cacheDir
        int len = strlen( _spec_coll->collection );
        if ( strncmp( _spec_coll->collection, _sub_file_path, len ) != 0 ) {
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
        for ( int i = 1; i < NUM_TAR_SUB_FILE_DESC; i++ ) {
            if ( PluginTarSubFileDesc[i].inuseFlag == FD_FREE ) {
                PluginTarSubFileDesc[i].inuseFlag = FD_INUSE;
                return i;
            };
        }

        rodsLog( LOG_NOTICE,
                 "alloc_tar_sub_file_desc: out of PluginTarSubFileDesc" );

        return SYS_OUT_OF_FILE_DESC;

    } // alloc_tar_sub_file_desc

    // =-=-=-=-=-=-=-
    // free an entry in the tar desc table
    int free_tar_sub_file_desc( int _idx ) {
        if ( _idx < 0 || _idx >= NUM_TAR_SUB_FILE_DESC ) {
            rodsLog( LOG_NOTICE,
                     "free_tar_sub_file_desc: index %d out of range", _idx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        memset( &PluginTarSubFileDesc[ _idx ], 0, sizeof( tarSubFileDesc_t ) );

        return 0;
    }

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    irods::error tar_file_create_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_create_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_create_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err = tar_struct_file_open( comm, spec_coll, struct_file_index,
                                fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_create_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if ( sub_index < 0 ) {
            return ERROR( sub_index, "tar_file_create_plugin - alloc_tar_sub_file_desc failed." );
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
        irods::error comp_err = compose_cache_dir_physical_path( fileCreateInp.fileName,
                                spec_coll,
                                fco->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG( "compose_cache_dir_physical_path failed.", comp_err );

        }

        fileCreateInp.mode       = fco->mode();
        fileCreateInp.flags      = fco->flags();
        fileCreateInp.otherFlags = NO_CHK_PERM_FLAG; // JMC - backport 4768
        snprintf( fileCreateInp.addr.hostAddr, sizeof( fileCreateInp.addr.hostAddr ), "%s", resc_host.c_str() );
        snprintf( fileCreateInp.resc_hier_, sizeof( fileCreateInp.resc_hier_ ), "%s", fco->resc_hier().c_str() );
        snprintf( fileCreateInp.objPath, sizeof( fileCreateInp.objPath ), "%s", fco->logical_path().c_str() );

        // =-=-=-=-=-=-=-
        // make the call to create a file
        fileCreateOut_t* create_out = NULL;
        int status = rsFileCreate( comm, &fileCreateInp, &create_out );
        free( create_out );
        if ( status < 0 ) {
            std::stringstream msg;
            msg << "tar_file_create_plugin - rsFileCreate failed for [";
            msg << fileCreateInp.fileName;
            msg << "], status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }
        else {
            PluginTarSubFileDesc[ sub_index ].fd = status;
            PluginStructFileDesc[ struct_file_index ].openCnt++;
            fco->file_descriptor( sub_index );
            return CODE( sub_index );
        }

    } // tar_file_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error tar_file_open_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_open_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_open_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_open_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if ( sub_index < 0 ) {
            return ERROR( sub_index, "tar_file_open_plugin - alloc_tar_sub_file_desc failed." );
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
        irods::error comp_err = compose_cache_dir_physical_path( fileOpenInp.fileName, spec_coll, fco->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG( "compose_cache_dir_physical_path failed.", comp_err );

        }

        fileOpenInp.mode       = fco->mode();
        fileOpenInp.flags      = fco->flags();
        fileOpenInp.otherFlags = NO_CHK_PERM_FLAG; // JMC - backport 4768
        snprintf( fileOpenInp.addr.hostAddr, sizeof( fileOpenInp.addr.hostAddr ), "%s", resc_host.c_str() );
        snprintf( fileOpenInp.resc_hier_, sizeof( fileOpenInp.resc_hier_ ), "%s", fco->resc_hier().c_str() );
        snprintf( fileOpenInp.objPath, sizeof( fileOpenInp.objPath ), "%s", fco->logical_path().c_str() );

        // =-=-=-=-=-=-=-
        // make the call to create a file
        int status = rsFileOpen( comm, &fileOpenInp );
        if ( status < 0 ) {
            std::stringstream msg;
            msg << "tar_file_open_plugin - rsFileOpen failed for [";
            msg << fileOpenInp.fileName;
            msg << "], status = ";
            msg << status;
            return ERROR( status, msg.str() );
        }
        else {
            PluginTarSubFileDesc[ sub_index ].fd = status;
            PluginStructFileDesc[ struct_file_index ].openCnt++;
            fco->file_descriptor( sub_index );
            return CODE( sub_index );
        }

    } // tar_file_open_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    irods::error tar_file_read_plugin(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_read_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if ( fco->file_descriptor() < 1                      ||
                fco->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
                PluginTarSubFileDesc[ fco->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tar_file_read_plugin - sub file index ";
            msg << fco->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a read structure and make the rs call
        fileReadInp_t fileReadInp;
        bytesBuf_t fileReadOutBBuf;
        memset( &fileReadInp, 0, sizeof( fileReadInp ) );
        memset( &fileReadOutBBuf, 0, sizeof( fileReadOutBBuf ) );
        fileReadInp.fileInx = PluginTarSubFileDesc[ fco->file_descriptor() ].fd;
        fileReadInp.len     = _len;
        fileReadOutBBuf.buf = _buf;

        // =-=-=-=-=-=-=-
        // make the call to read a file
        int status = rsFileRead( fco->comm(), &fileReadInp, &fileReadOutBBuf );
        if ( status < 0 ) {
            return ERROR( status, "rsFileRead failed" );
        }
        else {
            return CODE( status );
        }

    } // tar_file_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    irods::error tar_file_write_plugin(
        irods::resource_plugin_context& _ctx,
        void*                               _buf,
        int                                 _len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_write_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if ( fco->file_descriptor() < 1                      ||
                fco->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
                PluginTarSubFileDesc[ fco->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tar_file_write_plugin - sub file index ";
            msg << fco->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a write structure and make the rs call
        fileWriteInp_t fileWriteInp;
        bytesBuf_t     fileWriteOutBBuf;
        memset( &fileWriteInp,     0, sizeof( fileWriteInp ) );
        memset( &fileWriteOutBBuf, 0, sizeof( fileWriteOutBBuf ) );
        fileWriteInp.len     = fileWriteOutBBuf.len = _len;
        fileWriteInp.fileInx = PluginTarSubFileDesc[ fco->file_descriptor() ].fd;
        fileWriteOutBBuf.buf = _buf;

        // =-=-=-=-=-=-=-
        // make the write api call
        int status = rsFileWrite( fco->comm(), &fileWriteInp, &fileWriteOutBBuf );
        if ( status > 0 ) {
            // =-=-=-=-=-=-=-
            // cache has been written
            int         struct_idx = PluginTarSubFileDesc[ fco->file_descriptor() ].structFileInx;
            specColl_t* spec_coll   = PluginStructFileDesc[ struct_idx ].specColl;
            if ( spec_coll->cacheDirty == 0 ) {
                spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( fco->comm(), spec_coll, 0 );
                if ( status1 < 0 ) {
                    return CODE( status1 );
                }
            }
            return CODE( status );
        }
        else {
            return ERROR( status, "rsFileWrite failed" );
        }

    } // tar_file_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    irods::error tar_file_close_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_close_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if ( fco->file_descriptor() < 1                      ||
                fco->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
                PluginTarSubFileDesc[ fco->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tar_file_close_plugin - sub file index ";
            msg << fco->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a close structure and make the rs call
        fileCloseInp_t fileCloseInp;
        memset( &fileCloseInp, 0, sizeof( fileCloseInp ) );
        fileCloseInp.fileInx = PluginTarSubFileDesc[ fco->file_descriptor() ].fd;
        int status = rsFileClose( fco->comm(), &fileCloseInp );
        if ( status < 0 ) {
            std::stringstream msg;
            msg << "tar_file_close_plugin - failed in rsFileClose for fd [ ";
            msg << fco->file_descriptor();
            msg << " ]";
            return ERROR( status, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // close out the sub file allocation and free the space
        int struct_file_index = PluginTarSubFileDesc[ fco->file_descriptor() ].structFileInx;
        PluginStructFileDesc[ struct_file_index ].openCnt++;
        free_tar_sub_file_desc( fco->file_descriptor() );
        fco->file_descriptor( 0 );

        return CODE( status );
    } // tar_file_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    irods::error tar_file_unlink_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_unlink_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_unlink_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_unlink_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_unlink_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // build a file unlink structure to pass off to the server api call
        fileUnlinkInp_t fileUnlinkInp;
        memset( &fileUnlinkInp, 0, sizeof( fileUnlinkInp ) );
        snprintf( fileUnlinkInp.rescHier, sizeof( fileUnlinkInp.rescHier ), "%s", fco->resc_hier().c_str() );
        snprintf( fileUnlinkInp.objPath, sizeof( fileUnlinkInp.objPath ), "%s", fco->logical_path().c_str() );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        irods::error comp_err = compose_cache_dir_physical_path( fileUnlinkInp.fileName,
                                spec_coll,
                                fco->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG(
                       "tar_file_unlink_plugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        snprintf( fileUnlinkInp.addr.hostAddr, sizeof( fileUnlinkInp.addr.hostAddr ), "%s", resc_host.c_str() );

        // =-=-=-=-=-=-=-
        // make the call to unlink a file
        int status = rsFileUnlink( comm, &fileUnlinkInp );
        if ( status >= 0 ) {
            /* cache has been written */
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            if ( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if ( status1 < 0 ) {
                    return CODE( status1 );
                }
            }
            return CODE( status );
        }
        else {
            return ERROR( status, "rsFileUnlink failed" );
        }


    } // tar_file_unlink_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    irods::error tar_file_stat_plugin(
        irods::resource_plugin_context& _ctx,
        struct stat*                        _statbuf ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_stat_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_stat_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_stat_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_stat_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;


        // =-=-=-=-=-=-=-
        // build a file stat structure to pass off to the server api call
        fileStatInp_t fileStatInp;
        memset( &fileStatInp, 0, sizeof( fileStatInp ) );
        snprintf( fileStatInp.rescHier, MAX_NAME_LEN, "%s", fco->resc_hier().c_str() );
        snprintf( fileStatInp.objPath, MAX_NAME_LEN, "%s", fco->logical_path().c_str() );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        irods::error comp_err = compose_cache_dir_physical_path( fileStatInp.fileName, spec_coll, fco->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG(
                       "tar_file_stat_plugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        snprintf( fileStatInp.addr.hostAddr, NAME_LEN, "%s", resc_host.c_str() );
        snprintf( fileStatInp.rescHier, MAX_NAME_LEN, "%s", fco->resc_hier().c_str() );

        // =-=-=-=-=-=-=-
        // make the call to stat a file
        rodsStat_t* rods_stat;
        int status = rsFileStat( comm, &fileStatInp, &rods_stat );
        if ( status >= 0 ) {
            rodsStatToStat( _statbuf, rods_stat );
        }
        else {
            return ERROR( status, "tar_file_stat_plugin - rsFileStat failed." );
        }
        free( rods_stat );

        return CODE( status );

    } // tar_file_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    irods::error tar_file_lseek_plugin(
        irods::resource_plugin_context& _ctx,
        long long                        _offset,
        int                              _whence ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_lseek_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if ( fco->file_descriptor() < 1                      ||
                fco->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
                PluginTarSubFileDesc[ fco->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tar_file_lseek_plugin - sub file index ";
            msg << fco->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_lseek_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // build a lseek structure and make the rs call
        fileLseekInp_t fileLseekInp;
        memset( &fileLseekInp, 0, sizeof( fileLseekInp ) );
        fileLseekInp.fileInx = PluginTarSubFileDesc[ fco->file_descriptor() ].fd;
        fileLseekInp.offset  = _offset;
        fileLseekInp.whence  = _whence;

        fileLseekOut_t *fileLseekOut = NULL;
        int status = rsFileLseek( comm, &fileLseekInp, &fileLseekOut );

        if ( status < 0 || NULL == fileLseekOut ) { // JMC cppcheck - nullptr
            return ERROR( status, "rsFileLseek failed" );
        }
        else {
            rodsLong_t offset = fileLseekOut->offset;
            free( fileLseekOut );
            return CODE( offset );
        }

    } // tar_file_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error tar_file_mkdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_mkdir_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_mkdir_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_mkdir_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_mkdir_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // build a file mkdir structure to pass off to the server api call
        fileMkdirInp_t fileMkdirInp;
        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        snprintf( fileMkdirInp.addr.hostAddr, sizeof( fileMkdirInp.addr.hostAddr ), "%s", resc_host.c_str() );
        snprintf( fileMkdirInp.rescHier, sizeof( fileMkdirInp.rescHier ), "%s", spec_coll->rescHier );
        fileMkdirInp.mode = fco->mode();

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        irods::error comp_err = compose_cache_dir_physical_path( fileMkdirInp.dirName,
                                spec_coll,
                                fco->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG(
                       "tar_file_mkdir_plugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        // =-=-=-=-=-=-=-
        // make the call to the mkdir api
        int status = rsFileMkdir( comm, &fileMkdirInp );
        if ( status >= 0 ) {
            // use the specColl in PluginStructFileDesc
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written
            if ( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "tar_file_mkdir_plugin - Failed to call modCollInfo2" );
                }
            }

            return CODE( status );

        }
        else {
            return ERROR( status, "rsFileMkdir failed" );
        }

    } // tar_file_mkdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error tar_file_rmdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_rmdir_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_rmdir_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_rmdir_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_rmdir_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // build a file mkdir structure to pass off to the server api call
        fileRmdirInp_t fileRmdirInp;
        memset( &fileRmdirInp, 0, sizeof( fileRmdirInp ) );
        snprintf( fileRmdirInp.addr.hostAddr, NAME_LEN, "%s", resc_host.c_str() );
        snprintf( fileRmdirInp.rescHier, MAX_NAME_LEN, "%s", spec_coll->rescHier );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        irods::error comp_err = compose_cache_dir_physical_path( fileRmdirInp.dirName,
                                spec_coll,
                                fco->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG(
                       "tar_file_rmdir_plugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        // =-=-=-=-=-=-=-
        // make the call to the mkdir api
        int status = rsFileRmdir( comm, &fileRmdirInp );
        if ( status >= 0 ) {
            // use the specColl in PluginStructFileDesc
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written
            if ( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "tar_file_rmdir_plugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

    } // tar_file_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    irods::error tar_file_opendir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_opendir_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_opendir_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_opendir_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_opendir_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            irods::error ret = PASSMSG( msg.str(), open_err );
            irods::log( ret );
            return ret;
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_opendir_plugin - null spec_coll pointer in PluginStructFileDesc" );
        }

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if ( sub_index < 0 ) {
            return ERROR( sub_index, "tar_file_opendir_plugin - alloc_tar_sub_file_desc failed." );
        }

        // =-=-=-=-=-=-=-
        // build a file open structure to pass off to the server api call
        fileOpendirInp_t fileOpendirInp;
        memset( &fileOpendirInp, 0, sizeof( fileOpendirInp ) );
        snprintf( fileOpendirInp.addr.hostAddr, sizeof( fileOpendirInp.addr.hostAddr ), "%s", resc_host.c_str() );
        snprintf( fileOpendirInp.objPath, sizeof( fileOpendirInp.objPath ), "%s",       fco->logical_path().c_str() );
        snprintf( fileOpendirInp.resc_hier_, sizeof( fileOpendirInp.resc_hier_ ), "%s",    fco->resc_hier().c_str() );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        irods::error comp_err = compose_cache_dir_physical_path( fileOpendirInp.dirName,
                                spec_coll,
                                fco->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG(
                       "tarFileRmdirPlugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        // =-=-=-=-=-=-=-
        // make the api call to open a directory
        int status = rsFileOpendir( comm, &fileOpendirInp );
        if ( status < 0 ) {
            std::stringstream msg;
            msg << "tar_file_opendir_plugin - error returned from rsFileOpendir for: [";
            msg << fileOpendirInp.dirName;
            msg << "], status: ";
            msg << status;
            irods::error ret = ERROR( status, msg.str() );
            irods::log( ret );
            return ret;
        }
        else {
            PluginTarSubFileDesc[ sub_index ].fd = status;
            PluginStructFileDesc[ struct_file_index ].openCnt++;
            fco->file_descriptor( sub_index );

            return CODE( sub_index );
        }

    } // tar_file_opendir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    irods::error tar_file_closedir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_closedir_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // extract the fco
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if ( fco->file_descriptor() < 1                      ||
                fco->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
                PluginTarSubFileDesc[ fco->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tar_file_closedir_plugin - sub file index ";
            msg << fco->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a file close dir structure to pass off to the server api call
        fileClosedirInp_t fileClosedirInp;
        memset( &fileClosedirInp, 0, sizeof( fileClosedirInp ) );
        fileClosedirInp.fileInx = PluginTarSubFileDesc[ fco->file_descriptor() ].fd;
        int status = rsFileClosedir( _ctx.comm(), &fileClosedirInp );
        if ( status < 0 ) {
            return ERROR( status, "tar_file_closedir_plugin - failed on call to rsFileClosedir" );
        }

        // =-=-=-=-=-=-=-
        // close out the sub file index and free the allocation
        int struct_file_index = PluginTarSubFileDesc[ fco->file_descriptor() ].structFileInx;
        PluginStructFileDesc[ struct_file_index ].openCnt++;
        free_tar_sub_file_desc( fco->file_descriptor() );
        fco->file_descriptor( 0 );

        return CODE( status );

    } // tar_file_closedir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    irods::error tar_file_readdir_plugin(
        irods::resource_plugin_context& _ctx,
        struct rodsDirent**              _dirent_ptr ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_readdir_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // extract the fco
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // check range on the sub file index
        if ( fco->file_descriptor() < 1                      ||
                fco->file_descriptor() >= NUM_TAR_SUB_FILE_DESC ||
                PluginTarSubFileDesc[ fco->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tar_file_readdir_plugin - sub file index ";
            msg << fco->file_descriptor();
            msg << " is out of range.";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a file read dir structure to pass off to the server api call
        fileReaddirInp_t fileReaddirInp;
        memset( &fileReaddirInp, 0, sizeof( fileReaddirInp ) );
        fileReaddirInp.fileInx = PluginTarSubFileDesc[ fco->file_descriptor() ].fd;

        // =-=-=-=-=-=-=-
        // make the api call to read the directory
        int status = rsFileReaddir( _ctx.comm(), &fileReaddirInp, _dirent_ptr );
        if ( status < -1 ) {
            return ERROR( status, "tar_file_readdir_plugin - failed in call to rsFileReaddir" );
        }

        return CODE( status );

    } // tar_file_readdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX rename
    irods::error tar_file_rename_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                     _new_file_name ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_rename_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_rename_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_rename_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_rename_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // build a file rename structure to pass off to the server api call
        fileRenameInp_t fileRenameInp;
        memset( &fileRenameInp, 0, sizeof( fileRenameInp ) );
        snprintf( fileRenameInp.addr.hostAddr, NAME_LEN, "%s", resc_host.c_str() );
        snprintf( fileRenameInp.rescHier, MAX_NAME_LEN, "%s", fco->resc_hier().c_str() );
        snprintf( fileRenameInp.objPath, MAX_NAME_LEN, "%s", fco->logical_path().c_str() );

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        irods::error comp_err_old = compose_cache_dir_physical_path( fileRenameInp.oldFileName,
                                    spec_coll,
                                    fco->sub_file_path().c_str() );
        if ( !comp_err_old.ok() ) {
            return PASSMSG(
                       "tar_file_rename_plugin - compose_cache_dir_physical_path failed for old file name.",
                       comp_err_old );
        }

        irods::error comp_err_new = compose_cache_dir_physical_path( fileRenameInp.newFileName,
                                    spec_coll,
                                    _new_file_name );
        if ( !comp_err_new.ok() ) {
            return PASSMSG(
                       "tar_file_rename_plugin - compose_cache_dir_physical_path failed for new file name.",
                       comp_err_new );
        }

        // =-=-=-=-=-=-=-
        // make the api call for rename
        fileRenameOut_t* ren_out = 0;
        int status = rsFileRename( comm, &fileRenameInp, &ren_out );
        free( ren_out );
        if ( status >= 0 ) {
            // use the specColl in PluginStructFileDesc
            specColl_t* loc_spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written
            if ( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "tar_file_rename_plugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

    } // tar_file_rename_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    irods::error tar_file_truncate_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_truncate_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr struct_obj = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_truncate_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_truncate_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index,
                                 struct_obj->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_truncate_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if ( sub_index < 0 ) {
            return ERROR( sub_index, "tar_file_truncate_plugin - alloc_tar_sub_file_desc failed." );
        }

        // =-=-=-=-=-=-=-
        // cache struct file index into sub file index
        PluginTarSubFileDesc[ sub_index ].structFileInx = struct_file_index;

        // =-=-=-=-=-=-=-
        // build a file create structure to pass off to the server api call
        fileOpenInp_t fileTruncateInp;
        memset( &fileTruncateInp, 0, sizeof( fileTruncateInp ) );
        snprintf( fileTruncateInp.addr.hostAddr, sizeof( fileTruncateInp.addr.hostAddr ), "%s", resc_host.c_str() );
        snprintf( fileTruncateInp.objPath, sizeof( fileTruncateInp.objPath ), "%s", struct_obj->logical_path().c_str() );
        fileTruncateInp.dataSize = struct_obj->offset();

        // =-=-=-=-=-=-=-
        // build a physical path name to the cache dir
        irods::error comp_err = compose_cache_dir_physical_path( fileTruncateInp.fileName,
                                spec_coll,
                                struct_obj->sub_file_path().c_str() );
        if ( !comp_err.ok() ) {
            return PASSMSG(
                       "tar_file_truncate_plugin - compose_cache_dir_physical_path failed.", comp_err );
        }

        // =-=-=-=-=-=-=-
        // make the truncate api call
        int status = rsFileTruncate( comm, &fileTruncateInp );
        if ( status > 0 ) {
            // =-=-=-=-=-=-=-
            // cache has been written
            int         struct_idx = PluginTarSubFileDesc[ struct_obj->file_descriptor() ].structFileInx;
            specColl_t* spec_coll   = PluginStructFileDesc[ struct_idx ].specColl;
            if ( spec_coll->cacheDirty == 0 ) {
                spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( struct_obj->comm(), spec_coll, 0 );
                if ( status1 < 0 ) {
                    return CODE( status1 );
                }
            }
        }

        return CODE( status );

    } // tar_file_truncate_plugin

    // =-=-=-=-=-=-=-
    // interface to extract a tar file
    irods::error tar_file_extract_plugin(
        irods::resource_plugin_context& _ctx ) {

        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_extract_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_extract_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_extract_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // allocate a position in the struct table
        int struct_file_index = 0;
        if ( ( struct_file_index = alloc_struct_file_desc() ) < 0 ) {
            return ERROR( struct_file_index, "tar_file_extract_plugin - failed to allocate struct file descriptor" );
        }

        // =-=-=-=-=-=-=-
        // populate the file descriptor table
        PluginStructFileDesc[ struct_file_index ].inuseFlag = 1;
        PluginStructFileDesc[ struct_file_index ].specColl  = spec_coll;
        PluginStructFileDesc[ struct_file_index ].rsComm    = comm;
        snprintf( PluginStructFileDesc[ struct_file_index ].dataType,
                  sizeof( PluginStructFileDesc[ struct_file_index ].dataType ),
                  "%s", fco->data_type().c_str() );

        // =-=-=-=-=-=-=-
        // extract the file
        irods::error ext_err = extract_file( struct_file_index );
        if ( !ext_err.ok() ) {
            // NOTE:: may need to remove the cacheDir too
            std::stringstream msg;
            msg << "tar_file_extract_plugin - failed to extact structure file for [";
            msg << spec_coll->objPath;
            msg << "] in directory [";
            msg << spec_coll->cacheDir;
            msg << "] with errno of ";
            msg << errno;
            return PASSMSG( msg.str(), ext_err );
        }

        // =-=-=-=-=-=-=-
        // if the left over cache dir has a symlink in it, remove the
        // directory ( addresses Wisc Security Issue r4906 )
        if ( hasSymlinkInDir( spec_coll->cacheDir ) ) {
            rodsLog( LOG_ERROR, "extractTarFile: cacheDir %s has symlink in it",
                     spec_coll->cacheDir );

            rodsHostAddr_t* host_addr = NULL;
            _ctx.prop_map().get< rodsHostAddr_t* >( irods::RESOURCE_LOCATION, host_addr );

            if ( NULL == host_addr ) {
                return ERROR( SYS_INTERNAL_NULL_INPUT_ERR, "host_addr is null in rmDir" );
            }
            /* remove cache */
            fileRmdirInp_t fileRmdirInp;
            memset( &fileRmdirInp, 0, sizeof( fileRmdirInp ) );
            rstrcpy( fileRmdirInp.dirName,       spec_coll->cacheDir, MAX_NAME_LEN );
            rstrcpy( fileRmdirInp.addr.hostAddr, host_addr->hostAddr, NAME_LEN );
            rstrcpy( fileRmdirInp.rescHier,      spec_coll->rescHier, MAX_NAME_LEN );

            fileRmdirInp.flags = RMDIR_RECUR;
            int status = rsFileRmdir( comm, &fileRmdirInp );
            if ( status < 0 ) {
                std::stringstream msg;
                msg << "tar_file_extract_plugin - rmdir error for [";
                msg << spec_coll->cacheDir << "]";
                return ERROR( status, msg.str() );
            }

        } // if hasSymlinkInDir

        return CODE( ext_err.code() );

    } // tar_file_extract_plugin

    // =-=-=-=-=-=-=-
    // helper function to write an archive entry
    irods::error write_file_to_archive( const boost::filesystem::path _path,
                                        const std::string&            _cache_dir,
                                        struct archive*               _archive ) {
        namespace fs = boost::filesystem;
        struct archive_entry* entry = archive_entry_new();

        // =-=-=-=-=-=-=-
        // strip arch path from file name for header entry
        std::string path_name  = _path.string();
        std::string strip_file = path_name.substr( _cache_dir.size() + 1 ); // add one for the last '/'
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
        if ( archive_write_header( _archive, entry ) != ARCHIVE_OK ) {
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
        if ( -1 == fd )  {
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
        while ( len > 0 ) {
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
    irods::error build_directory_listing( const boost::filesystem::path&          _path,
                                          std::vector< boost::filesystem::path >& _listing ) {
        // =-=-=-=-=-=-=-
        // namespace alias for brevity
        namespace fs = boost::filesystem;
        irods::error final_error = SUCCESS();

        // =-=-=-=-=-=-=-
        // begin iterating over the cache dir
        try {
            if ( fs::is_directory( _path ) ) {
                // =-=-=-=-=-=-=-
                // iterate over the directory and archive it
                fs::directory_iterator end_iter;
                fs::directory_iterator dir_itr( _path );
                for ( ; dir_itr != end_iter; ++dir_itr ) {
                    // =-=-=-=-=-=-=-
                    // recurse on this new directory
                    irods::error ret = build_directory_listing( dir_itr->path(), _listing );
                    if ( !ret.ok() ) {
                        std::stringstream msg;
                        msg << "build_directory_listing - failed on [";
                        msg << dir_itr->path().string();
                        msg << "]";
                        final_error = PASSMSG( msg.str(), final_error );
                    }

                } // for dir_itr

            }
            else if ( fs::is_regular_file( _path ) ) {
                // =-=-=-=-=-=-=-
                // add file path to the vector
                _listing.push_back( _path );

            }
            else {
                std::stringstream msg;
                msg << "build_directory_listing - unhandled entry [";
                msg << _path.filename();
                msg << "]";
                rodsLog( LOG_NOTICE, msg.str().c_str() );
            }

        }
        catch ( const std::exception& ex ) {
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
    irods::error bundle_cache_dir( int         _index,
                                   std::string _data_type ) {
        // =-=-=-=-=-=-=-
        // namespace alias for brevity
        namespace fs = boost::filesystem;

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        if ( PluginStructFileDesc[ _index ].inuseFlag <= 0 ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - struct file index: ";
            msg << _index;
            msg << " is not in use";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        if ( spec_coll == NULL                   ||
                spec_coll->cacheDirty <= 0          ||
                strlen( spec_coll->cacheDir ) == 0  ||
                strlen( spec_coll->phyPath ) == 0 ) {
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
        if ( !fs::exists( full_path ) ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - cache directory does not exist [";
            msg << spec_coll->cacheDir;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // make sure it is a directory, just in case
        if ( !fs::is_directory( full_path ) ) {
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
        if ( !arch ) {
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
        if ( _data_type == ZIP_DT_STR ) {
            if ( archive_write_add_filter_lzip( arch ) != ARCHIVE_OK ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to set compression to lzip for archive [";
                msg << spec_coll->phyPath;
                msg << "] with error string [";
                msg << archive_error_string( arch );
                msg << "]";
                return ERROR( -1, msg.str() );

            }

        }
        else if ( _data_type == GZIP_TAR_DT_STR ) {
            if ( archive_write_add_filter_gzip( arch ) != ARCHIVE_OK ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to set compression to gzip for archive [";
                msg << spec_coll->phyPath;
                msg << "] with error string [";
                msg << archive_error_string( arch );
                msg << "]";
                return ERROR( -1, msg.str() );

            }

        }
        else if ( _data_type == BZIP2_TAR_DT_STR ) {
            if ( archive_write_add_filter_bzip2( arch ) != ARCHIVE_OK ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to set compression to bzip2 for archive [";
                msg << spec_coll->phyPath;
                msg << "] with error string [";
                msg << archive_error_string( arch );
                msg << "]";
                return ERROR( -1, msg.str() );

            }

        }
        else {
            if ( archive_write_add_filter_none( arch ) != ARCHIVE_OK ) {
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
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( spec_coll->rescHier, location );
        if ( !ret.ok() ) {
            return PASSMSG( "bundle_cache_dir - failed in get_loc_for_hier_string", ret );
        }

        // =-=-=-=-=-=-=-
        // create a context to pass to the callbacks
        cb_ctx_t cb_ctx;
        memset( &cb_ctx, 0, sizeof( cb_ctx_t ) );
        cb_ctx.desc_ = &PluginStructFileDesc[ _index ];
        snprintf( cb_ctx.loc_, sizeof( cb_ctx.loc_ ), "%s", location.c_str() );

        // =-=-=-=-=-=-=-
        // open the spec coll physical path for archival
        if ( archive_write_open(
                    arch,
                    &cb_ctx,
                    irods_file_open_for_write,
                    irods_file_write,
                    irods_file_close ) < ARCHIVE_OK ) {
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
        irods::error arch_err = SUCCESS();
        for ( size_t i = 0; i < listing.size(); ++i ) {
            // =-=-=-=-=-=-=-
            // strip off archive path from the filename
            irods::error ret = write_file_to_archive( listing[ i ].string(), cache_dir, arch );

            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << "bundle_cache_dir - failed to archive file [";
                msg << listing[ i ].string();
                msg << "]";
                arch_err = PASSMSG( msg.str(), arch_err );
                irods::log( PASSMSG( msg.str(), ret ) );
            }

        } // for i

        // =-=-=-=-=-=-=-
        // close the archive and clean up
        archive_write_close( arch );
        archive_write_free( arch );

        // =-=-=-=-=-=-=-
        // handle errors
        if ( !arch_err.ok() ) {
            std::stringstream msg;
            msg << "bundle_cache_dir - failed to archive [";
            msg << spec_coll->cacheDir;
            msg << "] into archive file [";
            msg << spec_coll->phyPath;
            msg << "]";
            return PASSMSG( msg.str(), arch_err );

        }
        else {
            return SUCCESS();

        }

    } // bundle_cache_dir

    // =-=-=-=-=-=-=-
    // interface for tar / zip up of cache dir which also updates
    // the icat with the new file size
    irods::error sync_cache_dir_to_tar_file( int         _index,
            int         _opr_type,
            std::string _host ) {
        specColl_t* spec_coll = PluginStructFileDesc[ _index ].specColl;
        rsComm_t*   comm      = PluginStructFileDesc[ _index ].rsComm;

        // =-=-=-=-=-=-=-
        // call bundle helper functions
        irods::error bundle_err = bundle_cache_dir( _index, PluginStructFileDesc[ _index ].dataType );
        if ( !bundle_err.ok() ) {
            return PASSMSG( "sync_cache_dir_to_tar_file - failed in bundle.", bundle_err );
        }

        // =-=-=-=-=-=-=-
        // create a file stat structure for the rs call
        fileStatInp_t file_stat_inp;
        memset( &file_stat_inp, 0, sizeof( file_stat_inp ) );
        rstrcpy( file_stat_inp.fileName, spec_coll->phyPath, MAX_NAME_LEN );
        snprintf( file_stat_inp.addr.hostAddr,  NAME_LEN,     "%s", _host.c_str() );
        snprintf( file_stat_inp.rescHier,       MAX_NAME_LEN, "%s", spec_coll->rescHier );
        snprintf( file_stat_inp.objPath,        MAX_NAME_LEN, "%s", spec_coll->objPath );

        // =-=-=-=-=-=-=-
        // call file stat api to get the size of the new file
        rodsStat_t* file_stat_out = NULL;
        int status = rsFileStat( comm, &file_stat_inp, &file_stat_out );
        if ( status < 0 || NULL ==  file_stat_out ) {
            std::stringstream msg;
            msg << "sync_cache_dir_to_tar_file - failed on call to rsFileStat for [";
            msg << spec_coll->phyPath;
            msg << "] with status of ";
            msg << status;
            return ERROR( status, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // update icat with the new size of the file
        if ( ( _opr_type & NO_REG_COLL_INFO ) == 0 ) {
            // NOTE :: for bundle opr, done at datObjClose
            status = regNewObjSize( comm, spec_coll->objPath, spec_coll->replNum, file_stat_out->st_size );

        }

        free( file_stat_out );

        return CODE( status );

    } // sync_cache_dir_to_tar_file

    // =-=-=-=-=-=-=-
    // interface for sync up of cache dir
    irods::error tar_file_sync_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "tar_file_sync_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast< irods::structured_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = fco->spec_coll();
        if ( !spec_coll ) {
            return ERROR( -1, "tar_file_sync_plugin - null spec_coll pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = fco->comm();
        if ( !comm ) {
            return ERROR( -1, "tar_file_sync_plugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        std::string resc_host;
        irods::error open_err = tar_struct_file_open( comm, spec_coll, struct_file_index,
                                fco->resc_hier(), resc_host );
        if ( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tar_file_sync_plugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath;
            return PASSMSG( msg.str(), open_err );
        }

        // =-=-=-=-=-=-=-
        // copy the data type requested by user for compression
        snprintf( PluginStructFileDesc[ struct_file_index ].dataType, NAME_LEN, "%s", fco->data_type().c_str() );

        // =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed
        spec_coll = PluginStructFileDesc[ struct_file_index ].specColl;

        // =-=-=-=-=-=-=-
        // we cant sync if the structured collection is currently in use
        if ( PluginStructFileDesc[ struct_file_index ].openCnt > 0 ) {
            return ERROR( SYS_STRUCT_FILE_BUSY_ERR, "tar_file_sync_plugin - spec coll in use" );
        }

        // =-=-=-=-=-=-=-
        // delete operation
        if ( ( fco->opr_type() & DELETE_STRUCT_FILE ) != 0 ) {
            /* remove cache and the struct file */
            free_struct_file_desc( struct_file_index );
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // check if there is a specified cache directory,
        // if not then any other operation isn't possible
        if ( strlen( spec_coll->cacheDir ) > 0 ) {
            if ( spec_coll->cacheDirty > 0 ) {
                // =-=-=-=-=-=-=-
                // write the tar file and register no dirty
                irods::error sync_err = sync_cache_dir_to_tar_file( struct_file_index,
                                        fco->opr_type(),
                                        resc_host );
                if ( !sync_err.ok() ) {
                    std::stringstream msg;
                    msg << "tar_file_sync_plugin - failed in sync_cache_dir_to_tar_file for [";
                    msg << spec_coll->cacheDir;
                    msg << "] with status of ";
                    msg << sync_err.code() << "]";

                    free_struct_file_desc( struct_file_index );
                    return PASSMSG( msg.str(), sync_err );
                }

                spec_coll->cacheDirty = 0;
                if ( ( fco->opr_type() & NO_REG_COLL_INFO ) == 0 ) {
                    int status = modCollInfo2( comm,  spec_coll, 0 );
                    if ( status < 0 ) {
                        free_struct_file_desc( struct_file_index );
                        return ERROR( status, "tar_file_sync_plugin - failed in modCollInfo2" );

                    } // if status

                }

            } // if cache is dirty

            // =-=-=-=-=-=-=-
            // remove cache dir if necessary
            if ( ( fco->opr_type() & PURGE_STRUCT_FILE_CACHE ) != 0 ) {
                // =-=-=-=-=-=-=-
                // need to unregister the cache before removing it
                int status = modCollInfo2( comm,  spec_coll, 1 );
                if ( status < 0 ) {
                    free_struct_file_desc( struct_file_index );
                    return ERROR( status, "tar_file_sync_plugin - failed in modCollInfo2" );
                }

                // =-=-=-=-=-=-=-
                // use the irods api to remove the directory
                fileRmdirInp_t rmdir_inp;
                memset( &rmdir_inp, 0, sizeof( rmdir_inp ) );
                rmdir_inp.flags    = RMDIR_RECUR;
                rstrcpy( rmdir_inp.dirName,       spec_coll->cacheDir, MAX_NAME_LEN );
                snprintf( rmdir_inp.addr.hostAddr,  NAME_LEN,       "%s", resc_host.c_str() );
                snprintf( rmdir_inp.rescHier,       MAX_NAME_LEN,   "%s", spec_coll->rescHier );

                status = rsFileRmdir( comm, &rmdir_inp );
                if ( status < 0 ) {
                    free_struct_file_desc( struct_file_index );
                    return ERROR( status, "tar_file_sync_plugin - failed in call to rsFileRmdir" );
                }

            } // if purge file cache

        } // if we have a cache dir

        free_struct_file_desc( struct_file_index );

        return SUCCESS();

    } // tar_file_sync_plugin

    // =-=-=-=-=-=-=-
    // interface for getting freespace of the fs
    irods::error tar_file_getfsfreespace_plugin(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, "tar_file_getfsfreespace_plugin is not implemented" );

    } // tar_file_getfsfreespace_plugin

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    irods::error tar_file_registered_plugin(
        irods::resource_plugin_context& ) {
        // NOOP
        return SUCCESS();
    }

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    irods::error tar_file_unregistered_plugin(
        irods::resource_plugin_context& ) {
        // NOOP
        return SUCCESS();
    }

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    irods::error tar_file_modified_plugin(
        irods::resource_plugin_context& ) {
        // NOOP
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // tar_file_rebalance - code which would rebalance the subtree
    irods::error tar_file_rebalance(
        irods::resource_plugin_context& ) {
        return SUCCESS();

    } // tar_file_rebalance

    // =-=-=-=-=-=-=-
    // tar_file_notify - code which would notify the subtree of a change
    irods::error tar_file_notify(
        irods::resource_plugin_context&,
        const std::string* ) {
        return SUCCESS();

    } // tar_file_rebalance


    // =-=-=-=-=-=-=-
    // 3. create derived class to handle tar file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class tarfilesystem_resource : public irods::resource {
        public:
            tarfilesystem_resource(
                const std::string& _inst_name,
                const std::string& _context ) :
                irods::resource( _inst_name, _context ) {
            } // ctor

    }; // class tarfilesystem_resource

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the irods facing interface defined in
    //    server/drivers/src/fileDriver.c
    irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

        // =-=-=-=-=-=-=-
        // 4a. create tarfilesystem_resource
        tarfilesystem_resource* resc = new tarfilesystem_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b1. set start and stop operations for alloc / free of tables
        resc->set_start_operation( "tarfilesystem_resource_start" );
        resc->set_stop_operation( "tarfilesystem_resource_stop" );

        // =-=-=-=-=-=-=-
        // 4b2. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( irods::RESOURCE_OP_CREATE,       "tar_file_create_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,         "tar_file_open_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READ,         "tar_file_read_plugin" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,        "tar_file_write_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,        "tar_file_close_plugin" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,       "tar_file_unlink_plugin" );
        resc->add_operation( irods::RESOURCE_OP_STAT,         "tar_file_stat_plugin" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,        "tar_file_lseek_plugin" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,        "tar_file_mkdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,        "tar_file_rmdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,      "tar_file_opendir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,     "tar_file_closedir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,      "tar_file_readdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,       "tar_file_rename_plugin" );
        resc->add_operation( irods::RESOURCE_OP_TRUNCATE,     "tar_file_truncate_plugin" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,    "tar_file_getfsfreespace_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,   "tar_file_registered_plugin" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED, "tar_file_unregistered_plugin" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,     "tar_file_modified_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,    "tar_file_rebalance" );
        resc->add_operation( irods::RESOURCE_OP_NOTIFY,       "tar_file_notify" );

        // =-=-=-=-=-=-=-
        // struct file specific operations
        resc->add_operation( "extract",      "tar_file_extract_plugin" );
        resc->add_operation( "sync",         "tar_file_sync_plugin" );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     irods::resource pointer
        return dynamic_cast<irods::resource*>( resc );

    } // plugin_factory

}; // extern "C"



