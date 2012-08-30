// =-=-=-=-=-=-=-
// E-iRODS Includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "eirods_resource_plugin.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>
#include <sstream>

// =-=-=-=-=-=-=-
// System Includes
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






extern "C" {

#define NB_READ_TOUT_SEC	60	/* 60 sec timeout */
#define NB_WRITE_TOUT_SEC	60	/* 60 sec timeout */

    // =-=-=-=-=-=-=-
	// Direct Rip from iRODS/server/drivers/src/unixFileDriver.c 
    int EIRODS_PLUGIN_VERSION=1.0;

    // =-=-=-=-=-=-=-
	// interface for POSIX create
	eirods::error unixFileCreatePlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap, 
	                                    const char* _file_name, int _mode, size_t _mySize, int* _fd ) {
        // =-=-=-=-=-=-=-
		// make call to umask & open for create
		mode_t myMask = umask((mode_t) 0000);
		int    fd     = open( _file_name, O_RDWR|O_CREAT|O_EXCL, _mode );

        // =-=-=-=-=-=-=-
		// reset the old mask 
		(void) umask((mode_t) myMask);
		
        // =-=-=-=-=-=-=-
		// if we got a 0 descriptor, try again
		if( fd == 0 ) {
			close (fd);
		    rodsLog( LOG_NOTICE, "unixFileCreatePlugin: 0 descriptor" );
			open ("/dev/null", O_RDWR, 0);
			fd = open(  _file_name, O_RDWR|O_CREAT|O_EXCL, _mode );
		}

        // =-=-=-=-=-=-=-
		// cache file descriptor in out-variable
        (*_fd) = fd;
		
        // =-=-=-=-=-=-=-
		// 
		if( fd < 0 ) {
			fd = UNIX_FILE_CREATE_ERR - errno;
			
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_fd) = fd;

			std::stringstream msg;
			msg << "unixFileCreatePlugin: open error for ";
			msg << _file_name;
			msg << ", status = ";
			msg << fd;
 
			return ERROR( false, errno, msg.str() );
		}

        // =-=-=-=-=-=-=-
		// declare victory!
		return SUCCESS();

	} // unixFileCreatePlugin

	// =-=-=-=-=-=-=-
	// interface for POSIX Open
	eirods::error unixFileOpenPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap, 
	                                  const char * _file_name, int _flags, int _mode, int* _fd ) {
        // =-=-=-=-=-=-=-
		// handle OSX weirdness...
        #if defined(osx_platform)
		/* For osx, O_TRUNC = 0x0400, O_TRUNC = 0x200 for other system */
		if( _flags & 0x200) {
			_flags = _flags ^ 0x200;
			_flags = _flags | O_TRUNC;
		}
        #endif

        // =-=-=-=-=-=-=-
		// make call to open
		errno = 0;
		int fd = open( _file_name, _flags, _mode );

        // =-=-=-=-=-=-=-
		// if we got a 0 descriptor, try again
		if( fd == 0 ) {
			close (fd);
			rodsLog( LOG_NOTICE, "unixFileOpenPlugin: 0 descriptor" );
			open ("/dev/null", O_RDWR, 0);
			fd = open( _file_name, _flags, _mode );
		}

        // =-=-=-=-=-=-=-
		// cache file descriptor in out-variable
        (*_fd) = fd;

        // =-=-=-=-=-=-=-
		// did we still get an error?
		if (fd < 0) {
			fd = UNIX_FILE_OPEN_ERR - errno;
			
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_fd) = fd;

            std::stringstream msg;
			msg << "unixFileOpenPlugin: open error for ";
			msg << _file_name;
			msg << ", status = ";
			msg << fd;
 
			return ERROR( false, errno, msg.str() );
		}
		
        // =-=-=-=-=-=-=-
		// declare victory!
		return SUCCESS();

	} // unixFileOpenPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Read
	eirods::error unixFileReadPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                  int _fd, void* _buf, int _len, int* _status )	{
		// =-=-=-=-=-=-=-
		// make the call to read
		int status = read( _fd, _buf, _len );

		// =-=-=-=-=-=-=-
		// cache status value into out variable
		(*_status) = status;

		// =-=-=-=-=-=-=-
		// pass along an error if it was not successful
		if( status < 0 ) {
			status = UNIX_FILE_READ_ERR - errno;
			
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileReadPlugin - read error fd = ";
			msg << _fd;
			msg << ", status = ";
			msg << _status;
			return ERROR( false, errno, msg.str() );
		}
		
		// =-=-=-=-=-=-=-
		// win!
		return SUCCESS();

	} // unixFileReadPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Write
	eirods::error unixFileWritePlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   int _fd, void* _buf, int _len, int* _status )	{
	    // =-=-=-=-=-=-=-
		// make the call to write
		int status = write( _fd, _buf, _len );

		// =-=-=-=-=-=-=-
		// cache status value into out variable
		(*_status) = status;

		// =-=-=-=-=-=-=-
		// pass along an error if it was not successful
		if (status < 0) {
			status = UNIX_FILE_WRITE_ERR - errno;
			
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileWritePlugin - write fd = ";
			msg << _fd;
			msg << ", status = ";
			msg << _status;
			return ERROR( false, errno, msg.str() );
		}
		
		// =-=-=-=-=-=-=-
		// win!
		return SUCCESS();

	} // unixFileWritePlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Close
	eirods::error unixFileClosePlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   int _fd, int* _status ) {
		int status = -1;

        // =-=-=-=-=-=-=-
		// make the call to close
		status = close( _fd );

		// =-=-=-=-=-=-=-
		// cache status value into out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// handle 0 file descriptor error
		if( _fd == 0 ) {
			rodsLog (LOG_NOTICE, "unixFileClosePlugin: 0 descriptor");
			open ( "/dev/null", O_RDWR, 0 );
		}

        // =-=-=-=-=-=-=-
		// log any error
		if (status < 0) {
			status = UNIX_FILE_CLOSE_ERR - errno;
			
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

            std::stringstream msg;
			msg << "unixFileClosePlugin: close error, ";
			msg << " status = ";
			msg << status;
            return ERROR( false, errno, msg.str() );
		}

		return SUCCESS();

	} // unixFileClosePlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Unlink
	eirods::error unixFileUnlinkPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                    const char* _file_name, int* _status ) {
	    // =-=-=-=-=-=-=-
		// make the call to unlink	
		int status = unlink( _file_name );

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

	    // =-=-=-=-=-=-=-
		// error handling
		if( status < 0 ) {
			status = UNIX_FILE_UNLINK_ERR - errno;
			
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;
            
			std::stringstream msg;
			msg << "unixFileUnlinkPlugin: unlink error for ";
			msg << _file_name;
			msg << ", status = ";
			msg << status;
            return ERROR( false, errno, msg.str() );
		}

		return SUCCESS();

	} // unixFileUnlinkPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Stat
	eirods::error unixFileStatPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                  const char* _file_name, struct stat* _statbuf, int* _status ) {
        // =-=-=-=-=-=-=-
		// make the call to stat
		int status = stat( _file_name, _statbuf );

        // =-=-=-=-=-=-=-
		// if the file can't be accessed due to permission denied 
		// try again using root credentials.
        #ifdef RUN_SERVER_AS_ROOT
		if( status < 0 && errno == EACCES && isServiceUserSet() ) {
			if (changeToRootUser() == 0) {
				status = stat (filename, statbuf);
				changeToServiceUser();
			}
		}
        #endif

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_STAT_ERR - errno;
 
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileStatPlugin: stat error for ";
			msg << _file_name;
			msg << ", status = ";
			msg << status;
            return ERROR( false, errno, msg.str() );
		}
		
		return SUCCESS();

	} // unixFileStatPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX Fstat
	eirods::error unixFileFstatPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   int _fd, struct stat* _statbuf, int* _status ) {
        // =-=-=-=-=-=-=-
		// make the call to fstat
		int status = fstat( _fd, _statbuf );

        // =-=-=-=-=-=-=-
		// if the file can't be accessed due to permission denied 
		// try again using root credentials.
        #ifdef RUN_SERVER_AS_ROOT
		if (status < 0 && errno == EACCES && isServiceUserSet()) {
			if (changeToRootUser() == 0) {
				status = fstat (fd, statbuf);
				changeToServiceUser();
			}
		}
        #endif

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_STAT_ERR - errno;
 
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileFstatPlugin: fstat error for ";
			msg << _fd;
			msg << ", status = ";
			msg << status;

            return ERROR( false, errno, msg.str() );

		} // if
	   
		return SUCCESS();

	} // unixFileFstatPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX lseek
	eirods::error unixFileLseekPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   int _fd, size_t _offset, int _whence, size_t* _status ) {
	    // =-=-=-=-=-=-=-
		// make the call to lseek	
		size_t status = lseek( _fd,  _offset, _whence );

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_LSEEK_ERR - errno;
 
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileLseekPlugin: lseek error for ";
			msg << _fd;
			msg << ", status = ";
			msg << status;
			
		    return ERROR( false, errno, msg.str() );
		}

		return SUCCESS();

	} // unixFileLseekPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX fsync
	eirods::error unixFileFsyncPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   int _fd, int* _status ) {
	    // =-=-=-=-=-=-=-
		// make the call to fsync	
		int status = fsync( _fd );

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_LSEEK_ERR - errno;
 
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileFsyncPlugin: fsync error for ";
			msg << _fd;
			msg << ", status = ";
			msg << status;
			
		    return ERROR( false, errno, msg.str() );
		}

		return SUCCESS();

	} // unixFileFsyncPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX mkdir
	eirods::error unixFileMkdirPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   const char* _file_name, int _mode, int* _status ) {
        // =-=-=-=-=-=-=-
		// make the call to mkdir & umask
		mode_t myMask = umask( ( mode_t ) 0000 );
		int    status = mkdir( _file_name, _mode );

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// reset the old mask 
		umask( ( mode_t ) myMask );

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_MKDIR_ERR - errno;
 
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

		    if (errno != EEXIST) {
				std::stringstream msg;
				msg << "unixFileMkdirPlugin: mkdir error for ";
				msg << _file_name;
				msg << ", status = ";
				msg << status;
				
				return ERROR( false, errno, msg.str() );

			} // if errno != EEXIST

		} // if status < 0 

		return SUCCESS();

	} // unixFileMkdirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX mkdir
	eirods::error unixFileChmodPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   const char* _file_name, int _mode, int* _status ) {
        // =-=-=-=-=-=-=-
		// make the call to chmod
		int status = chmod( _file_name, _mode );

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_CHMOD_ERR - errno;
 
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileChmodPlugin: chmod error for ";
			msg << _file_name;
			msg << ", status = ";
			msg << status;
			
			return ERROR( false, errno, msg.str() );
		} // if

		return SUCCESS();

	} // unixFileChmodPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX mkdir
	eirods::error unixFileRmdirPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   const char* _file_name, int* _status ) {
        // =-=-=-=-=-=-=-
		// make the call to chmod
		int status = rmdir( _file_name );

        // =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_RMDIR_ERR - errno;
 
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileRmdirPlugin: mkdir error for ";
			msg << _file_name;
			msg << ", status = ";
			msg << status;
			
			return ERROR( false, errno, msg.str() );
		}

		return SUCCESS();

	} // unixFileRmdirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX opendir
	eirods::error unixFileOpendirPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                     const char* _dir_name, void** _out_dir_ptr, int* _status ) {
        // =-=-=-=-=-=-=-
		// make the callt to opendir
		DIR* dir_ptr = opendir( _dir_name );

        // =-=-=-=-=-=-=-
		// if the directory can't be accessed due to permission
		// denied try again using root credentials.            
        #ifdef RUN_SERVER_AS_ROOT
		if( dir_ptr == NULL && errno == EACCES && isServiceUserSet() ) {
			if (changeToRootUser() == 0) {
				dir_ptr = opendir ( _dir_name );
				changeToServiceUser();
			} // if
		} // if
        #endif

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( NULL == dir_ptr ) {
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = UNIX_FILE_OPENDIR_ERR - errno;

			std::stringstream msg;
			msg << "unixFileOpendirPlugin: opendir error for ";
			msg << _dir_name;
			msg << ", status = ";
			msg << (*_status);
			
			return ERROR( false, errno, msg.str() );
		}

		// =-=-=-=-=-=-=-
		// cache dir_ptr & status in out variables
		(*_out_dir_ptr) = (void *) dir_ptr;
		(*_status)      = 0;

		return SUCCESS();

	} // unixFileOpendirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX closedir
	eirods::error unixFileClosedirPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                      const char* _dir_name, void* _dir_ptr, int* _status ) {
        // =-=-=-=-=-=-=-
		// make the callt to opendir
		int status = closedir( (DIR*) _dir_ptr );
			
		// =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// return an error if necessary
		if( status < 0 ) {
			status = UNIX_FILE_CLOSEDIR_ERR - errno;

			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = status;

			std::stringstream msg;
			msg << "unixFileClosedirPlugin: closedir error for ";
			msg << _dir_name;
			msg << ", status = ";
			msg << (*_status);
			
			return ERROR( false, errno, msg.str() );
		}

		return SUCCESS();

	} // unixFileClosedirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX readdir
	eirods::error unixFileReaddirPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                     void* _dir_ptr, struct dirent * _dirent_ptr, int* _status ) {
        // =-=-=-=-=-=-=-
		// zero out errno?
		errno = 0;

		// =-=-=-=-=-=-=-
		// make the call to readdir
		struct dirent * tmpDirentPtr = readdir( (DIR*) _dir_ptr );

        // =-=-=-=-=-=-=-
		// handle error cases
		int status = -1;
		if( tmpDirentPtr == NULL ) {
			if( errno == 0 ) { // just the end 
				// =-=-=-=-=-=-=-
				// cache status in out variable
				(*_status) = -1;
				return SUCCESS();
			} else {
				// =-=-=-=-=-=-=-
				// cache status in out variable
				(*_status) = UNIX_FILE_READDIR_ERR - errno;

				std::stringstream msg;
				msg << "unixFileReaddirPlugin: closedir error, status = ";
				msg << (*_status);
				
				return ERROR( false, errno, msg.str() );
			}
		} else {
			// =-=-=-=-=-=-=-
			// cache status and dirent ptr in out variables
			(*_status) = 0;
			(*_dirent_ptr) = (*tmpDirentPtr);

			#if defined(solaris_platform)
			rstrcpy( _dirent_ptr->d_name, tmpDirentPtr->d_name, MAX_NAME_LEN );
			#endif

		    return SUCCESS();
		}

    } // unixFileReaddirPlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX readdir
	eirods::error unixFileStagePlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                   const char* _path, int _flag, int* _status ) {
    #ifdef SAMFS_STAGE
		(*_status) = sam_stage (path, "i");

		if ((*_status) < 0) {
			(*_status) = UNIX_FILE_STAGE_ERR - errno;
			rodsLog( LOG_NOTICE,"unixFileStage: sam_stage error, status = %d\n",
			         (*_status) );
			return ERROR( false, errno, "unixFileStage: sam_stage error" );
		}

		return SUCCESS();
    #else
		return SUCCESS();
    #endif
	} // unixFileStagePlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX readdir
	eirods::error unixFileRenamePlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                    const char* _old_file_name, const char* _new_file_name, int* _status ) {
		// =-=-=-=-=-=-=-
		// make the call to rename
		int status = rename( _old_file_name, _new_file_name );

		// =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// handle error cases
		if( status < 0 ) {
			status = UNIX_FILE_RENAME_ERR - errno;
				
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = UNIX_FILE_READDIR_ERR - errno;

            std::stringstream msg;
			msg << "unixFileRenamePlugin: rename error for ";
			msg << _old_file_name;
			msg << " to ";
			msg << _new_file_name;
			msg << ", status = ";
			msg << (*_status);
			
			return ERROR( false, errno, msg.str() );

		}

		return SUCCESS();

	} // unixFileRenamePlugin

    // =-=-=-=-=-=-=-
	// interface for POSIX truncate
	eirods::error unixFileTruncatePlugin( const char* _file_name, size_t _size, int* _status ) {
		// =-=-=-=-=-=-=-
		// make the call to rename
		int status = truncate( _file_name, _size );

		// =-=-=-=-=-=-=-
		// cache status in out variable
		(*_status) = status;

        // =-=-=-=-=-=-=-
		// handle any error cases
		if( status < 0 ) {
			// =-=-=-=-=-=-=-
			// cache status in out variable
			(*_status) = UNIX_FILE_TRUNCATE_ERR - errno;

            std::stringstream msg;
			msg << "unixFileTruncatePlugin: rename error for ";
			msg << _file_name;
			msg << ", status = ";
			msg << (*_status);
			
			return ERROR( false, errno, msg.str() );
		}

		return SUCCESS();

	} // unixFileTruncatePlugin

	
    // =-=-=-=-=-=-=-
	// interface to determine free space on a device given a path
	eirods::error unixFileGetFsFreeSpacePlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                            const char* _path, int _flag, size_t* _size ) {
		int status = -1;
		rodsLong_t fssize = USER_NO_SUPPORT_ERR;
        #if defined(solaris_platform)
		struct statvfs statbuf;
        #else
		struct statfs statbuf;
        #endif

        #if defined(solaris_platform) || defined(sgi_platform) || defined(aix_platform) || defined(linux_platform) || defined(osx_platform)
            #if defined(solaris_platform)
			status = statvfs( _path, &statbuf );
            #else
                #if defined(sgi_platform)
		        status = statfs( _path, &statbuf, sizeof (struct statfs), 0 );
                #else
		        status = statfs( _path, &statbuf );
                #endif
            #endif

			// =-=-=-=-=-=-=-
			// handle error, if any
			if( status < 0 ) {
				status = UNIX_FILE_GET_FS_FREESPACE_ERR - errno;

				// =-=-=-=-=-=-=-
				// cache error in _size
				(*_size) = USER_NO_SUPPORT_ERR;

				std::stringstream msg;
				msg << "unixFileGetFsFreeSpacePlugin: statfs error for ";
				msg << _path;
				msg << ", status = ";
				msg << status;

				return ERROR( false, errno, msg.str() );
			}
            
			#if defined(sgi_platform)
			if (statbuf.f_frsize > 0) {
				fssize = statbuf.f_frsize;
			} else {
				fssize = statbuf.f_bsize;
			}
			fssize *= statbuf.f_bavail;
            #endif

            #if defined(aix_platform) || defined(osx_platform) || (linux_platform)
	        fssize = statbuf.f_bavail * statbuf.f_bsize;
            #endif
 
            #if defined(sgi_platform)
		    fssize = statbuf.f_bfree * statbuf.f_bsize;
            #endif

        #endif /* solaris_platform, sgi_platform .... */

		// =-=-=-=-=-=-=-
		// cache error in _size
		(*_size) = fssize;

		return SUCCESS();

	} // unixFileGetFsFreeSpacePlugin

	int
	unixFileCopyPlugin (int mode, const char *srcFileName, const char *destFileName)
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
			rodsLog (LOG_ERROR, "unixFileCopyPlugin: stat of %s error, status = %d",
			 srcFileName, status);
		return status;
		}

		inFd = open (srcFileName, O_RDONLY, 0);
		if (inFd < 0 || (statbuf.st_mode & S_IFREG) == 0) {
			status = UNIX_FILE_OPEN_ERR - errno;
			rodsLog (LOG_ERROR,
			   "unixFileCopyPlugin: open error for srcFileName %s, status = %d",
			   srcFileName, status);
			close( inFd ); // JMC cppcheck - resource
			return status;
		}

		outFd = open (destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode);
		if (outFd < 0) {
			status = UNIX_FILE_OPEN_ERR - errno;
			rodsLog (LOG_ERROR,
			 "unixFileCopyPlugin: open error for destFileName %s, status = %d",
			 destFileName, status);
		close (inFd);
			return status;
		}

		while ((bytesRead = read (inFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
		bytesWritten = write (outFd, (void *) myBuf, bytesRead);
		if (bytesWritten <= 0) {
			status = UNIX_FILE_WRITE_ERR - errno;
				rodsLog (LOG_ERROR,
				 "unixFileCopyPlugin: write error for srcFileName %s, status = %d",
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
			rodsLog (LOG_ERROR,
			 "unixFileCopyPlugin: Copied size %lld does not match source size %lld of %s",
			 bytesCopied, statbuf.st_size, srcFileName);
			return SYS_COPY_LEN_ERR;
		} else {
		return 0;
		}
	}


    // =-=-=-=-=-=-=-
	// unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
	// Just copy the file from filename to cacheFilename. optionalInfo info
	// is not used.
	eirods::error unixStageToCachePlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                      const char* _file_name, const char* _cache_file_name, 
	                                      int _mode, int flags,  size_t _data_size, 
										  keyValPair_t *condInput, int* _status ) {
		(*_status) = unixFileCopyPlugin( _mode, _file_name, _cache_file_name );
        if( (*_status) < 0 ) {
            return ERROR( false, (*_status), "unixStageToCachePlugin failed." );
		} else {
            return SUCCESS();
		}
	} // unixStageToCachePlugin

    // =-=-=-=-=-=-=-
	// unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
	// Just copy the file from cacheFilename to filename. optionalInfo info
	// is not used.
	eirods::error unixSyncToArchPlugin( eirods::resource_property_map* _prop_map, eirods::resource_child_map* _cmap,
	                                    const char* _file_name, char* _cache_file_name, int _mode, int _flags,  
										rodsLong_t _data_size, keyValPair_t* _cond_input, int* _status ) {

		(*_status) = unixFileCopyPlugin( _mode, _cache_file_name, _file_name );
        if( (*_status) < 0 ) {
            return ERROR( false, (*_status), "unixSyncToArchPlugin failed." );
		} else {
            return SUCCESS();
		}

	} // unixSyncToArchPlugin


#if 0
	int
	nbFileReadPlugin (rsComm_t *rsComm, int fd, void *buf, int len)
	{
		int status;
		struct timeval tv;
		int nbytes;
		int toRead;
		char *tmpPtr;
		fd_set set;

		bzero (&tv, sizeof (tv));
		tv.tv_sec = NB_READ_TOUT_SEC;

		/* Initialize the file descriptor set. */
		FD_ZERO (&set);
		FD_SET (fd, &set);

		toRead = len;
		tmpPtr = (char *) buf;

		while (toRead > 0) {
#ifndef _WIN32
			status = select (fd + 1, &set, NULL, NULL, &tv);
			if (status == 0) {
				/* timedout */
				if (len - toRead > 0) {
					return (len - toRead);
				} else {
					return UNIX_FILE_OPR_TIMEOUT_ERR - errno;
				}
			} else if (status < 0) {
				if ( errno == EINTR) {
					errno = 0;
					continue;
				} else {
					return UNIX_FILE_READ_ERR - errno;
				}
			}
#endif
			nbytes = read (fd, (void *) tmpPtr, toRead);
			if (nbytes < 0) {
				if (errno == EINTR) {
					/* interrupted */
					errno = 0;
					nbytes = 0;
				} else if (toRead == len) {
					return UNIX_FILE_READ_ERR - errno;
				} else {
					nbytes = 0;
					break;
				}
			} else if (nbytes == 0) {
				break;
			}
			toRead -= nbytes;
			tmpPtr += nbytes;
		}
		return (len - toRead);
	}
	 
	int
	nbFileWritePlugin (rsComm_t *rsComm, int fd, void *buf, int len)
	{
		int nbytes;
		int toWrite;
		char *tmpPtr;
		fd_set set;
		struct timeval tv;
		int status;

		bzero (&tv, sizeof (tv));
		tv.tv_sec = NB_WRITE_TOUT_SEC;

		/* Initialize the file descriptor set. */
		FD_ZERO (&set);
		FD_SET (fd, &set);

		toWrite = len;
		tmpPtr = (char *) buf;

		while (toWrite > 0) {
#ifndef _WIN32
			status = select (fd + 1, NULL, &set, NULL, &tv);
			if (status == 0) {
				/* timedout */
				return UNIX_FILE_OPR_TIMEOUT_ERR - errno;
			} else if (status < 0) {
				if ( errno == EINTR) {
					errno = 0;
					continue;
				} else {
					return UNIX_FILE_WRITE_ERR - errno;
				}
			}
#endif
			nbytes = write (fd, (void *) tmpPtr, len);
			if (nbytes < 0) {
				if (errno == EINTR) {
					/* interrupted */
					errno = 0;
					nbytes = 0;
				} else  {
					return UNIX_FILE_WRITE_ERR - errno;
			}
			}
			toWrite -= nbytes;
			tmpPtr += nbytes;
		}
		return (len);
	}
#endif







    // =-=-=-=-=-=-=-
	// First Pass at a resource plugin
    class eirods_unix_file_resource_plugin : public eirods::resource {
        public: 
		eirods_unix_file_resource_plugin() : eirods::resource() {}

    }; // class eirods_unix_file_resource_plugin

    // =-=-=-=-=-=-=-
	// 2.  Create the plugin factory function which will return a microservice
	//     table entry containing the microservice function pointer, the number
	//     of parameters that the microservice takes and the name of the micro
	//     service.  this will be called by the plugin loader in the irods server
	//     to create the entry to the table when the plugin is requested.
	eirods::resource* plugin_factory( ) {
		//eirods_unix_file_resource_plugin* resc = new eirods_unix_file_resource_plugin();
		eirods::resource* resc = new eirods::resource();

        resc->add_operation( "create",       "unixFileCreatePlugin" );
        resc->add_operation( "open",         "unixFileOpenPlugin" );
        resc->add_operation( "read",         "unixFileReadPlugin" );
        resc->add_operation( "write",        "unixFileWritePlugin" );
        resc->add_operation( "close",        "unixFileClosePlugin" );
        resc->add_operation( "unlink",       "unixFileUnlinkPlugin" );
        resc->add_operation( "stat",         "unixFileStatPlugin" );
        resc->add_operation( "fstat",        "unixFileFstatPlugin" );
        resc->add_operation( "fsync",        "unixFileFsyncPlugin" );
        resc->add_operation( "mkdir",        "unixFileMkdirPlugin" );
        resc->add_operation( "chmod",        "unixFileChmodPlugin" );
        resc->add_operation( "opendir",      "unixFileOpendirPlugin" );
        resc->add_operation( "readdir",      "unixFileReaddirPlugin" );
        resc->add_operation( "stage",        "unixFileStagePlugin" );
        resc->add_operation( "rename",       "unixFileRenamePlugin" );
        resc->add_operation( "freespace",    "unixFileGetFsFreeSpacePlugin" );
        resc->add_operation( "lseek",        "unixFileLseekPlugin" );
        resc->add_operation( "rmdir",        "unixFileRmdirPlugin" );
        resc->add_operation( "closedir",     "unixFileClosedirPlugin" );
        resc->add_operation( "truncate",     "unixFileTruncatePlugin" );
        resc->add_operation( "stagetocache", "unixStageToCachePlugin" );
        resc->add_operation( "synctoarch",   "unixSyncToArchPlugin" );

	    //return dynamic_cast<eirods::resource*>( resc );
        return resc;
	} // plugin_factory


}; // extern "C" 



