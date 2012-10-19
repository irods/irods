


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

// =-=-=-=-=-=-=-
// system includes
#ifndef TAR_EXEC_PATH
#include <libtar.h>

#ifdef HAVE_LIBZ
# include <zlib.h>
#endif

// JMC - #include <compat.h>
#endif	/* TAR_EXEC_PATH */
#ifndef windows_platform
#include <sys/wait.h>
#else
#include "Unix2Nt.h"
#endif

// =-=-=-=-=-=-=-
// irods includes
#include "tarSubStructFileDriver.h"
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
#include "structFileDriver.h"
#include "tarSubStructFileDriver.h"


// =-=-=-=-=-=-=-=-
// 
structFileDesc_t StructFileDesc[ NUM_STRUCT_FILE_DESC  ];
tarSubFileDesc_t TarSubFileDesc[ NUM_TAR_SUB_FILE_DESC ];
extern eirods::resource_manager resc_mgr;

extern "C" {

    // =-=-=-=-=-=-=-
    // Begin Tar Operations used by libtar
    // this set of codes irodsTarXYZ are used by libtar to perform file level
    // I/O in iRODS 
    
    // =-=-=-=-=-=-=-
    // FIXME :: Black Magic that desperately needs fixed
    int encodeIrodsTarfdPlugin(int upperInt, int lowerInt) {
        /* use the upper 5 of the 6 bits for upperInt */

        if (upperInt > 60) {	/* 0x7c */
            rodsLog (LOG_NOTICE,
              "encodeIrodsTarfdPlugin: upperInt %d larger than 60", lowerInt);
            return (SYS_STRUCT_FILE_DESC_ERR);
        }

        if ((lowerInt & 0xfc000000) != 0) {
            rodsLog (LOG_NOTICE,
              "encodeIrodsTarfdPlugin: lowerInt %d too large", lowerInt);
            return (SYS_STRUCT_FILE_DESC_ERR);
        }

        return (lowerInt | (upperInt << 26));
        
    }
    
    // =-=-=-=-=-=-=-
    // FIXME :: Black Magic that desperately needs fixed
    int decodeIrodsTarfdPlugin(int inpInt, int *upperInt, int *lowerInt) {
        *lowerInt = inpInt & 0x03ffffff;
        *upperInt = (inpInt & 0x7c000000) >> 26;

        return (0);
    }


    int irodsTarOpenPlugin( char *pathname, int oflags, int mode ) {
        int structFileInx;
        int myMode;
        int status;
        fileOpenInp_t fileOpenInp;
        rescInfo_t *rescInfo;
        specColl_t *specColl = NULL;
        int rescTypeInx;
        int l3descInx;

        /* the upper most 4 bits of mode is the structFileInx */ 
        decodeIrodsTarfdPlugin(mode, &structFileInx, &myMode); 
        status = verifyStructFileDesc (structFileInx, pathname, &specColl);
        if (status < 0 || NULL == specColl ) return -1;	/* tar lib looks for -1 return */ // JMC cppcheck - nullptr

        rescInfo = StructFileDesc[structFileInx].rescInfo;
        rescTypeInx = rescInfo->rescTypeInx;

        memset (&fileOpenInp, 0, sizeof (fileOpenInp));
        fileOpenInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        rstrcpy (fileOpenInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);
        rstrcpy (fileOpenInp.fileName, specColl->phyPath, MAX_NAME_LEN);
        fileOpenInp.mode = myMode;
        fileOpenInp.flags = oflags;

        l3descInx = rsFileOpen( StructFileDesc[structFileInx].rsComm, &fileOpenInp );
        
        if (l3descInx < 0) {
            rodsLog (LOG_NOTICE, 
          "irodsTarOpen: rsFileOpen of %s in Resc %s error, status = %d",
          fileOpenInp.fileName, rescInfo->rescName, l3descInx);
            return (-1);	/* libtar only accept -1 */
        }
        return (encodeIrodsTarfdPlugin(structFileInx, l3descInx));
    } // irodsTarOpenPlugin

    int irodsTarClosePlugin( int fd ) {
        fileCloseInp_t fileCloseInp;
        int structFileInx, l3descInx;
        int status;

        decodeIrodsTarfdPlugin(fd, &structFileInx, &l3descInx);
        memset (&fileCloseInp, 0, sizeof (fileCloseInp));
        fileCloseInp.fileInx = l3descInx;
        status = rsFileClose (StructFileDesc[structFileInx].rsComm, 
          &fileCloseInp);

        return (status);
    } // irodsTarClosePlugin

    int irodsTarReadPlugin( int fd, char *buf, int len ) {
        fileReadInp_t fileReadInp;
        int structFileInx, l3descInx;
        int status;
        bytesBuf_t readOutBBuf;

        decodeIrodsTarfdPlugin(fd, &structFileInx, &l3descInx);
        memset (&fileReadInp, 0, sizeof (fileReadInp));
        fileReadInp.fileInx = l3descInx;
        fileReadInp.len = len;
        memset (&readOutBBuf, 0, sizeof (readOutBBuf));
        readOutBBuf.buf = buf;
        status = rsFileRead (StructFileDesc[structFileInx].rsComm, 
          &fileReadInp, &readOutBBuf);

        return (status);

    } // irodsTarReadPlugin

    int irodsTarWritePlugin( int fd, char *buf, int len ) {
        fileWriteInp_t fileWriteInp;
        int structFileInx, l3descInx;
        int status;
        bytesBuf_t writeInpBBuf;

        decodeIrodsTarfdPlugin(fd, &structFileInx, &l3descInx);
        memset (&fileWriteInp, 0, sizeof (fileWriteInp));
        fileWriteInp.fileInx = l3descInx;
        fileWriteInp.len = len;
        memset (&writeInpBBuf, 0, sizeof (writeInpBBuf));
        writeInpBBuf.buf = buf;
        status = rsFileWrite (StructFileDesc[structFileInx].rsComm, 
          &fileWriteInp, &writeInpBBuf);

        return (status);
    } // irodsTarWritePlugin

    tartype_t irodstype = { (openfunc_t) irodsTarOpenPlugin, (closefunc_t) irodsTarClosePlugin,
            (readfunc_t) irodsTarReadPlugin, (writefunc_t) irodsTarWritePlugin
    };
     
    // End Tar Operations
    // =-=-=-=-=-=-=-



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
            return ERROR( false, -1, msg );
		}

	} // param_check

    // =-=-=-=-=-=-=-
    // external program call to unzip and extract
    eirods::error extract_file_with_unzip( int _index ) {

        char *av[NAME_LEN];
        int status;
        int inx = 0;

        // =-=-=-=-=-=-=-
        // error check special collection
        specColl_t* spec_coll = StructFileDesc[ _index ].specColl;
        if( StructFileDesc[ _index ].inuseFlag <= 0 ) {
            std::stringstream msg;
            msg << "extract_file_with_unzip - index: ";
            msg << _index;
            msg << " is not in use.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // error check special collection
        if( spec_coll == NULL                 || 
            strlen( spec_coll->cacheDir ) == 0 || 
            strlen( spec_coll->phyPath  ) == 0 ) {
            std::stringstream msg;
            msg << "extract_file_with_unzip - bad special collection for index: ";
            msg << _index;
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // mwan? - cd to the cacheDir 
        // build executable string with parameters
        bzero (av, sizeof (av));
        av[inx] = (char*)UNZIP_EXEC_PATH;
        inx++;
        av[inx] = (char*)( "-q" );
        inx++;
        av[inx] = (char*)( "-d" );
        inx++;
        av[inx] = spec_coll->cacheDir;
        inx++;
        av[inx] = spec_coll->phyPath;
        
        // =-=-=-=-=-=-=-
        // make the call to the external program
        if( status = forkAndExec (av) >= 0 ) {
            return CODE( status );
        } else {
            return ERROR( false, status, "extract_file_with_unzip - forkAndExec failed." );
        }

    } // extract_file_with_unzip

    // =-=-=-=-=-=-=-
    // extract the contents of a tar file using libtar
    eirods::error extract_tar_file_with_lib( int _index ) {

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        if( StructFileDesc[ _index ].inuseFlag <= 0 ) {
            std::stringstream msg;
            msg << "extract_tar_file_with_lib - struct file index: ";
            msg << _index;
            msg << " is not in use";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // check struct desc table in use flag
        specColl_t* spec_coll = StructFileDesc[ _index ].specColl;
        if( spec_coll == NULL                  || 
            strlen( spec_coll->cacheDir ) == 0 ||
            strlen( spec_coll->phyPath  ) == 0) {
            std::stringstream msg;
            msg << "extract_tar_file_with_lib - bad special collection for index: ";
            msg << _index;
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // encode file mode?  default is 0600 Black Magic...
        int myMode = encodeIrodsTarfdPlugin( _index, getDefFileMode() );
        if( myMode < 0 ) {
            return ERROR( false, myMode, "extract_tar_file_with_lib - failed in encodeIrodsTarfdPlugin" );
        }

        // =-=-=-=-=-=-=-
        // make call to tar open
        TAR* tar_handle = 0;
        int status = tar_open( &tar_handle, spec_coll->phyPath, &irodstype, O_RDONLY, myMode, TAR_GNU );
        if( status < 0 ) {
              std::stringstream msg;
              msg << "extract_tar_file_with_lib - tar_open error for [";
              msg << spec_coll->phyPath;
              msg << "], errno = ";
              msg << errno;
            return ERROR( false, SYS_TAR_OPEN_ERR - errno, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // make call to extract
        status = tar_extract_all( tar_handle, spec_coll->cacheDir );
        if( status != 0 ) {
              std::stringstream msg;
              msg << "extract_tar_file_with_lib - tar_extract_all error for [";
              msg << spec_coll->phyPath;
              msg << "], errno = ";
              msg << errno;
            return ERROR( false, SYS_TAR_EXTRACT_ALL_ERR - errno, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // make call to close
        status = tar_close( tar_handle );
        if( status != 0 ) {
            std::stringstream msg;
            msg << "extract_tar_file_with_lib - tar_close error for [";
            msg << spec_coll->phyPath; 
            msg << ", errno = ";
            msg << errno;
            return ERROR( false, SYS_TAR_CLOSE_ERR - errno, msg.str() );
        }

        return SUCCESS();

    } // extract_tar_file_with_lib

    // =-=-=-=-=-=-=-
    // call tar file extraction for struct file 
    eirods::error extract_tar_file( int _index ) {
       
        std::string type = StructFileDesc[ _index ].dataType;
        if( type == ZIP_DT_STR ) {
            return extract_file_with_unzip( _index );
        } else {
            return extract_tar_file_with_lib( _index );
        } 

    } // extract_tar_file
    
    // =-=-=-=-=-=-=-
    //  
    eirods::error make_tar_cache_dir( int _index ) {

        // =-=-=-=-=-=-=-
        // extract and test comm pointer
        rsComm_t* rs_comm = StructFileDesc[ _index ].rsComm;
        if( !rs_comm ) {
            std::stringstream msg;
            msg << "make_tar_cache_dir - null rsComm pointer for index: ";
            msg << _index;
            return ERROR( false, SYS_INTERNAL_NULL_INPUT_ERR, msg.str() );
        }
        
        // =-=-=-=-=-=-=-
        // extract and test special collection pointer
        specColl_t* spec_coll = StructFileDesc[ _index ].specColl;
        if( !spec_coll ) {
            std::stringstream msg;
            msg << "make_tar_cache_dir - null specColl pointer for index: ";
            msg << _index;
            return ERROR( false, SYS_INTERNAL_NULL_INPUT_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // extract and test rescInfo pointer
        rescInfo_t* resc_info = StructFileDesc[ _index ].rescInfo;
        if( !resc_info ) {
            std::stringstream msg;
            msg << "make_tar_cache_dir - null rescInfo pointer for index: ";
            msg << _index;
            return ERROR( false, SYS_INTERNAL_NULL_INPUT_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // construct a mkdir structure
        fileMkdirInp_t fileMkdirInp;
        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        fileMkdirInp.fileType = UNIX_FILE_TYPE;	  // the only type for cache
        fileMkdirInp.mode     = DEFAULT_DIR_MODE;
        rstrcpy( fileMkdirInp.addr.hostAddr,  resc_info->rescLoc, NAME_LEN );

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
                    return ERROR( false, status, "make_tar_cache_dir - failed to create cache directory" );

                } // else

            } // else

        } // while
        
        // =-=-=-=-=-=-=-
        // copy successful cache dir out of mkdir struct
        rstrcpy( spec_coll->cacheDir, fileMkdirInp.dirName, MAX_NAME_LEN );

        // =-=-=-=-=-=-=-
        // Win!
        return SUCCESS();

    } // make_tar_cache_dir
        
    // =-=-=-=-=-=-=-
    // extract the tar file into a cache dir
    eirods::error stage_tar_struct_file( int _index ) {
        int status = -1;

        // =-=-=-=-=-=-=-
        // extract the special collection from the struct file table
        specColl_t* spec_coll = StructFileDesc[_index].specColl;
        if( spec_coll == NULL ) {
            return ERROR( false, SYS_INTERNAL_NULL_INPUT_ERR, "" );            
        }

        // =-=-=-=-=-=-=-
        // check to see if we have a cache dir and make one if not
        if( strlen( spec_coll->cacheDir ) == 0 ) {
            // =-=-=-=-=-=-=-
            // no cache. stage one. make the CacheDir first
            eirods::error mk_err = make_tar_cache_dir( _index );
            if( !mk_err.ok() ) {
                return PASS( false, mk_err.code(), "stage_tar_struct_file - failed to create cachedir", mk_err );
            }

            // =-=-=-=-=-=-=-
            // expand tar file into cache dir
            eirods::error extract_err = extract_tar_file( _index );
            if( !extract_err.ok() ) {
                std::stringstream msg;
                msg << "stage_tar_struct_file - extract_tar_file failed for [";
                msg << spec_coll->objPath;
                msg << "] in cache directory [";
                msg << spec_coll->cacheDir;

                /* XXXX may need to remove the cacheDir too */
                return PASS( false, SYS_TAR_STRUCT_FILE_EXTRACT_ERR - errno, msg.str(), extract_err ); 

            } // if !ok

            // =-=-=-=-=-=-=-
            // register the CacheDir 
            status = modCollInfo2( StructFileDesc[ _index ].rsComm, spec_coll, 0 );
            if( status < 0 ) {
               return ERROR( false, status, "stage_tar_struct_file - modCollInfo2 failed." ); 
            }

        } // if empty cachedir  

        return SUCCESS();

    } // stage_tar_struct_file
  
    // =-=-=-=-=-=-=-
    //  
    int match_struct_file_desc( specColl_t* _spec_coll ) {

        for( int i = 1; i < NUM_STRUCT_FILE_DESC; i++) {
            if( StructFileDesc[i].inuseFlag == FD_INUSE &&
                StructFileDesc[i].specColl  != NULL     &&
                strcmp( StructFileDesc[i].specColl->collection, _spec_coll->collection ) == 0 &&
                strcmp( StructFileDesc[i].specColl->objPath,    _spec_coll->objPath    ) == 0 ) {
                return (i);
            };
        } // for

        return (SYS_OUT_OF_FILE_DESC);

    } // match_struct_file_desc

    // =-=-=-=-=-=-=-
    // local function to manage the open of a tar file 
    eirods::error tar_struct_file_open( rsComm_t*   _comm, 
                                        specColl_t* _spec_coll, 
                                        int&        _struct_desc_index ) {
        int struct_desc_index       = 0;
        int status                  = 0;
        specCollCache_t* spec_cache = 0;

        // =-=-=-=-=-=-=-
        // trap null parameters
        if( 0 == _spec_coll ) {
            std::string msg( "tar_struct_file_open - null special collection parameter" );
            return ERROR( false, SYS_INTERNAL_NULL_INPUT_ERR, msg );
        }

        if( 0 == _comm ) {
            std::string msg( "tar_struct_file_open - null rsComm_t parameter" );
            return ERROR( false, SYS_INTERNAL_NULL_INPUT_ERR, msg );
        }

        // =-=-=-=-=-=-=-
        // look for opened StructFileDesc
        struct_desc_index = match_struct_file_desc( _spec_coll );
        if( struct_desc_index > 0 ) {
            _struct_desc_index = struct_desc_index;
            return SUCCESS();
        }
     
        // =-=-=-=-=-=-=-
        // alloc and trap bad alloc
        if( ( struct_desc_index = allocStructFileDesc() ) < 0 ) {
            return ERROR( false, struct_desc_index, "tar_struct_file_open - call to allocStructFileDesc failed." );
        }

        // =-=-=-=-=-=-=-
        // [ mwan? :: Have to do this because  _spec_coll could come from a remote host ]
        // NOTE :: i dont see any remote server to server comms here
        if( ( status = getSpecCollCache( _comm,  _spec_coll->collection, 0, &spec_cache ) ) >= 0 ) {
            // =-=-=-=-=-=-=-
            // copy pointer to cached special collection
            StructFileDesc[ struct_desc_index ].specColl = &spec_cache->specColl;

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
            StructFileDesc[ struct_desc_index ].specColl =  _spec_coll;
        }

        // =-=-=-=-=-=-=-
        // cache pointer to comm struct
        StructFileDesc[ struct_desc_index ].rsComm = _comm;

        // =-=-=-=-=-=-=-
        // resolve resource - fill in rescInfo data structure...
        status = resolveResc( StructFileDesc[ struct_desc_index ].specColl->resource, &StructFileDesc[ struct_desc_index ].rescInfo );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "tar_struct_file_open - error returned from resolveResc for: [";
            msg << _spec_coll->resource;
            msg << "], status: ";
            msg << status;
            freeStructFileDesc( struct_desc_index );
            return ERROR( false, struct_desc_index, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // TODO :: need to deal with remote open here 

        // =-=-=-=-=-=-=-
        // tage the tar file so we can get at its tasty innards
        eirods::error stage_err = stage_tar_struct_file( struct_desc_index );
        if( !stage_err.ok() ) {
            freeStructFileDesc( struct_desc_index );
            return PASS( false, struct_desc_index, "tar_struct_file_open - stage_tar_struct_file failed.", stage_err );
        }

        // =-=-=-=-=-=-=-
        // Win!
        return CODE( struct_desc_index );

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
            return ERROR( false, SYS_STRUCT_FILE_PATH_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=- 
        // compose the path
        snprintf( _phy_path, MAX_NAME_LEN, "%s%s", _spec_coll->cacheDir, _sub_file_path + len );
        
        // =-=-=-=-=-=-=- 
        // Win!
        return SUCCESS();

    } // compose_cache_dir_physical_path
    
	// =-=-=-=-=-=-=-
	// 
    int alloc_tar_sub_file_desc() {
        for( int i = 1; i < NUM_TAR_SUB_FILE_DESC; i++ ) {
            if (TarSubFileDesc[i].inuseFlag == FD_FREE) {
                TarSubFileDesc[i].inuseFlag = FD_INUSE;
                return (i);
            };
        }

        rodsLog (LOG_NOTICE,
         "alloc_tar_sub_file_desc: out of TarSubFileDesc");

        return (SYS_OUT_OF_FILE_DESC);
    }

	// =-=-=-=-=-=-=-
	// interface for POSIX create
    eirods::error tarFileCreatePlugin( eirods::resource_property_map* 
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
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileCreatePlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileCreatePlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileCreatePlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

		// =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( false, sub_index, "tarFileCreatePlugin - alloc_tar_sub_file_desc failed." );
        }

		// =-=-=-=-=-=-=-
        // cache struct file index into sub file index
        TarSubFileDesc[ sub_index ].structFileInx = struct_file_index;
        
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
        rstrcpy( fileCreateInp.addr.hostAddr,
                 StructFileDesc[ struct_file_index ].rescInfo->rescLoc, 
                 NAME_LEN );

		// =-=-=-=-=-=-=-
        // make the call to create a file
        int status = rsFileCreate( comm, &fileCreateInp );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "tarFileCreatePlugin - rsFileCreate failed for [";
            msg << fileCreateInp.fileName;
            msg << "], status = ";
            msg << status;
            return ERROR( false, status, msg.str() );
        } else {
            TarSubFileDesc[ sub_index ].fd = status;
            StructFileDesc[ struct_file_index ].openCnt++;
            _object->file_descriptor( sub_index );
            return CODE(  sub_index  );
        }

	} // tarFileCreatePlugin

	// =-=-=-=-=-=-=-
	// interface for POSIX Open
	eirods::error tarFileOpenPlugin( eirods::resource_property_map* 
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
        eirods::structured_object* struct_obj = dynamic_cast< eirods::structured_object* >( _object );
        if( !struct_obj ) {
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileOpenPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileOpenPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileOpenPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

		// =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( false, sub_index, "tarFileOpenPlugin - alloc_tar_sub_file_desc failed." );
        }

		// =-=-=-=-=-=-=-
        // cache struct file index into sub file index
        TarSubFileDesc[ sub_index ].structFileInx = struct_file_index;
        
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
        rstrcpy( fileOpenInp.addr.hostAddr,
                 StructFileDesc[ struct_file_index ].rescInfo->rescLoc, 
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
            return ERROR( false, status, msg.str() );
        } else {
            TarSubFileDesc[ sub_index ].fd = status;
            StructFileDesc[ struct_file_index ].openCnt++;
            _object->file_descriptor( sub_index );
            return CODE(  sub_index  );
        }

	} // tarFileOpenPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Read
	eirods::error tarFileReadPlugin( eirods::resource_property_map* 
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
            TarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a read structure and make the rs call
        fileReadInp_t fileReadInp;
        bytesBuf_t fileReadOutBBuf;
        memset (&fileReadInp, 0, sizeof (fileReadInp));
        memset (&fileReadOutBBuf, 0, sizeof (fileReadOutBBuf));
        fileReadInp.fileInx = TarSubFileDesc[ _object->file_descriptor() ].fd;
        fileReadInp.len     = _len;
        fileReadOutBBuf.buf = _buf;
        int status = rsFileRead( _object->comm(), &fileReadInp, &fileReadOutBBuf );

        return CODE(status);

	} // tarFileReadPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Write
	eirods::error tarFileWritePlugin( eirods::resource_property_map* 
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
            TarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a write structure and make the rs call
        fileWriteInp_t fileWriteInp;
        bytesBuf_t     fileWriteOutBBuf;
        memset( &fileWriteInp,     0, sizeof (fileWriteInp) );
        memset( &fileWriteOutBBuf, 0, sizeof (fileWriteOutBBuf) );
        fileWriteInp.len     = fileWriteOutBBuf.len = _len;
        fileWriteInp.fileInx = TarSubFileDesc[ _object->file_descriptor() ].fd;
        fileWriteOutBBuf.buf = _buf;

        // =-=-=-=-=-=-=-
        // make the write api call
        int status = rsFileWrite( _object->comm(), &fileWriteInp, &fileWriteOutBBuf );
        if( status > 0 ) {
            // =-=-=-=-=-=-=-
            // cache has been written 
            int         struct_idx = TarSubFileDesc[ _object->file_descriptor() ].structFileInx;
            specColl_t* spec_coll   = StructFileDesc[ struct_idx ].specColl;
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
	eirods::error tarFileClosePlugin( eirods::resource_property_map* 
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
            TarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // build a close structure and make the rs call
        fileCloseInp_t fileCloseInp;
        fileCloseInp.fileInx = TarSubFileDesc[ _object->file_descriptor() ].fd;
        int status = rsFileClose( _object->comm(), &fileCloseInp );
        if( status < 0 ) {

        }

        // =-=-=-=-=-=-=-
        // close out the sub file allocation and free the space
        int struct_file_index = TarSubFileDesc[ _object->file_descriptor() ].structFileInx;
        StructFileDesc[ struct_file_index ].openCnt++;
        freeTarSubFileDesc ( _object->file_descriptor() );
        _object->file_descriptor( 0 );

        return CODE( status );

	} // tarFileClosePlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Unlink
	eirods::error tarFileUnlinkPlugin( eirods::resource_property_map* 
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
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileUnlinkPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileUnlinkPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileUnlinkPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

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
        rstrcpy( fileUnlinkInp.addr.hostAddr, StructFileDesc[ struct_file_index ].rescInfo->rescLoc, NAME_LEN );

		// =-=-=-=-=-=-=-
        // make the call to unlink a file
        int status = rsFileUnlink( comm, &fileUnlinkInp );
        if( status >= 0 ) {
            specColl_t* specColl;
            /* cache has been written */
            specColl_t* loc_spec_coll = StructFileDesc[ struct_file_index ].specColl;
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
	eirods::error tarFileStatPlugin( eirods::resource_property_map* 
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
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileStatPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileStatPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileStatPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

    
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
        rstrcpy (fileStatInp.addr.hostAddr,  
        StructFileDesc[ struct_file_index ].rescInfo->rescLoc, NAME_LEN);

		// =-=-=-=-=-=-=-
        // make the call to stat a file
        rodsStat_t* rods_stat;
        int status = rsFileStat( comm, &fileStatInp, &rods_stat );
        if( status >= 0 ) {
            rodsStatToStat( _statbuf, rods_stat );
        }

        return CODE( status );

	} // tarFileStatPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Fstat
	eirods::error tarFileFstatPlugin( eirods::resource_property_map* 
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
            TarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileFstatPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // build a fstat structure and make the rs call
        fileFstatInp_t fileFstatInp;
        memset( &fileFstatInp, 0, sizeof( fileFstatInp ) );
        fileFstatInp.fileInx = TarSubFileDesc[ _object->file_descriptor() ].fd;
        
        rodsStat_t* rods_stat;
        int status = rsFileFstat( comm, &fileFstatInp, &rods_stat );
        if( status >= 0 ) {
            rodsStatToStat( _statbuf, rods_stat );
        }

        return CODE( status );

	} // tarFileFstatPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX lseek
	eirods::error tarFileLseekPlugin( eirods::resource_property_map* 
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
            TarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileLseekPlugin - null comm pointer in structure_object" );
        }

        // =-=-=-=-=-=-=-
        // build a lseek structure and make the rs call
        fileLseekInp_t fileLseekInp;
        memset( &fileLseekInp, 0, sizeof( fileLseekInp ) );
        fileLseekInp.fileInx = TarSubFileDesc[ _object->file_descriptor() ].fd;
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
	eirods::error tarFileFsyncPlugin( eirods::resource_property_map* 
	                                                       _prop_map, 
									   eirods::resource_child_map* 
									                       _cmap,
	                                   eirods::first_class_object*
														   _object ) {
		// =-=-=-=-=-=-=-
        // Not Implemented for this plugin
        return ERROR( false, -1, "tarFileFsyncPlugin is not implemented." );

	} // tarFileFsyncPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX mkdir
	eirods::error tarFileMkdirPlugin( eirods::resource_property_map*
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
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileMkdirPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileMkdirPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileMkdirPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

		// =-=-=-=-=-=-=-
        // build a file mkdir structure to pass off to the server api call
        fileMkdirInp_t fileMkdirInp;
        fileMkdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        rstrcpy( fileMkdirInp.addr.hostAddr, StructFileDesc[ struct_file_index ].rescInfo->rescLoc, NAME_LEN );
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
            // use the specColl in StructFileDesc 
            specColl_t* loc_spec_coll = StructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written 
            if( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if (status1 < 0) {
                    return ERROR( false, status1, "tarFileMkdirPlugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

	} // tarFileMkdirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX mkdir
	eirods::error tarFileChmodPlugin( eirods::resource_property_map* 
	                                                       _prop_map, 
									   eirods::resource_child_map* 
									                       _cmap,
                                       eirods::first_class_object*
									                       _object,
									   int                 _mode ) {
		// =-=-=-=-=-=-=-
        // Not Implemented for this plugin
        return ERROR( false, -1, "tarFileChmodPlugin is not implemented." );

	} // tarFileChmodPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX mkdir
	eirods::error tarFileRmdirPlugin( eirods::resource_property_map* 
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
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileRmdirPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileRmdirPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileRmdirPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

		// =-=-=-=-=-=-=-
        // build a file mkdir structure to pass off to the server api call
        fileRmdirInp_t fileRmdirInp;
        fileRmdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        rstrcpy( fileRmdirInp.addr.hostAddr, StructFileDesc[ struct_file_index ].rescInfo->rescLoc, NAME_LEN );

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
            // use the specColl in StructFileDesc 
            specColl_t* loc_spec_coll = StructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written 
            if( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if (status1 < 0) {
                    return ERROR( false, status1, "tarFileRmdirPlugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

	} // tarFileRmdirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX opendir
	eirods::error tarFileOpendirPlugin( eirods::resource_property_map* 
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
            return ERROR( false, -1, "tarFileOpendirPlugin - failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileOpendirPlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileOpendirPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileOpendirPlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

		// =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( false, sub_index, "tarFileOpenPlugin - alloc_tar_sub_file_desc failed." );
        }

		// =-=-=-=-=-=-=-
        // build a file open structure to pass off to the server api call
        fileOpendirInp_t fileOpendirInp;
        memset( &fileOpendirInp, 0, sizeof( fileOpendirInp ) );
        fileOpendirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        rstrcpy( fileOpendirInp.addr.hostAddr, StructFileDesc[ struct_file_index ].rescInfo->rescLoc, NAME_LEN );
        

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
            return ERROR( false, status, msg.str() );
        } else {
            TarSubFileDesc[ sub_index ].fd = status;
            StructFileDesc[ struct_file_index ].openCnt++;
            _object->file_descriptor( sub_index );
            return CODE( sub_index );
        }

	} // tarFileOpendirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX closedir
	eirods::error tarFileClosedirPlugin( eirods::resource_property_map* 
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
            TarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileClosedirPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // build a file close dir structure to pass off to the server api call
        fileClosedirInp_t fileClosedirInp;
        fileClosedirInp.fileInx = TarSubFileDesc[ _object->file_descriptor() ].fd;
        int status = rsFileClosedir( comm, &fileClosedirInp );
        if( status < 0 ) {
            return ERROR( false, -1, "tarFileClosedirPlugin - failed on call to rsFileClosedir" );
        }

		// =-=-=-=-=-=-=-
        // close out the sub file index and free the allocation
        int struct_file_index = TarSubFileDesc[ _object->file_descriptor() ].structFileInx;
        StructFileDesc[ struct_file_index ].openCnt++;
        freeTarSubFileDesc( _object->file_descriptor() );
        _object->file_descriptor( 0 );

        return CODE( status );

	} // tarFileClosedirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX readdir
	eirods::error tarFileReaddirPlugin( eirods::resource_property_map* 
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
            TarSubFileDesc[ _object->file_descriptor() ].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "tarFileReadPlugin - sub file index ";
            msg << _object->file_descriptor();
            msg << " is out of range.";
            return ERROR( false, SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = _object->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileReaddirPlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // build a file read dir structure to pass off to the server api call
        fileReaddirInp_t fileReaddirInp; 
        fileReaddirInp.fileInx = TarSubFileDesc[ _object->file_descriptor() ].fd;

        // =-=-=-=-=-=-=-
        // make the api call to read the directory
        int status = rsFileReaddir( comm, &fileReaddirInp, _dirent_ptr );
        if( status < 0 ) {
            return ERROR( false, status, "tarFileReaddirPlugin - failed in call to rsFileReaddir" );
        }
        
        return CODE( status );

    } // tarFileReaddirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX readdir
	eirods::error tarFileStagePlugin( eirods::resource_property_map* 
	                                                       _prop_map, 
									   eirods::resource_child_map* 
									                       _cmap,
                                       eirods::first_class_object*
									                       _object ) {
		// =-=-=-=-=-=-=-
        // this interface is not implemented in this plugin
        return ERROR( false, -1, "tarFileStagePlugin - not implemented." );

	} // tarFileStagePlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX rename
	eirods::error tarFileRenamePlugin( eirods::resource_property_map* 
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
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileRenamePlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileRenamePlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileRenamePlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;
        
		// =-=-=-=-=-=-=-
        // build a file rename structure to pass off to the server api call
        fileRenameInp_t fileRenameInp;
        memset (&fileRenameInp, 0, sizeof (fileRenameInp));
        fileRenameInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        rstrcpy( fileRenameInp.addr.hostAddr, StructFileDesc[ struct_file_index ].rescInfo->rescLoc, NAME_LEN );
         
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
            // use the specColl in StructFileDesc 
            specColl_t* loc_spec_coll = StructFileDesc[ struct_file_index ].specColl;
            // =-=-=-=-=-=-=-
            // cache has been written 
            if( loc_spec_coll->cacheDirty == 0 ) {
                loc_spec_coll->cacheDirty = 1;
                int status1 = modCollInfo2( comm, loc_spec_coll, 0 );
                if (status1 < 0) {
                    return ERROR( false, status1, "tarFileRenamePlugin - Failed to call modCollInfo2" );
                }
            }

        } // if status

        return CODE( status );

	} // tarFileRenamePlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX truncate
	eirods::error tarFileTruncatePlugin( eirods::resource_property_map* 
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
            return ERROR( false, -1, "failed to cast first_class_object to structured_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the special collection pointer
        specColl_t* spec_coll = struct_obj->spec_coll();
        if( !spec_coll ) {
            return ERROR( false, -1, "tarFileTruncatePlugin - null spec_coll pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // extract and check the comm pointer
        rsComm_t* comm = struct_obj->comm();
        if( !comm ) {
            return ERROR( false, -1, "tarFileTruncatePlugin - null comm pointer in structure_object" );
        }

		// =-=-=-=-=-=-=-
        // open and stage the tar file, get its index
        int struct_file_index = 0;
        eirods::error open_err =  tar_struct_file_open( comm, spec_coll, struct_file_index );
        if( !open_err.ok() ) {
            std::stringstream msg;
            msg << "tarFileTruncatePlugin - tar_struct_file_open error for [";
            msg << spec_coll->objPath; 
            return PASS( false, -1, msg.str(), open_err );
        }

		// =-=-=-=-=-=-=-
        // use the cached specColl. specColl may have changed 
        spec_coll = StructFileDesc[ struct_file_index ].specColl;

		// =-=-=-=-=-=-=-
        // allocate yet another index into another table
        int sub_index = alloc_tar_sub_file_desc();
        if( sub_index < 0 ) {
            return ERROR( false, sub_index, "tarFileTruncatePlugin - alloc_tar_sub_file_desc failed." );
        }

		// =-=-=-=-=-=-=-
        // cache struct file index into sub file index
        TarSubFileDesc[ sub_index ].structFileInx = struct_file_index;
        
		// =-=-=-=-=-=-=-
        // build a file create structure to pass off to the server api call
        fileOpenInp_t fileTruncateInp;
        memset( &fileTruncateInp, 0, sizeof( fileTruncateInp ) );
        rstrcpy( fileTruncateInp.addr.hostAddr,  StructFileDesc[ struct_file_index ].rescInfo->rescLoc, NAME_LEN );
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
            int         struct_idx = TarSubFileDesc[ _object->file_descriptor() ].structFileInx;
            specColl_t* spec_coll   = StructFileDesc[ struct_idx ].specColl;
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
	// interface to determine free space on a device given a path
	eirods::error tarFileGetFsFreeSpacePlugin( eirods::resource_property_map* 
	                                                                _prop_map, 
												eirods::resource_child_map* 
												                    _cmap,
                                                eirods::first_class_object*
									                                _object ) { 

		// =-=-=-=-=-=-=-
        // this interface is not implemented in this plugin
        return ERROR( false, -1, "tarFileGetFsFreeSpacePlugin - is not implemented." );

	} // tarFileGetFsFreeSpacePlugin

    // =-=-=-=-=-=-=-
	// tarFileCopyPlugin
	eirods::error tarFileCopyPlugin( eirods::resource_property_map*
	                                                          _prop_map, 
										  eirods::resource_child_map* 
										                      _cmap,
										  eirods::first_class_object*
										                      _object,
	                                      const char *destFileName ) {
		// =-=-=-=-=-=-=-
        // this interface is not implemented in this plugin
        return ERROR( false, -1, "tarFileCopyPlugin - is not implemented." );

	} // tarFileCopyPlugin


    // =-=-=-=-=-=-=-
	// tarStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
	// Just copy the file from filename to cacheFilename. optionalInfo info
	// is not used.
	eirods::error tarStageToCachePlugin( eirods::resource_property_map* 
	                                                          _prop_map, 
										  eirods::resource_child_map* 
										                      _cmap,
										  eirods::first_class_object*
										                      _object,
										  const char*         _cache_file_name ) { 
		// =-=-=-=-=-=-=-
        // this interface is not implemented in this plugin
        return ERROR( false, -1, "tarStageToCachePlugin - is not implemented." );
										  
	} // tarStageToCachePlugin

    // =-=-=-=-=-=-=-
	// tarSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
	// Just copy the file from cacheFilename to filename. optionalInfo info
	// is not used.
	eirods::error tarSyncToArchPlugin( eirods::resource_property_map* 
	                                                        _prop_map, 
										eirods::resource_child_map* 
										                    _cmap,
										eirods::first_class_object*
										                    _object,
										const char*         _cache_file_name ) { 
		// =-=-=-=-=-=-=-
        // this interface is not implemented in this plugin
        return ERROR( false, -1, "tarSyncToArchPlugin - is not implemented." );

	} // tarSyncToArchPlugin

    // =-=-=-=-=-=-=-
	// 3. create derived class to handle tar file system resources
	//    necessary to do custom parsing of the context string to place
	//    any useful values into the property map for reference in later
	//    operations.  semicolon is the preferred delimiter
	class tarfilesystem_resource : public eirods::resource {
    public:
        tarfilesystem_resource( std::string _context ) : eirods::resource( _context ) {
		} // ctor

	}; // class tarfilesystem_resource
  
    // =-=-=-=-=-=-=-
	// 4. create the plugin factory function which will return a dynamically
	//    instantiated object of the previously defined derived resource.  use
	//    the add_operation member to associate a 'call name' to the interfaces
	//    defined above.  for resource plugins these call names are standardized
	//    as used by the e-irods facing interface defined in 
	//    server/drivers/src/fileDriver.c
	eirods::resource* plugin_factory( std::string _context  ) {

        // =-=-=-=-=-=-=-
		// 4a. create tarfilesystem_resource
		tarfilesystem_resource* resc = new tarfilesystem_resource( _context );

        // =-=-=-=-=-=-=-
		// 4b. map function names to operations.  this map will be used to load
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
        resc->add_operation( "freespace",    "tarFileGetFsFreeSpacePlugin" );
        resc->add_operation( "rmdir",        "tarFileRmdirPlugin" );
        resc->add_operation( "closedir",     "tarFileClosedirPlugin" );
        resc->add_operation( "truncate",     "tarFileTruncatePlugin" );
        resc->add_operation( "stagetocache", "tarStageToCachePlugin" );
        resc->add_operation( "synctoarch",   "tarSyncToArchPlugin" );

        // =-=-=-=-=-=-=-
		// 4c. return the pointer through the generic interface of an
		//     eirods::resource pointer
	    return dynamic_cast<eirods::resource*>( resc );
        
	} // plugin_factory

}; // extern "C" 



