#include "utility.hpp"
#include <irods/authentication_plugin_framework.hpp>
#include <irods/rods.h>
#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <unistd.h>
#include <termios.h>
#include <irods/termiosUtil.hpp>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>

#include <cstdio>
#include <iostream>
#include <string>

namespace irods_auth = irods::authentication;

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

    // This must be done before parseCmdLineOpt! parseCmdLineOpt is EVIL and considers any unknown options invalid.
    const auto no_scramble = utils::option_specified("--no-scramble", argc, argv);

    status = parseCmdLineOpt(argc, argv, "ehfvVlZ", 0, &myRodsArgs);
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

    status = getRodsEnv( &myEnv ); /* Need to get irodsAuthFile (if set) */
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    const auto using_native = std::string_view{"native"} == myEnv.rodsAuthScheme;

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

    // If --no-scramble was specified, make sure the UserAdmin API of the connected server actually supports the
    // feature. If not, exit with an error. The password will not be usable if it is set with this option and the
    // server does not support this feature.
    if (no_scramble) {
        const auto version = irods::to_version(Conn->svrVersion->relVersion);
        if (version && version.value() < irods::version{5, 0, 90}) {
            fmt::print("Setting password failed: --no-scramble option is not supported before iRODS 5.1.0.\n");
            rcDisconnect(Conn);
            exit(3);
        }
    }

    // This section maintains historical ipasswd behavior for native authentication.
    if (using_native) {
        if (myRodsArgs.verbose == True) {
            i = obfSavePw(echoFlag, 1, 1, password);
        }
        else {
            i = obfSavePw(echoFlag, 0, 0, password);
        }

        if (i != 0) {
            rodsLogError(LOG_ERROR, i, "Save Password failure");
            exit(1);
        }

        /* and check that the user/password is OK */
        status = utils::authenticate_client(Conn, myEnv);
    }
    else {
        namespace ia = irods::authentication;
        const auto ctx = nlohmann::json{
            {ia::scheme_name, myEnv.rodsAuthScheme}, {ia::record_auth_file, true}, {ia::force_password_prompt, true}};
        status = ia::authenticate_client(*Conn, ctx);
    }
    if ( status != 0 ) {
        print_error_stack_to_file(Conn->rError, stderr);
        rcDisconnect( Conn );
        exit( 7 );
    }

    /* get the new password */
#ifdef WIN32
    HANDLE hStdin = GetStdHandle( STD_INPUT_HANDLE );
    DWORD mode;
    DWORD lastMode = 0;
    BOOL error = false;
    int errsv = 0;

    if (echoFlag != 1)
    {
        GetConsoleMode( hStdin, &mode );
        lastMode = mode;
        mode &= ~ENABLE_ECHO_INPUT;
        error = !SetConsoleMode( hStdin, mode );
        errsv = -1;
    }
#else
    int error = 0;
    int errsv = 0;
    irods::termiosUtil tiosutl(STDIN_FILENO);
    if ( echoFlag != 1 && !tiosutl.echoOff() )
    {
        error = 1;
        errsv = tiosutl.getError();
    }
#endif

    if ( echoFlag != 1 && error )
    {
        printf( "WARNING: Error %d disabling echo mode. Password will be displayed in plaintext.\n", errsv );
    }

    len = 0;
    for (int ntimes = 0 ; len < 3 || len > MAX_PASSWORD_LEN - 8; ++ntimes) {
        if (ntimes == 3)
        {
#ifdef WIN32
            if ( echoFlag != 1 && !SetConsoleMode( hStdin, lastMode ) )
            {
                printf( "Error reinstating echo mode.\n" );
            }
#else
            if( echoFlag != 1 && tiosutl.getValid() && !tiosutl.echoOn() )
            {
                printf( "Error reinstating echo mode.\n" );
               }
#endif
            printf ("Invalid password. Aborting...\n");
            rcDisconnect( Conn );
            exit( 8 );
        }
        printf( "Enter your new iRODS password:" );
        std::string password = "";
        if (!getline( std::cin, password ))
        {
            printf( "End of file encountered.\n" );
#ifdef WIN32
            if ( echoFlag != 1 && !SetConsoleMode( hStdin, lastMode ) )
            {
                printf( "Error reinstating echo mode.\n" );
            }
#else
            if( echoFlag != 1 && tiosutl.getValid() && !tiosutl.echoOn() )
            {
                printf( "Error reinstating echo mode.\n" );
            }
#endif
            rcDisconnect( Conn );
            exit( 8 );
        }
        printf( "\n" );
        strncpy( newPw, password.c_str(), MAX_PASSWORD_LEN );
        len = strlen( newPw );
        if ( len < 3  || len > MAX_PASSWORD_LEN - 8 ) {
            printf( "Your password must be between 3 and 42 characters long.\n" );
        }
    }

    if ( myRodsArgs.force != True ) {
        printf( "Reenter your new iRODS password:" );
        std::string password = "";
        if (!getline( std::cin, password ))
        {
#ifdef WIN32
            if ( echoFlag != 1 && !SetConsoleMode( hStdin, lastMode ) )
            {
                printf( "Error reinstating echo mode.\n" );
            }
#else
            printf( "End of file encountered.\n" );
            if( echoFlag != 1 && tiosutl.getValid() && !tiosutl.echoOn() )
            {
                printf( "Error reinstating echo mode.\n" );
            }
#endif
            rcDisconnect( Conn );
            exit( 8 );
        }
        printf( "\n" );
        strncpy( newPw2, password.c_str(), MAX_PASSWORD_LEN );
        if ( strncmp( newPw, newPw2, MAX_PASSWORD_LEN ) != 0 ) {
            printf( "Entered passwords do not match\n" );

#ifdef WIN32
            if ( echoFlag != 1 && !SetConsoleMode( hStdin, lastMode ) )
            {
                printf( "Error reinstating echo mode.\n" );
            }
#else
            if( echoFlag != 1 && tiosutl.getValid() && !tiosutl.echoOn() )
            {
                printf( "Error reinstating echo mode.\n" );
            }
#endif
            rcDisconnect( Conn );
            exit( 8 );
        }
    }
#ifdef WIN32
    if ( echoFlag != 1 && SetConsoleMode( hStdin, lastMode ) ) {
        printf( "Error reinstating echo mode." );
    }
#else
    if( echoFlag != 1 && tiosutl.getValid() && !tiosutl.echoOn() )
    {
        printf( "Error reinstating echo mode.\n" );
    }
#endif

    userAdminInp.arg0 = "userpw";
    userAdminInp.arg1 = myEnv.rodsUserName;
    userAdminInp.arg2 = "password";

    if (no_scramble) {
        // The password should be provided in plaintext if --no-scramble is specified.
        userAdminInp.arg3 = newPw;
        userAdminInp.arg4 = "no-scramble";
    }
    else {
        strncpy(buf0, newPw, MAX_PASSWORD_LEN);
        len = strlen(newPw);
        lcopy = MAX_PASSWORD_LEN - 10 - len;
        if (lcopy > 15) {
            // this is a random string used to pad, arbitrary, but must match the server side:
            constexpr const char rand[] = "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs";

            // server will look for 15 characters of random string
            strncat(buf0, rand, lcopy);
        }
        i = obfGetPw(buf1);
        if (i != 0) {
            printf("Error getting current password\n");
            exit(1);
        }
        obfEncodeByKey(buf0, buf1, buf2);

        userAdminInp.arg3 = buf2;
        userAdminInp.arg4 = "";
    }

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
    else if (using_native) {
        /* initialize with the new password */
        i = obfSavePw( 0, 0, 0, newPw );
    }

    printErrorStack( Conn->rError );

    rcDisconnect( Conn );

    exit( 0 );
}


void usage( char *prog ) {
    printf("Usage: %s [-ehfvVl] [--no-scramble]\n\n", prog);
    printf("Changes your iRODS password.\n\n");
    printf("Options:\n");
    printf( " -v  verbose\n" );
    printf( " -V  Very verbose\n" );
    printf( " -l  long format (somewhat verbose)\n" );
    printf( " -e  echo the password as entered\n" );
    printf( " -f  force: do not ask user to reenter the new password\n" );
    printf(" --no-scramble\n");
    printf("     Indicate that the password should not be");
    printf("     scrambled before being sent to the server.\n");
    printf("     NOTE: The --no-scramble option causes the password to be\n");
    printf("     sent in plaintext. Please ensure TLS is in use when using\n");
    printf("     this option. Using no-scramble with any field other than\n");
    printf("     password has no effect. Using no-scramble with servers before\n");
    printf("     iRODS 5.1.0 is not allowed.\n");
    printf( " -h  this help\n" );
    printReleaseInfo( "ipasswd" );
}
