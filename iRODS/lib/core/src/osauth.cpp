/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  These routines are used to implement the OS level authentication scheme.


  Returns codes of 0 indicate success, others are iRODS error codes.
*/

#include <stdlib.h>
#include <stdio.h>
#include <limits>
#ifndef windows_platform
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#endif

#include "rods.hpp"
#include "rcGlobalExtern.hpp"
#include "authenticate.hpp"
extern "C" {

    int
    osauthVerifyResponse( char *challenge, char *username, char *response ) {
#if defined(OS_AUTH)
        static char fname[] = "osauthVerifyResponse";
        char authenticator[16]; /* MD5 hash */
        char md5buffer[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
        char md5digest[RESPONSE_LEN + 2];
        int uid, status, i;
        char *keybuf = NULL;
        int key_len;
        MD5_CTX ctx;

        uid = osauthGetUid( username );
        if ( uid == -1 ) {
            return SYS_USER_RETRIEVE_ERR;
        }

        /* read the key from the key file */
        if ( ( status = osauthGetKey( &keybuf, &key_len ) ) ) {
            rodsLogError( LOG_ERROR, status,
                          "%s: error retrieving key.", fname );
            return status;
        }

        /* generate the authenticator */
        status = osauthGenerateAuthenticator( username, uid, challenge,
                                              keybuf, key_len,
                                              authenticator, 16 );
        if ( status ) {
            rodsLog( LOG_ERROR,
                     "%s: could not generate the authenticator", fname );
            free( keybuf );
            return status;
        }
        free( keybuf );

        /* now append this hash with the challenge, and
           take the md5 sum of that */
        memset( md5buffer, 0, sizeof( md5buffer ) );
        memset( md5digest, 0, sizeof( md5digest ) );
        memcpy( md5buffer, challenge, CHALLENGE_LEN );
        memcpy( md5buffer + CHALLENGE_LEN, authenticator, 16 );
        MD5Init( &ctx );
        MD5Update( &ctx, ( unsigned char* )md5buffer, CHALLENGE_LEN + MAX_PASSWORD_LEN );
        MD5Final( ( unsigned char* )md5digest, &ctx );
        for ( i = 0; i < RESPONSE_LEN; i++ ) {
            /* make sure 'string' doesn't end early
               (this matches client digest generation). */
            if ( md5digest[i] == '\0' ) {
                md5digest[i]++;
            }
        }

        /* compare the calculated digest to the client response */
        for ( i = 0; i < RESPONSE_LEN; i++ ) {
            if ( response[i] != md5digest[i] ) {
                rodsLog( LOG_ERROR, "%s: calculated digest doesn't match client response",
                         fname );
                return CAT_INVALID_AUTHENTICATION;
            }
        }

        return 0;

#else /* defined OS_AUTH */
        if ( ProcessType == CLIENT_PT ) {
            return OSAUTH_NOT_BUILT_INTO_CLIENT;
        }
        else {
            return OSAUTH_NOT_BUILT_INTO_SERVER;
        }
#endif
    }

    /*
     * osauthGenerateAuthenticator - this functions creates
     * an authenticator from the passed in parameters. The
     * parameters are concatenated together, and then an
     * md5 hash is generated and provided in the output
     * parameter. Returns 0 on success, error code on error.
     */
    int
    osauthGenerateAuthenticator( char *username, int uid,
                                 char *challenge, char *key, int key_len,
                                 char *authenticator, int authenticator_len ) {
#if defined(OS_AUTH)
        static char fname[] = "osauthGenerateAuthenticator";
        char *buffer, *bufp;
        int buflen;
        char md5digest[16];
        MD5_CTX ctx;

        if ( authenticator == NULL ||
                authenticator_len < 16 ) {
            return USER_INPUT_OPTION_ERR;
        }

        /* calculate buffer size and allocate */
        buflen = username ? strlen( username ) : 0;
        buflen += sizeof( uid ) + CHALLENGE_LEN + key_len;
        buffer = ( char* )malloc( buflen );
        if ( buffer == NULL ) {
            rodsLog( LOG_ERROR,
                     "%s: could not allocate memory buffer. errno = %d",
                     fname, errno );
            return SYS_MALLOC_ERR;
        }

        /* concatenate the input parameters */
        bufp = buffer;
        if ( username && strlen( username ) ) {
            memcpy( bufp, username, strlen( username ) );
            bufp += strlen( username );
        }
#if defined(OS_AUTH_NO_UID)
        uid = 0;
#endif
        memcpy( bufp, &uid, sizeof( uid ) );
        bufp += sizeof( uid );
        if ( challenge ) {
            memcpy( bufp, challenge, CHALLENGE_LEN );
            bufp += CHALLENGE_LEN;
        }
        if ( key ) {
            memcpy( bufp, key, key_len );
        }

        /* generate an MD5 hash of the buffer, and copy it
           to the output parameter authenticator */
        MD5Init( &ctx );
        MD5Update( &ctx, ( unsigned char* )buffer, buflen );
        MD5Final( ( unsigned char* )md5digest, &ctx );
        memcpy( authenticator, md5digest, 16 );

        free( buffer );
        return 0;

#else /* defined OS_AUTH */
        if ( ProcessType == CLIENT_PT ) {
            return OSAUTH_NOT_BUILT_INTO_CLIENT;
        }
        else {
            return OSAUTH_NOT_BUILT_INTO_SERVER;
        }
#endif
    }

    /*
     * osauthGetKey - read key from OS_AUTH_KEYFILE
     *
     * The return parameters include a malloc'ed buffer in
     * *key that is the responsibility of the caller to free.
     * The length of the buffer is in *key_len.
     */
    int
    osauthGetKey( char **key, int *key_len ) {
#if defined(OS_AUTH)
        static char fname[] = "osauthGetKey";
        char *keyfile;
        int buflen, key_fd, nb;

        if ( key == NULL || key_len == NULL ) {
            return USER__NULL_INPUT_ERR;
        }

        keyfile = getenv( "irodsOsAuthKeyfile" );
        if ( keyfile == NULL || *keyfile == '\0' ) {
            keyfile = OS_AUTH_KEYFILE;
        }

        key_fd = open( keyfile, O_RDONLY, 0 );
        if ( key_fd < 0 ) {
            rodsLog( LOG_ERROR,
                     "%s: couldn't open %s for reading. errno = %d",
                     fname, keyfile, errno );
            return FILE_OPEN_ERR;
        }
        off_t lseek_return = lseek( key_fd, 0, SEEK_END );
        int errsv = errno;
        if ( ( off_t ) - 1 == lseek_return ) {
            fprintf( stderr, "SEEK_END lseek failed with error %d.\n", errsv );
            close( key_fd );
            return UNIX_FILE_LSEEK_ERR;
        }
        if ( lseek_return > std::numeric_limits<long long>::max() ) {
            fprintf( stderr, "file of size %ju is too large for a long long.\n", ( uintmax_t )lseek_return );
            close( key_fd );
            return UNIX_FILE_LSEEK_ERR;
        }
        buflen = lseek_return;
        lseek_return = lseek( key_fd, 0, SEEK_SET );
        errsv = errno;
        if ( ( off_t ) - 1 == lseek_return ) {
            fprintf( stderr, "SEEK_SET lseek failed with error %d.\n", errsv );
            close( key_fd );
            return UNIX_FILE_LSEEK_ERR;
        }

        char * keybuf = ( char* )malloc( buflen );
        if ( keybuf == NULL ) {
            rodsLog( LOG_ERROR,
                     "%s: could not allocate memory for key buffer. errno = %d",
                     fname, errno );
            close( key_fd );
            return SYS_MALLOC_ERR;
        }
        nb = read( key_fd, keybuf, buflen );
        close( key_fd );
        if ( nb < 0 ) {
            rodsLog( LOG_ERROR,
                     "%s: couldn't read key from %s. errno = %d",
                     fname, keyfile, errno );
            free( keybuf );
            return FILE_READ_ERR;
        }

        *key_len = buflen;
        *key = keybuf;

        return 0;
#else /* defined OS_AUTH */
        if ( ProcessType == CLIENT_PT ) {
            return OSAUTH_NOT_BUILT_INTO_CLIENT;
        }
        else {
            return OSAUTH_NOT_BUILT_INTO_SERVER;
        }
#endif
    }

    /*
     * osauthGetAuth - this function runs the OS_AUTH_CMD command to
     * retrieve an authenticator for the calling user.
     */
    int
    osauthGetAuth( char *challenge,
                   char *username,
                   char *authenticator,
                   int authenticator_buflen ) {
#if defined(OS_AUTH)
        static char fname[] = "osauthGetAuth";
        int pipe1[2], pipe2[2];
        pid_t childPid;
        int childStatus = 0;
        int child_stdin, child_stdout, nb;
        int buflen, challenge_len = CHALLENGE_LEN;
        char buffer[128];

        if ( challenge == NULL || username == NULL || authenticator == NULL ) {
            return USER__NULL_INPUT_ERR;
        }

        if ( pipe( pipe1 ) < 0 ) {
            rodsLog( LOG_ERROR, "%s: pipe1 create failed. errno = %d",
                     fname, errno );
            return SYS_PIPE_ERROR - errno;
        }
        if ( pipe( pipe2 ) < 0 ) {
            rodsLog( LOG_ERROR, "%s: pipe2 create failed. errno = %d",
                     fname, errno );
            close( pipe1[0] );
            close( pipe1[1] );
            return SYS_PIPE_ERROR - errno;
        }

        childPid = RODS_FORK();

        if ( childPid < 0 ) {
            rodsLog( LOG_ERROR, "%s: RODS_FORK failed. errno = %d",
                     fname, errno );
            close( pipe1[0] );
            close( pipe1[1] );
            close( pipe2[0] );
            close( pipe2[1] );
            return SYS_FORK_ERROR;
        }
        else if ( childPid == 0 ) {
            /* in the child process */

            /* pipe1 will be for child's stdin */
            close( pipe1[1] );
            dup2( pipe1[0], 0 );
            /* pipe2 will be for child's stdout */
            close( pipe2[0] );
            dup2( pipe2[1], 1 );

            /* set the username in an environment variable */
            setenv( OS_AUTH_ENV_USER, username, 1 );

            /* run the OS_AUTH_CMD */
            execlp( OS_AUTH_CMD, OS_AUTH_CMD, ( char* )NULL );
            rodsLog( LOG_ERROR, "%s: child execl %s failed. errno = %d",
                     fname, OS_AUTH_CMD, errno );
        }
        else {
            /* in the parent process */
            close( pipe1[0] );
            child_stdin = pipe1[1];  /* parent writes to child's stdin */
            close( pipe2[1] );
            child_stdout = pipe2[0]; /* parent reads from child's stdout */

            /* send the challenge to the OS_AUTH_CMD program on its stdin */
            nb = write( child_stdin, &challenge_len, sizeof( challenge_len ) );
            if ( nb < 0 ) {
                rodsLog( LOG_ERROR,
                         "%s: error writing challenge_len to %s. errno = %d",
                         fname, OS_AUTH_CMD, errno );
                close( child_stdin );
                close( child_stdout );
                return SYS_PIPE_ERROR - errno;
            }
            nb = write( child_stdin, challenge, challenge_len );
            if ( nb < 0 ) {
                rodsLog( LOG_ERROR,
                         "%s: error writing challenge to %s. errno = %d",
                         fname, OS_AUTH_CMD, errno );
                close( child_stdin );
                close( child_stdout );
                return SYS_PIPE_ERROR - errno;
            }

            /* read the response */
            buflen = read( child_stdout, buffer, 128 );
            if ( buflen < 0 ) {
                rodsLog( LOG_ERROR, "%s: error reading from %s. errno = %d",
                         fname, OS_AUTH_CMD, errno );
                close( child_stdin );
                close( child_stdout );
                return SYS_PIPE_ERROR - errno;
            }

            close( child_stdin );
            close( child_stdout );

            if ( waitpid( childPid, &childStatus, 0 ) < 0 ) {
                rodsLog( LOG_ERROR, "%s: waitpid error. errno = %d",
                         fname, errno );
                return EXEC_CMD_ERROR;
            }

            if ( WIFEXITED( childStatus ) ) {
                if ( WEXITSTATUS( childStatus ) ) {
                    rodsLog( LOG_ERROR,
                             "%s: command failed: %s", fname, buffer );
                    return EXEC_CMD_ERROR;
                }
            }
            else {
                rodsLog( LOG_ERROR,
                         "%s: some error running %s", fname, OS_AUTH_CMD );
            }

            /* authenticator is in buffer now */
            if ( buflen > authenticator_buflen ) {
                rodsLog( LOG_ERROR,
                         "%s: not enough space in return buffer for authenticator", fname );
                return EXEC_CMD_OUTPUT_TOO_LARGE;
            }
            memcpy( authenticator, buffer, buflen );
        }

        return 0;

#else /* defined OS_AUTH */
        if ( ProcessType == CLIENT_PT ) {
            return OSAUTH_NOT_BUILT_INTO_CLIENT;
        }
        else {
            return OSAUTH_NOT_BUILT_INTO_SERVER;
        }
#endif
    }

    /*
     * osauthGetUid - looks up the given username using getpwnam
     * and returns the user's uid if successful, or -1 if not.
     */
    int
    osauthGetUid( char *username ) {
        if ( NULL == username ) {
            rodsLog( LOG_ERROR, "Error: osauthGetUid called with null username argument." );
            return -1;
        }
        static char fname[] = "osauthGetUid";

        errno = 0;
        struct passwd *pwent = getpwnam( username );
        if ( pwent == NULL ) {
            if ( errno ) {
                rodsLog( LOG_ERROR,
                         "%s: error calling getpwnam for %s. errno = %d",
                         fname, username, errno );
                return -1;
            }
            else {
                rodsLog( LOG_ERROR,
                         "%s: no such user %s", fname, username );
                return -1;
            }
        }
        return pwent->pw_uid;
    }

    /*
     * osauthGetUsername - looks up the user identified by
     * the uid determined by getuid(). Places the username
     * in the provided username buffer and returns the uid
     * on success, or -1 if not.
     */
    int
    osauthGetUsername( char *username, int username_len ) {
        static char fname[] = "osauthGetUsername";
        struct passwd *pwent;
        int uid;

        uid = getuid();
        errno = 0;
        pwent = getpwuid( uid );
        if ( pwent == NULL ) {
            if ( errno ) {
                rodsLog( LOG_ERROR,
                         "%s: error calling getpwuid for uid %d. errno = %d",
                         fname, uid, errno );
                return -1;
            }
            else {
                rodsLog( LOG_ERROR, "%s: no user with uid %d",
                         fname, uid );
                return -1;
            }
        }
        if ( ( unsigned int )username_len <= strlen( pwent->pw_name ) ) {
            rodsLog( LOG_ERROR, "%s: username input buffer too small (%d <= %d)",
                     fname, username_len, strlen( pwent->pw_name ) );
            return -1;
        }
        strcpy( username, pwent->pw_name );

        return uid;
    }

} // extern "C"
