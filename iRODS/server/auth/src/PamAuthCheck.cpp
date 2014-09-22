/*
  This program does a PAM authentication check using the username on
  the command line and reading the password from stdin (to be more
  secure).  When PAM mode is enabled and users request LDAP/PAM
  authentication (irodsAuthScheme of 'PAM' or 'pam'), the irodsAgent
  spawns this process and writes the input password on stdin.  You can
  also run this manually, entering the password after PamAuthCheck is
  started; which will be echoed:
  $ ./PamAuthCheck testuser2
  asfkskdlfkd
  Authenticated
  $

  You may need to install PAM libraries, such as libpam0g-dev:
  sudo apt-get install libpam0g-dev

  This is built when PAM is enabled (in config/config.mk, change
  # PAM_AUTH = 1
  to
  PAM_AUTH = 1

  But you can also build it via:
  gcc PamAuthCheck.c -L /usr/lib -l pam -o PamAuthCheck

  It needs to be set UID root:
    sudo chown root PamAuthCheck
    sudo chmod u+s PamAuthCheck

  You may need to add the following (or equivalent) to the /etc/pam.conf file.
  # check authorization
  check   auth       required     pam_unix_auth.so
  check   account    required     pam_unix_acct.so

  Loosely based on various PAM examples.
*/

#include <security/pam_appl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <stdint.h>
#include <map>
#include <string>

namespace {
    const char pam_service[] = "irods";

    struct AppData {
        bool debug_mode;
        std::string password;
    };

    int
    null_conv( int num_msg, const struct pam_message **msg,
               struct pam_response **resp, void *appdata_ptr ) {

        const AppData &appdata = *static_cast<AppData*>( appdata_ptr );
        if ( appdata.debug_mode ) {
            std::map<int, const char*> pam_message_types;
            pam_message_types[PAM_PROMPT_ECHO_OFF] = "PAM_PROMPT_ECHO_OFF";
            pam_message_types[PAM_PROMPT_ECHO_ON] = "PAM_PROMPT_ECHO_ON";
            pam_message_types[PAM_ERROR_MSG] = "PAM_ERROR_MSG";
            pam_message_types[PAM_TEXT_INFO] = "PAM_TEXT_INFO";

            printf( "null_conv: num_msg: %d\n", num_msg );
            for ( int i = 0; i < num_msg; ++i ) {
                const int msg_style = msg[i]->msg_style;
                printf( "  null_conv: msg index: %d\n", i );
                printf( "    null_conv: msg_style: %d -> %s\n", msg_style, pam_message_types[msg_style] );
                printf( "    null_conv: msg: %s\n", msg[i]->msg );
            }
        }

        if ( num_msg < 1 ) {
            return PAM_SUCCESS;
        }

        *resp = static_cast<pam_response*>( malloc( sizeof( **resp ) ) );
        if ( *resp == NULL ) {
            fprintf( stderr, "null_conv: PamAuthCheck: malloc error\n" );
            return PAM_BUF_ERR;
        }

        ( *resp )->resp = strdup( appdata.password.c_str() );
        if ( ( *resp )->resp == NULL ) {
            free( *resp );
            fprintf( stderr, "PamAuthCheck: malloc error\n" );
            return PAM_BUF_ERR;
        }

        ( *resp )->resp_retcode = 0;
        return PAM_SUCCESS;
    }
}

int main( int argc, char *argv[] ) {

    const char *username = NULL;
    if ( argc == 2 || argc == 3 ) {
        username = argv[1];
    }
    else {
        fprintf( stderr, "Usage: PamAuthCheck username [extra-arg-activates-debug-mode]\n" );
        return 2;
    }

    AppData appdata;
    appdata.debug_mode = argc == 3;

    // read the password from stdin
    std::getline( std::cin, appdata.password );
    if ( appdata.debug_mode ) {
        printf( "password bytes: %ju\n", ( uintmax_t )appdata.password.size() );
    }

    pam_conv conv = { null_conv, &appdata };
    pam_handle_t *pamh = NULL;
    const int retval_pam_start = pam_start( pam_service, username, &conv, &pamh );
    if ( appdata.debug_mode ) {
        printf( "retval_pam_start: %d\n", retval_pam_start );
    }

    if ( retval_pam_start != PAM_SUCCESS ) {
        fprintf( stderr, "PamAuthCheck: pam_start error\n" );
        return 3;
    }

    // check username-password
    const int retval_pam_authenticate = pam_authenticate( pamh, 0 );
    if ( appdata.debug_mode ) {
        printf( "retval_pam_authenticate: %d\n", retval_pam_authenticate );
    }

    if ( retval_pam_authenticate == PAM_SUCCESS ) {
        fprintf( stdout, "Authenticated\n" );
    }
    else {
        fprintf( stdout, "Not Authenticated\n" );
    }

    // close Linux-PAM
    if ( pam_end( pamh, retval_pam_authenticate ) != PAM_SUCCESS ) {
        pamh = NULL;
        fprintf( stderr, "PamAuthCheck: failed to release authenticator\n" );
        return 4;
    }

    // indicate success (valid username and password) or not
    return retval_pam_authenticate == PAM_SUCCESS ? 0 : 1;
}
