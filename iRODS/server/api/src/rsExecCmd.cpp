/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjRead.h for a description of this API call.*/

#include <sys/types.h>
#ifndef windows_platform
#include <sys/wait.h>
#else
#include "Unix2Nt.hpp"
#endif
#include "execCmd.hpp"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "miscServerFunct.hpp"
#include "rodsLog.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"

#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"

#include <boost/thread/mutex.hpp>
boost::mutex ExecCmdMutex;
int initExecCmdMutex() {
    return 0;
}

int
rsExecCmd( rsComm_t *rsComm, execCmd_t *execCmdInp, execCmdOut_t **execCmdOut ) {
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    rodsHostAddr_t addr;
    irods::error err = SUCCESS();

    /* some sanity check on the cmd path */
    if ( strchr( execCmdInp->cmd, '/' ) != NULL ) {
        rodsLog( LOG_ERROR,
                 "rsExecCmd: bad cmd path %s", execCmdInp->cmd );
        return BAD_EXEC_CMD_PATH;
    }

    /* Also check for anonymous.  As an additional safety precaution,
       by default, do not allow the anonymous user (if defined) to
       execute commands via rcExecCmd.  If your site needs to allow
       this for some particular feature, you can remove the
       following check.
    */
    if ( strncmp( ANONYMOUS_USER, rsComm->clientUser.userName, NAME_LEN ) == 0 ) {
        return USER_NOT_ALLOWED_TO_EXEC_CMD;
    }

    memset( &addr, 0, sizeof( addr ) );
    if ( *execCmdInp->hintPath != '\0' ) {
        dataObjInp_t dataObjInp;
        memset( &dataObjInp, 0, sizeof( dataObjInp ) );
        rstrcpy( dataObjInp.objPath, execCmdInp->hintPath, MAX_NAME_LEN );

        // =-=-=-=-=-=-=-
        // determine the resource hierarchy if one is not provided
        std::string resc_hier;
        char*       resc_hier_ptr = getValByKey( &dataObjInp.condInput, RESC_HIER_STR_KW );
        if ( resc_hier_ptr == NULL ) {
            irods::error ret = irods::resolve_resource_hierarchy( irods::OPEN_OPERATION,
                               rsComm, &dataObjInp, resc_hier );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << "failed in irods::resolve_resource_hierarchy for [";
                msg << dataObjInp.objPath << "]";
                irods::log( PASSMSG( msg.str(), ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // we resolved the redirect and have a host, set the hier str for subsequent
            // api calls, etc.
            addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, resc_hier.c_str() );
            addKeyVal( &execCmdInp->condInput, RESC_HIER_STR_KW, resc_hier.c_str() );
        }
        else {
            resc_hier = resc_hier_ptr;

        }

        status = getDataObjInfo( rsComm, &dataObjInp, &dataObjInfoHead, ACCESS_READ_OBJECT, 0 );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsExecCmd: getDataObjInfo error for hintPath %s",
                     execCmdInp->hintPath );
            return status;
        }

        status = sortObjInfoForOpen( &dataObjInfoHead, &execCmdInp->condInput, 0 );

        if ( status < 0 || NULL == dataObjInfoHead ) {
            return status;    // JMC cppcheck nullptr
        }

        if ( execCmdInp->addPathToArgv > 0 ) {
            char tmpArgv[HUGE_NAME_LEN];
            rstrcpy( tmpArgv, execCmdInp->cmdArgv, HUGE_NAME_LEN );
            snprintf( execCmdInp->cmdArgv, HUGE_NAME_LEN, "%s %s",
                      dataObjInfoHead->filePath, tmpArgv );
        }

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        err = irods::get_loc_for_hier_string( dataObjInfoHead->rescHier, location );
        if ( !err.ok() ) {
            irods::log( PASSMSG( "rsExecCmd - failed in get_loc_for_hier_string", err ) );
            return err.code();
        }

        // =-=-=-=-=-=-=-
        // extract zone name from resource hierarchy
        std::string zone_name;
        err = irods::get_resc_hier_property<std::string>( dataObjInfoHead->rescHier, irods::RESOURCE_ZONE, zone_name );
        if ( !err.ok() ) {
            irods::log( PASSMSG( "rsExecCmd - failed in get_resc_hier_property", err ) );
            return err.code();
        }

        rstrcpy( addr.zoneName, zone_name.c_str(), NAME_LEN );
        rstrcpy( addr.hostAddr, location.c_str(), LONG_NAME_LEN );

        /* just in case we have to do it remote */
        *execCmdInp->hintPath = '\0';	/* no need for hint */
        rstrcpy( execCmdInp->execAddr, location.c_str(),
                 LONG_NAME_LEN );
        freeAllDataObjInfo( dataObjInfoHead );
        remoteFlag = resolveHost( &addr, &rodsServerHost );
    }
    else if ( *execCmdInp->execAddr  != '\0' ) {
        rstrcpy( addr.hostAddr, execCmdInp->execAddr, LONG_NAME_LEN );
        remoteFlag = resolveHost( &addr, &rodsServerHost );
    }
    else {
        rodsServerHost = LocalServerHost;
        remoteFlag = LOCAL_HOST;
    }

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsExecCmd( execCmdInp, execCmdOut );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteExecCmd( rsComm, execCmdInp, execCmdOut,
                                rodsServerHost );
    }
    else {
        rodsLog( LOG_NOTICE,
                 "rsFileOpenByHost: resolveHost of %s error, status = %d",
                 addr.hostAddr, remoteFlag );
        status = SYS_UNRECOGNIZED_REMOTE_FLAG;
    }

    return status;
}

int
remoteExecCmd( rsComm_t *rsComm, execCmd_t *execCmdInp,
               execCmdOut_t **execCmdOut, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteExecCmd: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcExecCmd( rodsServerHost->conn, execCmdInp, execCmdOut );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "remoteExecCmd: rcExecCmd failed for %s. status = %d",
                 execCmdInp->cmd, status );
    }
    else if ( status > 0 &&
              getValByKey( &execCmdInp->condInput, STREAM_STDOUT_KW ) != NULL ) {
        int fileInx = status;
        ( *execCmdOut )->status = bindStreamToIRods( rodsServerHost, fileInx );
        if ( ( *execCmdOut )->status < 0 ) {
            fileCloseInp_t remFileCloseInp;
            rodsLog( LOG_ERROR,
                     "remoteExecCmd: bindStreamToIRods failed. status = %d",
                     ( *execCmdOut )->status );
            memset( &remFileCloseInp, 0, sizeof( remFileCloseInp ) );
            remFileCloseInp.fileInx = fileInx;
            rcFileClose( rodsServerHost->conn, &remFileCloseInp );
        }
        status = ( *execCmdOut )->status;
    }
    else {
        status = 0;
    }
    return status;
}

int
_rsExecCmd( execCmd_t *execCmdInp, execCmdOut_t **execCmdOut ) {
    int childPid;
    int stdoutFd[2];
    int stderrFd[2];
    int statusFd[2];
    execCmdOut_t *myExecCmdOut;
    bytesBuf_t statusBuf;
    int status, childStatus;

#ifndef windows_platform    /* UNIX */
    ExecCmdMutex.lock();
    if ( pipe( stdoutFd ) < 0 )
#else
    int pipe_buf_size = META_STR_LEN;
    if ( _pipe( stdoutFd, pipe_buf_size, O_BINARY ) < 0 )
#endif
    {
        rodsLog( LOG_ERROR,
                 "_rsExecCmd: pipe create failed. errno = %d", errno );
        ExecCmdMutex.unlock();
        return SYS_PIPE_ERROR - errno;
    }

#ifndef windows_platform    /* UNIX */
    if ( pipe( stderrFd ) < 0 )
#else
    if ( _pipe( stderrFd, pipe_buf_size, O_BINARY ) < 0 )
#endif
    {
        rodsLog( LOG_ERROR,
                 "_rsExecCmd: pipe create failed. errno = %d", errno );
        ExecCmdMutex.unlock();
        return SYS_PIPE_ERROR - errno;
    }

#ifndef windows_platform    /* UNIX */
    if ( pipe( statusFd ) < 0 )
#else
    if ( _pipe( statusFd, pipe_buf_size, O_BINARY ) < 0 )
#endif
    {
        rodsLog( LOG_ERROR,
                 "_rsExecCmd: pipe create failed. errno = %d", errno );
        ExecCmdMutex.unlock();
        return SYS_PIPE_ERROR - errno;
    }

#ifndef windows_platform   /* UNIX */
    /* use fork instead of vfork to handle mylti-thread */
    childPid = fork();

    ExecCmdMutex.unlock();
    if ( childPid == 0 ) {
        char *tmpStr;
        /* Indicate that the call came from internal rule */
        if ( ( tmpStr = getValByKey( &execCmdInp->condInput, EXEC_CMD_RULE_KW ) )
                != NULL ) {
            char *myStr = ( char* )malloc( NAME_LEN + 20 );
            snprintf( myStr, NAME_LEN + 20, "%s=%s", EXEC_CMD_RULE_KW, tmpStr );
            putenv( myStr );
            //free( myStr ); // JMC cppcheck - leak	 ==> backport 'fix' from comm trunk for solaris
        }
        close( stdoutFd[0] );
        close( stderrFd[0] );
        close( statusFd[0] );
        status = execCmd( execCmdInp, stdoutFd[1], stderrFd[1] );
        if ( status < 0 ) {
            status = EXEC_CMD_ERROR - errno;
        }
        /* send the status back to parent */
        if ( write( statusFd[1], &status, 4 ) == -1 ) {
            int errsv = errno;
            irods::log( ERROR( errsv, "Write failed when sending status back to parent." ) );
        }
        /* gets here. must be bad */
        exit( 1 );
    }
    else if ( childPid < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsExecCmd: RODS_FORK failed. errno = %d", errno );
        return SYS_FORK_ERROR;
    }
#else  /* Windows */
    status = execCmd( execCmdInp, stdoutFd[1], stderrFd[1] );
    if ( status < 0 ) {
        status = EXEC_CMD_ERROR - errno;
    }
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "_rsExecCmd: RODS_FORK failed. errno = %d", errno );
        return SYS_FORK_ERROR;
    }
#endif


    /* the parent gets here */
#ifndef windows_platform
    close( stdoutFd[1] );
    close( stderrFd[1] );
    close( statusFd[1] );
#endif


    myExecCmdOut = *execCmdOut = ( execCmdOut_t* )malloc( sizeof( execCmdOut_t ) );
    memset( myExecCmdOut, 0, sizeof( execCmdOut_t ) );

    readToByteBuf( stdoutFd[0], &myExecCmdOut->stdoutBuf );
    if ( getValByKey( &execCmdInp->condInput, STREAM_STDOUT_KW ) != NULL &&
            myExecCmdOut->stdoutBuf.len >= MAX_SZ_FOR_EXECMD_BUF ) {
        /* more to come. don't close stdoutFd. close stderrFd and statusFd
         * because the child is not done */
        close( stderrFd[0] );
        close( statusFd[0] );
        myExecCmdOut->status = bindStreamToIRods( LocalServerHost, stdoutFd[0] );
        if ( myExecCmdOut->status < 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsExecCmd: bindStreamToIRods failed. status = %d",
                     myExecCmdOut->status );
            close( stdoutFd[0] );
        }
    }
    else {
        close( stdoutFd[0] );
        readToByteBuf( stderrFd[0], &myExecCmdOut->stderrBuf );
        close( stderrFd[0] );
        memset( &statusBuf, 0, sizeof( statusBuf ) );
        readToByteBuf( statusFd[0], &statusBuf );
        close( statusFd[0] );
        if ( statusBuf.len == sizeof( int ) + 1 ) {
            myExecCmdOut->status = *( ( int * )statusBuf.buf );
            free( statusBuf.buf );
        }
        childStatus = 0;

#ifndef windows_platform   /* UNIX */
        status = waitpid( childPid, &childStatus, 0 );
        if ( status >= 0 && myExecCmdOut->status >= 0 && childStatus != 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsExecCmd: waitpid status = %d, myExecCmdOut->status = %d, childStatus = %d", status, myExecCmdOut->status, childStatus );
            myExecCmdOut->status = EXEC_CMD_ERROR;
        }
#endif
    } // else
    return myExecCmdOut->status;
}

int
execCmd( execCmd_t *execCmdInp, int stdOutFd, int stdErrFd ) {
    char cmdPath[LONG_NAME_LEN];
    char *av[LONG_NAME_LEN];
    int status;


    snprintf( cmdPath, LONG_NAME_LEN, "%s/%s", CMD_DIR, execCmdInp->cmd );
    rodsLog( LOG_NOTICE, "execCmd:%s argv:%s", cmdPath, execCmdInp->cmdArgv );
    initCmdArg( av, execCmdInp->cmdArgv, cmdPath );

    closeAllL1desc( ThisComm );

#ifndef windows_platform
    /* set up the pipe as the stdin, stdout and stderr */

    close( 0 );
    close( 1 );
    close( 2 );

    dup2( stdOutFd, 0 );
    dup2( stdOutFd, 1 );
    dup2( stdErrFd, 2 );
    close( stdOutFd );
    close( stdErrFd );

    status = execv( av[0], av );

#else /* Windows: Can Windows redirect the stdin, etc, to a pipe? */
    status = _spawnv( _P_NOWAIT, av[0], av );
#endif

    return status;
}

int
initCmdArg( char *av[], char *cmdArgv, char *cmdPath ) {
    int avInx = 0, i;
    char *startPtr, *curPtr;
    int quoteCnt, curLen;
    char tmpCmdArgv[HUGE_NAME_LEN];

    av[avInx] = strdup( cmdPath );
    avInx++;

    /* parse cmdArgv */

    if ( strlen( cmdArgv ) > 0 ) {
        rstrcpy( tmpCmdArgv, cmdArgv, HUGE_NAME_LEN );
        startPtr = curPtr = tmpCmdArgv;
        quoteCnt = curLen = 0;
        while ( *curPtr != '\0' ) {
            if ( ( *curPtr == ' ' && quoteCnt == 0 && curLen > 0 ) ||
                    quoteCnt == 2 ) {
                /* end of a argv */
                *curPtr = '\0';		/* mark the end of argv */
                curPtr++;
                av[avInx] = strdup( startPtr );
                avInx++;
                startPtr = curPtr;
                quoteCnt = curLen = 0;
            }
            else if ( *curPtr == ' ' && quoteCnt <= 1 && curLen == 0 ) {
                /* skip over a leading blank */
                curPtr++;
                startPtr = curPtr;
                /**  Added by Raja to take care of escaped quotes Oct 28 09 */
            }
            else if ( ( *curPtr == '\'' || *curPtr == '\"' )
                      && ( *( curPtr - 1 ) == '\\' ) ) {
                curPtr++;
                curLen++;
                /**  Added by Raja to take care of escaped quotes Oct 28 09 */
            }
            else if ( *curPtr == '\'' || *curPtr == '\"' ) {
                quoteCnt++;
                /* skip over the quote */
                if ( quoteCnt == 1 ) {
                    curPtr++;
                    startPtr = curPtr;
                }
            }
            else {
                /* just normal char */
                curPtr++;
                curLen++;
            }
        }
        /* The last one */
        if ( curLen > 0 ) {
            av[avInx] = strdup( startPtr );
            avInx++;
        }
    }

    /* put a NULL on the last one */

    av[avInx] = NULL;

    /**  Added by Raja to take care of escaped quotes Oct 28 09 */
    for ( i = 0; i < avInx ; i++ ) {
        curPtr = av[i];
        startPtr = curPtr;
        while ( *curPtr != '\0' ) {
            if ( *curPtr == '\\' && ( *( curPtr + 1 ) == '\'' || *( curPtr + 1 ) == '\"' ) ) {
                curPtr++;
            }
            *startPtr = *curPtr;
            curPtr++;
            startPtr++;
        }
        *startPtr = '\0';
    }
    /**  Added by Raja to take care of escaped quotes Oct 28 09 */

    return 0;
}

