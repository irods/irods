#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

#include "authenticate.h"
#include "osauth.h"


int
main( int argc, char *argv[] ) {

    /* make sure the provided username matches what the OS thinks */
    char * username = getenv( OS_AUTH_ENV_USER );
    if ( username == NULL ) {
        if ( argc > 0 ) {
            /* probably means someone has run from command-line */
            printf( "%s is run through an iRODS library call, and probably won't do anything useful for you.\n",
                    argv[0] );
            printf( "%s exiting.\n", argv[0] );
            return 1;
        }
        printf( "Username is null" );
        return 1;
    }
    char username2[NAME_LEN];
    int uid = osauthGetUsername( username2, NAME_LEN );
    if ( uid == -1 ) {
        return 1;
    }
    if ( strcmp( username, username2 ) ) {
        printf( "Username %s does not match OS user %s",
                username, username2 );
        return 1;
    }

    /* read the challenge from stdin */
    int challenge_len;
    if ( sizeof( challenge_len ) != read( 0, ( void* )&challenge_len, sizeof( challenge_len ) ) ) {
        printf( "Couldn't read challenge length from stdin: %s",
                strerror( errno ) );
        return 1;
    }
    if ( challenge_len != CHALLENGE_LEN ) {
        printf( "Challenge length must be %ju", ( uintmax_t )CHALLENGE_LEN );
        return 1;
    }
    std::vector<char> challenge( CHALLENGE_LEN );
    if ( CHALLENGE_LEN != read( 0, &challenge[0], CHALLENGE_LEN ) ) {
        printf( "Couldn't read challenge from stdin: %s",
                strerror( errno ) );
        return 1;
    }

    /* read the key from the key file */
    char * keybuf = NULL;
    int key_len;
    if ( osauthGetKey( &keybuf, &key_len ) ) {
        printf( "Error retrieving key. Exiting." );
        return 1;
    }

    char authenticator[16];  /* hard coded at 16 bytes .. size of an md5 hash */
    if ( osauthGenerateAuthenticator( username, uid, &challenge[0], keybuf, key_len, authenticator, 16 ) ) {
        printf( "Could not generate the authenticator" );
        return 1;
    }

    /* write out the authenticator to stdout */
    if ( write( 1, authenticator, 16 ) == -1 ) {
        int errsv = errno;
        printf( "Error %s writing the authenticator to stdout.",
                strerror( errsv ) );
        return 1;
    }

    return 0;
}

