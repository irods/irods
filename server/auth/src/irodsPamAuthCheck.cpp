/*
  This program performs a PAM authentication check, taking the username from
  the first command line argument and reading the password from stdin (for
  security). When PAM mode is enabled and users request LDAP/PAM
  authentication (irods_authentication_scheme of 'PAM' or 'pam'), the irodsAgent
  spawns this process.

  This program can be run manually, to directly check that PAM
  is configured correctly for iRODS.

  $ ./irodsPamAuthCheck username
  password
  Authenticated
  $

  You may need to install PAM libraries, such as libpam0g-dev:
  sudo apt-get install libpam0g-dev

  It needs to be set UID root:
    sudo chown root irodsPamAuthCheck
    sudo chmod u+s irodsPamAuthCheck

  You will need to create the file "/etc/pam.d/irods" and configure it
  according to your specific authentication requirements.

*/

#include <security/pam_appl.h>
#include <stdio.h>
#include <cstdlib>
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
            fprintf( stderr, "null_conv: irodsPamAuthCheck: malloc error\n" );
            return PAM_BUF_ERR;
        }

        ( *resp )->resp = strdup( appdata.password.c_str() );
        if ( ( *resp )->resp == NULL ) {
            free( *resp );
            fprintf( stderr, "irodsPamAuthCheck: malloc error\n" );
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
        fprintf( stderr, "Usage: irodsPamAuthCheck username [extra-arg-activates-debug-mode]\n" );
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
        fprintf( stderr, "irodsPamAuthCheck: pam_start error\n" );
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
        fprintf( stderr, "irodsPamAuthCheck: failed to release authenticator\n" );
        return 4;
    }

    // indicate success (valid username and password) or not
    return retval_pam_authenticate == PAM_SUCCESS ? 0 : 1;
}
