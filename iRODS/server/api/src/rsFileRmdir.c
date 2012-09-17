/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileRmdir.h for a description of this API call.*/

#include "fileRmdir.h"
#include "fileOpendir.h"
#include "fileReaddir.h"
#include "fileClosedir.h"
#include "miscServerFunct.h"
#include "tarSubStructFileDriver.h"

// =-=-=-=-=-=-=-
// eirods include
#include "eirods_log.h"
#include "eirods_collection_object.h"


int
rsFileRmdir (rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileRmdirInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileRmdir (rsComm, fileRmdirInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileRmdir (rsComm, fileRmdirInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileRmdir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileRmdir (rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileRmdir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileRmdir (rodsServerHost->conn, fileRmdirInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileOpen: rcFileRmdir failed for %s",
          fileRmdirInp->dirName);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function which handles removing directories via the resource plugin
int _rsFileRmdir (rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp) {
    
	int status = 0;
    eirods::collection_object coll_obj( fileRmdirInp->dirName, 0, 0 );

    if ((fileRmdirInp->flags & RMDIR_RECUR) != 0) {
		// =-=-=-=-=-=-=-
		// FIXME :: revisit this after moving to First class Objects
		// recursive. This is a very dangerous operation. curently
		// it is only used to remove cache directory of structured 
		// files 

        #if defined(solaris_platform)
		char fileDirent[sizeof (struct dirent) + MAX_NAME_LEN];
		struct dirent *myFileDirent = (struct dirent *) fileDirent;
        #else
		struct dirent fileDirent;
		struct dirent *myFileDirent = &fileDirent;
        #endif

		// =-=-=-=-=-=-=-
		// if it is not a cache dir, return an error as we only do this
		// for cache dirs at the moment.
		if( strstr( fileRmdirInp->dirName, CACHE_DIR_STR) == NULL ) {
			rodsLog( LOG_ERROR,"_rsFileRmdir: recursive rm of non cachedir path %s",
			         fileRmdirInp->dirName );
			return (SYS_INVALID_FILE_PATH);
		}

		// =-=-=-=-=-=-=-
		// call opendir via resource plugin
		eirods::error opendir_err = fileOpendir( coll_obj );

		// =-=-=-=-=-=-=-
		// log an error, if any
		if( !opendir_err.ok() ) {
			std::stringstream msg;
			msg << "_rsFileRmdir: fileOpendir for ";
			msg << fileRmdirInp->dirName; 
			msg << ", status = ";
			msg << status;
			eirods::error err = PASS( false, status, msg.str(), opendir_err );
			eirods::log ( err );
			return (status);
		}

        // =-=-=-=-=-=-=-
        // read the directory via resource plugin and either handle files or recurse into another directory
		eirods::error readdir_err = fileReaddir( coll_obj, myFileDirent );
		while( readdir_err.ok() && 0 == readdir_err.code() ) {
			struct stat statbuf;
			char myPath[MAX_NAME_LEN];

            // =-=-=-=-=-=-=-
			// handle relative paths
			if (strcmp (myFileDirent->d_name, ".") == 0 || strcmp (myFileDirent->d_name, "..") == 0) {
		        readdir_err = fileReaddir( coll_obj, myFileDirent );
				continue;
			}

            // =-=-=-=-=-=-=-
			// cache path name
			snprintf( myPath, MAX_NAME_LEN, "%s/%s", fileRmdirInp->dirName, myFileDirent->d_name );

            // =-=-=-=-=-=-=-
			// call stat via resource plugin, handle errors
            eirods::collection_object myPath_obj( myPath, 0, 0 );
			eirods::error stat_err = fileStat( myPath_obj, &statbuf );
			if( !stat_err.ok() ) {
				std::stringstream msg;
				msg << "_rsFileRmdir: fileStat for ";
				msg << myPath;
				msg << ", status = ";
				msg << stat_err.code();
				eirods::error log_err = PASS( false, stat_err.code(), msg.str(), stat_err );
				eirods::log( log_err ); 

                // =-=-=-=-=-=-=-
				// call closedir via resource plugin, handle errors
				eirods::error closedir_err = fileClosedir( myPath_obj );
				if( !closedir_err.ok() ) {
					std::stringstream msg;
					msg << "_rsFileRmdir: fileClosedir for ";
					msg << myPath;
					msg << ", status = ";
					msg << status;
					eirods::error log_err = PASS( false, stat_err.code(), msg.str(), closedir_err );
					eirods::log( log_err ); 
				}

				return (status);

			}  // if !stat_err.ok()

            // =-=-=-=-=-=-=-
			// filter based on stat results: file, dir, or error
			if ((statbuf.st_mode & S_IFREG) != 0) {
				// =-=-=-=-=-=-=-
				// handle case where file is found
				eirods::error unlink_err = fileUnlink( myPath_obj );
				if( !unlink_err.ok() ) {
					eirods::error log_err = PASS( false, unlink_err.code(), 
					                        "_rsFileRmDir - error during unlink.", 
											unlink_err );
					eirods::log( log_err );
				}
				
			} else if ((statbuf.st_mode & S_IFDIR) != 0) {
				// =-=-=-=-=-=-=-
				// handle case where directory is found
				fileRmdirInp_t myRmdirInp;
				memset( &myRmdirInp, 0, sizeof( myRmdirInp ) );

				myRmdirInp.fileType = fileRmdirInp->fileType;
				myRmdirInp.flags    = fileRmdirInp->flags;
				rstrcpy( myRmdirInp.dirName, myPath, MAX_NAME_LEN );
            
			    // =-=-=-=-=-=-=-
			    // RECURSE - call _rsFileRmdir on this new found directory
				status = _rsFileRmdir (rsComm, &myRmdirInp);

			} else {
				rodsLog( LOG_NOTICE, "_rsFileRmdir:  for %s, status = %d", myPath, status);
			}

            // =-=-=-=-=-=-=-
			// handle error condition on the above three cases
			if( status < 0 ) {
				rodsLog( LOG_NOTICE,"_rsFileRmdir:  rm of %s failed, status = %d", myPath, status );

                // =-=-=-=-=-=-=-
				// call closedir via resource plugin, handle errors
				eirods::error closedir_err = fileClosedir( myPath_obj );
				if( !closedir_err.ok() ) {
					std::stringstream msg;
					msg << "_rsFileRmdir: fileClosedir for ";
					msg << myPath;
					msg << ", status = ";
					msg << closedir_err.code();
					eirods::error log_err = PASS( false, closedir_err.code(), msg.str(), closedir_err );
					eirods::log( log_err ); 
				}
				
				return closedir_err.code();

			} // if status < 0
	
	        // =-=-=-=-=-=-=-
			// continue readdir via resource plugin	
		    readdir_err = fileReaddir( coll_obj, myFileDirent );
		
		} // while

        // =-=-=-=-=-=-=-
		// call closedir via resource plugin, handle errors
		eirods::error closedir_err = fileClosedir( coll_obj );
		if( !closedir_err.ok() ) {
			std::stringstream msg;
			msg << "_rsFileRmdir: fileClosedir for ";
			msg << fileRmdirInp->dirName;
			msg << ", status = ";
			msg << closedir_err.code();
			eirods::error log_err = PASS( false, closedir_err.code(), msg.str(), closedir_err );
			eirods::log( log_err ); 
		}

		if( status < 0 && status != -1 ) {	/* end of file */
		    return (status);
		}

    } // if RMDIR_RECUR ( recursive delete )

    // =-=-=-=-=-=-=-
	// make the call to rmdir via the resource plugin
	eirods::error rmdir_err = fileRmdir( coll_obj );
	if( !rmdir_err.ok() ) {
		std::stringstream msg;
		msg << "_rsFileRmdir: fileRmdir for ";
		msg << fileRmdirInp->dirName;
		msg << ", status = ";
		msg << rmdir_err.code();
		eirods::error err = PASS( false, rmdir_err.code(), msg.str(), rmdir_err );
		eirods::log ( err );
	}

    return rmdir_err.code();

} // _rsFileRmdir






 















