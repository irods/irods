/*
 * iexecmd - The irods utility to execute user composed rules.
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

void usage();

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    char *optStr;
    execCmd_t execCmd;
    execCmdOut_t *execCmdOut = NULL;
    int nArg;
    char *tmpPtr;

    optStr = "hvH:p:P:";

    status = parseCmdLineOpt( argc, argv, optStr, 1, &myRodsArgs );

    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    nArg = argc - optind;

    if ( nArg <= 0 || nArg > 1 ) {
        rodsLog( LOG_ERROR, "iexecmd: no input or too many input" );
        printf( "Use -h for help.\n" );
        exit( 2 );
    }

    if ( myRodsArgs.physicalPath == True && myRodsArgs.logicalPath == True ) {
        rodsLog( LOG_ERROR,
                 "iexecmd: -p and -P options cannot be used together" );
        exit( 3 );
    }

    memset( &execCmd, 0, sizeof( execCmd ) );

    if ( ( tmpPtr = strchr( argv[optind], ' ' ) ) != NULL &&
            tmpPtr != argv[optind] ) {
        /* have argv in the command */
        *tmpPtr = '\0';
        rstrcpy( execCmd.cmd, argv[optind], LONG_NAME_LEN );
        tmpPtr ++;
        rstrcpy( execCmd.cmdArgv, tmpPtr, HUGE_NAME_LEN );
    }
    else {
        rstrcpy( execCmd.cmd, argv[optind], LONG_NAME_LEN );
    }

    if ( myRodsArgs.hostAddr == True ) {
        rstrcpy( execCmd.execAddr, myRodsArgs.hostAddrString, LONG_NAME_LEN );
    }

    if ( myRodsArgs.physicalPath == True ) {
        rstrcpy( execCmd.hintPath, myRodsArgs.physicalPathString,
                 LONG_NAME_LEN );
        execCmd.addPathToArgv = 0;
    }
    else if ( myRodsArgs.logicalPath == True ) {
        rstrcpy( execCmd.hintPath, myRodsArgs.logicalPathString,
                 LONG_NAME_LEN );
        execCmd.addPathToArgv = 1;
    }
    addKeyVal( &execCmd.condInput, STREAM_STDOUT_KW, "" );

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        exit( 7 );
    }

    status = rcExecCmd( conn, &execCmd, &execCmdOut );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "rcExecCmd error. " );
        printErrorStack( conn->rError );
        if ( execCmdOut != NULL && execCmdOut->stderrBuf.buf != NULL ) {
            fprintf( stderr, "%s", ( char * ) execCmdOut->stderrBuf.buf );
        }

        rcDisconnect( conn );
        exit( 4 );
    }


    if ( myRodsArgs.verbose == True ) {
        printf( "rcExecCmd completed successfully.    Output \n\n" );
    }

    if ( execCmdOut != NULL ) {
        int i;
        char *tmpPtr;
        fileReadInp_t streamReadInp;
        fileCloseInp_t streamCloseInp;
        bytesBuf_t *streamReadOutBBuf;
        int bytesRead;

        if ( execCmdOut->stdoutBuf.buf != NULL ) {
            tmpPtr = ( char* )execCmdOut->stdoutBuf.buf;
            for ( i = 0; i < execCmdOut->stdoutBuf.len; i++ ) {
                fputc( ( int )( *tmpPtr ), stdout );
                tmpPtr++;
            }
        }
        if ( execCmdOut->stderrBuf.buf != NULL ) {
            tmpPtr = ( char* )execCmdOut->stderrBuf.buf;
            for ( i = 0; i < execCmdOut->stderrBuf.len; i++ ) {
                fputc( ( int )( *tmpPtr ), stderr );
                tmpPtr++;
            }
        }
        if ( execCmdOut->status > 0 ) {
            /* (execCmdOut->status is a stream descriptor */
            bzero( &streamReadInp, sizeof( streamReadInp ) );
            streamReadOutBBuf = &execCmdOut->stdoutBuf;
            if ( streamReadOutBBuf->len < MAX_SZ_FOR_EXECMD_BUF ) {
                if ( streamReadOutBBuf->buf != NULL ) {
                    free( streamReadOutBBuf->buf );
                }
                streamReadOutBBuf->buf = malloc( MAX_SZ_FOR_EXECMD_BUF );
            }
            streamReadOutBBuf->len = streamReadInp.len = MAX_SZ_FOR_EXECMD_BUF;
            streamReadInp.fileInx = execCmdOut->status;
            while ( ( bytesRead = rcStreamRead( conn, &streamReadInp,
                                                streamReadOutBBuf ) ) > 0 ) {
                tmpPtr = ( char* )streamReadOutBBuf->buf;
                for ( i = 0; i < bytesRead; i++ ) {
                    fputc( ( int )( *tmpPtr ), stdout );
                    tmpPtr++;
                }
            }
            streamCloseInp.fileInx = execCmdOut->status;
            rcStreamClose( conn, &streamCloseInp );
        }
    }
    rcDisconnect( conn );

    exit( 0 );
}

void
usage() {
    char *msgs[] = {
        "Usage : iexecmd [-hv] [-p hintPath]|[-P hintPath] [-H hostAddr] command",
        "Remotely Execute a command installed in the server/bin/cmd directory of ",
        "the server.",
        "The input parameter 'command' is the remote command to execute. Input",
        "arguments  for the command is supported by appending arguments to the",
        "command to be executed. The command and arguments must be in quotes to",
        "differentiate the arguments from the arguments of the iexecmd. e.g.:",
        "    iexecmd -H zero.sdsc.edu \"hello Mary\"",
        "If neither -H, -p nor -P is specified, the command will be executed on",
        "the host where the client is connected.",
        " ",
        "Options are:",
        " -H  hostAddr - the host address where the command will be executed.",
        " -p  hintPath - A full iRODS file path. The command will be executed on",
        "      the host where this file is stored.",
        " -P  hintPath - Same as the -p option except that the resolved pphysical",
        "      path from the logical hintPath will be used as the first argument",
        "      the command",
        " -v  verbose",
        " -h  this help",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "iexecmd" );
}
