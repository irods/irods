/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* User command to change their password. */
#include "rods.hpp"
#include "rodsClient.hpp"
#include <unistd.h>
#include <termios.h>
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

#include <iostream>
#include <string>

void usage( char *prog );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int i, ix, status;
    int echoFlag = 0;
    char *password;
    rodsEnv myEnv;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;

    char newPw[MAX_PASSWORD_LEN + 10];
    char newPw2[MAX_PASSWORD_LEN + 10];
    int len, lcopy;

    char buf0[MAX_PASSWORD_LEN + 10];
    char buf1[MAX_PASSWORD_LEN + 10];
    char buf2[MAX_PASSWORD_LEN + 10];

    userAdminInp_t userAdminInp;

    /* this is a random string used to pad, arbitrary, but must match
       the server side: */
    char rand[] = "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs";

    status = parseCmdLineOpt( argc, argv, "ehfvVl", 0, &myRodsArgs );
    if ( status != 0 ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }

    if ( myRodsArgs.echo == True ) {
        echoFlag = 1;
    }
    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        exit( 0 );
    }

    if ( myRodsArgs.longOption == True ) {
        rodsLogLevel( LOG_NOTICE );
    }

    ix = myRodsArgs.optind;

    password = "";
    if ( ix < argc ) {
        password = argv[ix];
    }

    status = getRodsEnv( &myEnv ); /* Need to get irodsAuthFileName (if set) */
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    if ( myRodsArgs.verbose == True ) {
        i = obfSavePw( echoFlag, 1, 1, password );
    }
    else {
        i = obfSavePw( echoFlag, 0, 0, password );
    }

    if ( i != 0 ) {
        rodsLogError( LOG_ERROR, i, "Save Password failure" );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    /* Connect... */
    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );
    if ( Conn == NULL ) {
        rodsLog( LOG_ERROR,
                 "Saved password, but failed to connect to server %s",
                 myEnv.rodsHost );
        exit( 2 );
    }

    /* and check that the user/password is OK */
    status = clientLogin( Conn );
    if ( status != 0 ) {
        rcDisconnect( Conn );
        exit( 7 );
    }

    /* get the new password */
#ifdef WIN32
    HANDLE hStdin = GetStdHandle( STD_INPUT_HANDLE );
    DWORD mode;
    GetConsoleMode( hStdin, &mode );
    DWORD lastMode = mode;
    mode &= ~ENABLE_ECHO_INPUT;
    BOOL error = !SetConsoleMode( hStdin, mode );
    int errsv = -1;
#else
    struct termios tty;
    tcgetattr( STDIN_FILENO, &tty );
    tcflag_t oldflag = tty.c_lflag;
    tty.c_lflag &= ~ECHO;
    int error = tcsetattr( STDIN_FILENO, TCSANOW, &tty );
    int errsv = errno;
#endif
    if ( error ) {
        printf( "WARNING: Error %d disabling echo mode. Password will be displayed in plaintext.", errsv );
    }

    len = 0;
    for ( ; len < 4; ) {
        printf( "Enter your new iRODS password:" );
        std::string password = "";
        getline( std::cin, password );
        strncpy( newPw, password.c_str(), MAX_PASSWORD_LEN );
        len = strlen( newPw );
        if ( len < 4 ) {
            printf( "\nYour password must be at least 3 characters long.\n" );
        }
    }
    if ( myRodsArgs.force != True ) {
        printf( "Reenter your new iRODS password:" );
        std::string password = "";
        getline( std::cin, password );
        strncpy( newPw2, password.c_str(), MAX_PASSWORD_LEN );
        if ( strncmp( newPw, newPw2, MAX_PASSWORD_LEN ) != 0 ) {
            printf( "Entered passwords do not match\n" );
#ifdef WIN32
            if ( !SetConsoleMode( hStdin, lastMode ) ) {
                printf( "Error reinstating echo mode." );
            }
#else
            tty.c_lflag = oldflag;
            if ( tcsetattr( STDIN_FILENO, TCSANOW, &tty ) ) {
                printf( "Error reinstating echo mode." );
            }
#endif
            rcDisconnect( Conn );
            exit( 8 );
        }
    }
#ifdef WIN32
    if ( SetConsoleMode( hStdin, lastMode ) ) {
        printf( "Error reinstating echo mode." );
    }
#else
    tty.c_lflag = oldflag;
    if ( tcsetattr( STDIN_FILENO, TCSANOW, &tty ) ) {
        printf( "Error reinstating echo mode." );
    }
#endif

    strncpy( buf0, newPw, MAX_PASSWORD_LEN );
    len = strlen( newPw );
    lcopy = MAX_PASSWORD_LEN - 10 - len;
    if ( lcopy > 15 ) {
        /* server will look for 15 characters
        		  of random string */
        strncat( buf0, rand, lcopy );
    }
    i = obfGetPw( buf1 );
    if ( i != 0 ) {
        printf( "Error getting current password\n" );
        exit( 1 );
    }
    obfEncodeByKey( buf0, buf1, buf2 );

    userAdminInp.arg0 = "userpw";
    userAdminInp.arg1 = myEnv.rodsUserName;
    userAdminInp.arg2 = "password";
    userAdminInp.arg3 = buf2;
    userAdminInp.arg4 = "";
    userAdminInp.arg5 = "";
    userAdminInp.arg6 = "";
    userAdminInp.arg7 = "";
    userAdminInp.arg8 = "";
    userAdminInp.arg9 = "";

    status = rcUserAdmin( Conn, &userAdminInp );
    if ( status != 0 ) {
        rodsLog( LOG_ERROR, "rcUserAdmin error. status = %d",
                 status );

    }
    else {
        /* initialize with the new password */
        i = obfSavePw( 0, 0, 0, newPw );
    }

    printErrorStack( Conn->rError );

    rcDisconnect( Conn );

    exit( 0 );
}


void usage( char *prog ) {
    printf( "Changes your irods password and, like iinit, stores your new iRODS\n" );
    printf( "password in a scrambled form to be used automatically by the icommands.\n" );
    printf( "Prompts for your old and new passwords.\n" );
    printf( "Usage: %s [-hvVl]\n", prog );
    printf( " -v  verbose\n" );
    printf( " -V  Very verbose\n" );
    printf( " -l  long format (somewhat verbose)\n" );
    printf( " -e  echo the password as entered\n" );
    printf( " -f  force: do not ask user to reenter the new password\n" );
    printf( " -h  this help\n" );
    printReleaseInfo( "ipasswd" );
}
