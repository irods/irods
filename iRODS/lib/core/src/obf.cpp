/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  These routines obfuscate (scramble or sort-of encrypt) passwords.

  These are used to avoid storing plain-text passwords in the files,
  yet still allow automatic authentication.  This is better than
  plain-text passwords sitting around in files and often being
  transmitted on networks in NFS packets.  But, of course, this isn't
  highly secure, particularly since with this source file one can
  defeat it.  But it does stop attackers from finding passwords by
  just searching directories.

  Externally callable routines begin with "obf" and internal routines
  begin with "obfi".

  A timestamp is used with this, which will be gotten from the modify
  time of the authentication file.  This way, a simple copy of the
  file will not successfully authenticate.

  The UID of the running process is also used in the algorithm.

  The same password is obfuscated differently (usually), each time the
  obfuscation is done, so that one will usually be unable to tell by
  looking at the ciphertext whether two passwords are the same or not.
  Also, this makes it more difficult to determine the obfuscation
  algorithm.

  When the iRODS commands authenticate, they first check if this is
  available, and use that if possible.  Otherwise, they prompt for the
  password.

  Returns codes of 0 indicate success, others are iRODS error codes.
*/


#include <stdio.h>
#ifndef windows_platform
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif
#include <stdlib.h>

#include "rods.hpp"
#include "rodsPath.hpp"
#include "authenticate.hpp"

#ifdef _WIN32
#include "Unix2Nt.hpp"  /* May need something like this for Windows */
#include "iRODSNtUtil.hpp"    /* May need something like this for Windows */
#include "rcGlobalExtern.hpp"
#endif

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#define TMP_FLAG "%TEMPORARY_PW%"

#define AUTH_FILENAME_DEFAULT ".irods/.irodsA" /* under the HOME dir */

int obfDebug = 0;
int timeVal = 0;
int isTemp = 0;
int doTemp = 0;

int cipherBlockChaining = 0;

/* For now, edit this to change the default hash type, types are
   HASH_TYPE_MD5 and HASH_TYPE_SHA1 */
static int defaultHashType = HASH_TYPE_MD5;

/*
  What can be a main routine for some simple tests.
 */
int
obftestmain( int argc, char *argv[] ) {
    char p3[MAX_PASSWORD_LEN];
    int i;

    obfDebug = 2;

    if ( argc < 2 ) {
        printf( "Usage: -d|-e\n" );
        exit( -1 );
    }

    if ( strcmp( argv[1], "-d" ) == 0 ) {
        i = obfGetPw( p3 );
        if ( obfDebug ) {
            printf( "val  = %d \n", i );
        }
    }

    if ( strcmp( argv[1], "-e" ) == 0 ) {
        i = obfSavePw( 1, 0, 1, "" );
        if ( obfDebug ) {
            printf( "val  = %d \n", i );
        }
    }
    return 0;
}

int
obfSetDebug( int opt ) {
    obfDebug = opt;
    return 0;
}

int
obfiGetFilename( char *fileName ) {
    char *envVar = NULL;

    envVar = getRodsEnvAuthFileName();
    if ( envVar != NULL && *envVar != '\0' ) {
        strcpy( fileName, envVar );
        return 0;
    }

#ifdef windows_platform
    if ( ProcessType != CLIENT_PT ) {
        char *tmpstr1;
        int t;
        tmpstr1 = iRODSNtGetServerConfigPath();
        sprintf( fileName, "%s\\irodsA.txt", tmpstr1 );
        return 0;
    }

    /* for clients */
    envVar = iRODSNt_gethome();
#else
    envVar =  getenv( "HOME" );
#endif
    if ( envVar == NULL ) {
        return ENVIRONMENT_VAR_HOME_NOT_DEFINED;
    }
    strncpy( fileName, envVar, MAX_NAME_LEN );
    strncat( fileName, "/", MAX_NAME_LEN );
    strncat( fileName, AUTH_FILENAME_DEFAULT, MAX_NAME_LEN );

#ifdef windows_platform
    if ( envVar != NULL ) {
        free( envVar );
    }
#endif

    return 0;
}

int
obfGetPw( char *pw ) {
    char myPw[MAX_PASSWORD_LEN + 10];
    char myPwD[MAX_PASSWORD_LEN + 10];
    char fileName[MAX_NAME_LEN + 10];
    char *cp;

    int i;
    int envVal;

    strcpy( pw, "" );

    envVal = obfiGetEnvKey();

    i = obfiGetFilename( fileName );
    if ( i < 0 ) {
        return i;
    }

    i = obfiGetTv( fileName );
    if ( i < 0 ) {
        return i;
    }

    i = obfiGetPw( fileName, myPw );
    if ( i < 0 ) {
        return i;
    }

    i = obfiDecode( myPw, myPwD, envVal );
    if ( i < 0 ) {
        return i;
    }

    isTemp = 0;
    cp = strstr( myPwD, TMP_FLAG );
    if ( cp != 0 ) {
        isTemp = 1;
        *cp = '\0';
    }

    if ( obfDebug ) {
        printf( "out:%s\n", myPwD );
    }
    strcpy( pw, myPwD );

    return 0;
}

/*
 remove the password file
 opt: if non-zero, don't ask and don't print error;just remove it if it exists.
*/
int
obfRmPw( int opt ) {
    char fileName[MAX_NAME_LEN + 10];
    char inbuf[MAX_NAME_LEN + 10];

    if ( int status = obfiGetFilename( fileName ) ) {
        return status;
    }
    boost::filesystem::path filePath( fileName );
    if ( !boost::filesystem::exists( filePath ) ) {
        if ( opt == 0 ) {
            printf( "%s does not exist\n", fileName );
        }
        return AUTH_FILE_DOES_NOT_EXIST;
    }
    if ( opt == 0 ) {
        printf( "Remove %s?:", fileName );
        const char *fgets_ret = fgets( inbuf, MAX_NAME_LEN, stdin );
        if ( fgets_ret == NULL || strlen( inbuf ) < 1 || inbuf[0] != 'y' ) {
            return 0;
        }
    }
    boost::system::error_code error;
    boost::filesystem::remove( filePath, error );
    if ( error.value() ) {
        return UNLINK_FAILED;
    }
    return 0;
}

/* Set timeVal from a fstat of the file after writing to it.
   Could just use system time, but if it's way off from the file
   system (NFS on a VM host, for example), it would fail. */
int
obfiSetTimeFromFile( int fd ) {
#ifndef windows_platform
    struct stat statBuf;
#else
    struct irodsntstat statBuf;
#endif
    int wval, fval, lval;
    wval = write( fd, " ", 1 );
    if ( wval != 1 ) {
        return FILE_WRITE_ERR;
    }
#ifndef windows_platform
    fval = fstat( fd, &statBuf );
#else
    fval = _fstat64( fd, &statBuf );
#endif
    if ( fval < 0 ) {
        timeVal = 0;
        return UNABLE_TO_STAT_FILE;
    }
    lval = lseek( fd, 0, SEEK_SET );
    if ( lval < 0 ) {
        return UNABLE_TO_STAT_FILE;
    }

    timeVal = statBuf.st_mtime & 0xffff;   /* keep it bounded */
    return 0;
}



/* int promptOpt; if 1, echo password as typed, else no echo */
/* int fileOpt; if 1, ask permission before writing file */
/* int printOpt; if 1, display an updated file msg on success */
/* char *pwArg; if non-0-length, this is the new password */
int
obfSavePw( int promptOpt, int fileOpt, int printOpt, const char *pwArg ) {
    using namespace boost::filesystem;
    char fileName[MAX_NAME_LEN + 10];
    char inbuf[MAX_PASSWORD_LEN + 100];
    char myPw[MAX_PASSWORD_LEN + 10];
    int i = 0, fd = 0, envVal = 0;

    i = obfiGetFilename( fileName );
    if ( i != 0 ) {
        return i;
    }

    envVal = obfiGetEnvKey();

    if ( strlen( pwArg ) == 0 ) {

#ifdef windows_platform
        iRODSNtGetUserPasswdInputInConsole( inbuf, "Enter your current iRODS password:", promptOpt );
#else
        if ( promptOpt != 1 ) {
            path p( "/bin/stty" );
            if ( exists( p ) ) {
                if ( 0 != system( "/bin/stty -echo" ) ) {
                    // TODO: revisit this condition
                }
            }
        }

        printf( "Enter your current iRODS password:" );
        if ( NULL == fgets( inbuf, MAX_PASSWORD_LEN + 50, stdin ) ) {
            // end of line reached or no input
        }

        if ( promptOpt != 1 ) {
            if ( 0 != system( "/bin/stty echo" ) ) {
                // TODO: revisit this condition
            }
            printf( "\n" );
        }
#endif

    }
    else {
        strncpy( inbuf, pwArg, MAX_PASSWORD_LEN );
    }
    i = strlen( inbuf );
    if ( i < 1 ) {
        return NO_PASSWORD_ENTERED;
    }
    if ( strlen( inbuf ) > MAX_PASSWORD_LEN - 2 ) {
        return PASSWORD_EXCEEDS_MAX_SIZE;
    }
    if ( inbuf[i - 1] == '\n' ) {
        inbuf[i - 1] = '\0';    /* remove trailing \n */
    }

    if ( doTemp ) {
        strcat( inbuf, TMP_FLAG );
    }

    fd = obfiOpenOutFile( fileName, fileOpt );
    if ( fd < 0 ) {
        return FILE_OPEN_ERR;
    }

    if ( fd == 0 ) {
        return ( 0 );   /* user canceled */
    }

    i = obfiSetTimeFromFile( fd );
    if ( i < 0 ) {
        close( fd );
        return i;
    }

    obfiEncode( inbuf, myPw, envVal );
    if ( obfDebug > 1 ) {
        printf( " in:%s out:%s\n", inbuf, myPw );
    }

    i = obfiWritePw( fd, myPw );
    close( fd );
    if ( i < 0 ) {
        return i;
    }

    if ( obfDebug || printOpt ) {
        printf( "Successfully wrote %s\n", fileName );
    }

    return 0;
}

/* various options for temporary auth files/pws */
int obfTempOps( int tmpOpt ) {
    char fileName[MAX_NAME_LEN + 10];
    char pw[MAX_PASSWORD_LEN + 10];
    int i;
    if ( tmpOpt == 1 ) { /* store pw flagged as temporary */
        doTemp = 1;
    }

    if ( tmpOpt == 2 ) { /* remove the pw file if temporary */
        i = obfGetPw( pw );
        strcpy( pw, "           " );
        if ( i != 0 ) {
            return i;
        }
        if ( isTemp ) {
            i = obfiGetFilename( fileName );
            if ( i != 0 ) {
                return i;
            }
            unlink( fileName );
        }
    }
    return 0;
}

int
obfiGetTv( char *fileName ) {
#ifndef windows_platform
    struct stat statBuf;
#else
    struct irodsntstat statBuf;
#endif
    int fval;

#ifndef windows_platform
    fval = stat( fileName, &statBuf );
#else
    fval = iRODSNt_stat( fileName, &statBuf );
#endif

    if ( fval < 0 ) {
        timeVal = 0;
        return UNABLE_TO_STAT_FILE;
    }

    timeVal = statBuf.st_mtime & 0xffff;   /* keep it bounded */
    return 0;
}

int
obfiGetPw( const char *fileName, char *pw ) {
#ifdef windows_platform
    int fd_in = iRODSNt_open( fileName, O_RDONLY, 1 );
#else
    int fd_in = open( fileName, O_RDONLY, 0 );
#endif

    if ( fd_in < 0 ) {
        return FILE_OPEN_ERR;
    }

    char buf[MAX_PASSWORD_LEN + 1];
    int rval = read( fd_in, buf, MAX_PASSWORD_LEN );
    close( fd_in );

    if ( rval < 0 ) {
        return FILE_READ_ERR;
    }
    buf[rval] = '\0';
    if ( strlen( buf ) >= MAX_PASSWORD_LEN ) {
        return PASSWORD_EXCEEDS_MAX_SIZE;
    }

    snprintf( pw, MAX_PASSWORD_LEN, "%s", buf );
    return 0;
}

int
obfiOpenOutFile( const char *fileName, int fileOpt ) {
    char inbuf[MAX_NAME_LEN] = "";
    int i = 0, fd_out = 0;

#ifdef _WIN32
    fd_out = iRODSNt_open( fileName, O_CREAT | O_WRONLY | O_EXCL, 1 );
#else
    fd_out = open( fileName, O_CREAT | O_WRONLY | O_EXCL, 0600 );
#endif

    if ( fd_out < 0 ) {
        if ( errno != EEXIST ) {
            return FILE_OPEN_ERR;
        }
        if ( fileOpt > 0 ) {
            printf( "Overwrite '%s'?:", fileName );
            if ( NULL == fgets( inbuf, MAX_NAME_LEN, stdin ) ) {
                // end of line reached or no input
            }
            i = strlen( inbuf );
            if ( i < 2 ) {
                return 0;
            }
        }
        else {
            strcpy( inbuf, "y" );
        }
        if ( inbuf[0] == 'y' ) {
#ifdef windows_platform
            fd_out = iRODSNt_open( fileName, O_CREAT | O_WRONLY | O_TRUNC, 1 );
#else
            fd_out = open( fileName, O_CREAT | O_WRONLY | O_TRUNC, 0600 );
#endif
            if ( fd_out < 0 ) {
                return FILE_OPEN_ERR;
            }
        }
        else {
            return 0;
        }
    }
    return fd_out;
}

int
obfiWritePw( int fd, const char *pw ) {
    int wval, len;
    len = strlen( pw );
    wval = write( fd, pw, len + 1 );
    if ( wval != len + 1 ) {
        return FILE_WRITE_ERR;
    }
    return 0;
}

int obfiTimeval() {
    long sec;
    int val;

    struct timeval nowtime;

    ( void )gettimeofday( &nowtime, ( struct timezone * )0 );
    sec = nowtime.tv_sec;

    val = sec;
    val = val & 0xffff;      /* keep it bounded */
    if ( obfDebug > 1 ) {
        printf( "val  = %d %x\n", val, val );
    }
    return val;
}


/*
  Obfuscate a password
*/
void
obfiEncode( const char *in, char *out, int extra ) {
    int i;
    long seq;
    const char *my_in;

    /*   struct timeb timeb_time; */
    struct timeval nowtime;

    int rval;
    int wheel_len;
    int wheel[26 + 26 + 10 + 15];
    int j, addin, addin_i, found;
    int ii;
    int now;
    char headstring[10];
#ifndef _WIN32
    int uid;
#endif
    /*
     Set up an array of characters that we will transpose.
    */

    wheel_len = 26 + 26 + 10 + 15;
    j = 0;
    for ( i = 0; i < 10; i++ ) {
        wheel[j++] = ( int )'0' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'A' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'a' + i;
    }
    for ( i = 0; i < 15; i++ ) {
        wheel[j++] = ( int )'!' + i;
    }

    /*
     get uid to use as part of the key
    */
#ifndef _WIN32
    uid = getuid();
    uid = uid & 0xf5f; /* keep it fairly small and not exactly uid */
#endif

    /*
     get a pseudo random number
    */

    ( void )gettimeofday( &nowtime, ( struct timezone * )0 );
    rval = nowtime.tv_usec & 0xf;

    /*
     and use it to pick a pattern for ascii offsets
    */
    seq = 0;
    if ( rval == 0 ) {
        seq = 0xd768b678;
    }
    if ( rval == 1 ) {
        seq = 0xedfdaf56;
    }
    if ( rval == 2 ) {
        seq = 0x2420231b;
    }
    if ( rval == 3 ) {
        seq = 0x987098d8;
    }
    if ( rval == 4 ) {
        seq = 0xc1bdfeee;
    }
    if ( rval == 5 ) {
        seq = 0xf572341f;
    }
    if ( rval == 6 ) {
        seq = 0x478def3a;
    }
    if ( rval == 7 ) {
        seq = 0xa830d343;
    }
    if ( rval == 8 ) {
        seq = 0x774dfa2a;
    }
    if ( rval == 9 ) {
        seq = 0x6720731e;
    }
    if ( rval == 10 ) {
        seq = 0x346fa320;
    }
    if ( rval == 11 ) {
        seq = 0x6ffdf43a;
    }
    if ( rval == 12 ) {
        seq = 0x7723a320;
    }
    if ( rval == 13 ) {
        seq = 0xdf67d02e;
    }
    if ( rval == 14 ) {
        seq = 0x86ad240a;
    }
    if ( rval == 15 ) {
        seq = 0xe76d342e;
    }

    /*
     get the timestamp and other id
    */
    if ( timeVal != 0 ) {
        now = timeVal; /* from file, normally */
    }
    else {
        now = obfiTimeval();
    }
    headstring[1] = ( ( now >> 4 ) & 0xf ) + 'a';
    headstring[2] = ( now & 0xf ) + 'a';
    headstring[3] = ( ( now >> 12 ) & 0xf ) + 'a';
    headstring[4] = ( ( now >> 8 ) & 0xf ) + 'a';
    headstring[5] = '\0';
    headstring[0] = 'S' - ( ( rval & 0x7 ) * 2 ); /* another check value */

    *out++ = '.';          /* store our initial, human-readable identifier */

    addin_i = 0;
    my_in = headstring;    /* start with head string */
    for ( ii = 0;; ) {
        ii++;
        if ( ii == 6 ) {
            *out++ = rval + 'e';  /* store the key */
            my_in = in;   /* now start using the input string */
        }
        found = 0;
        addin = ( seq >> addin_i ) & 0x1f;
        addin += extra;
#ifndef _WIN32
        addin += uid;
        addin_i += 3;
#else
        addin_i += 2;
#endif
        if ( addin_i > 28 ) {
            addin_i = 0;
        }
        for ( i = 0; i < wheel_len; i++ ) {
            if ( *my_in == ( char )wheel[i] ) {
                j = i + addin;
                if ( obfDebug > 1 ) {
                    printf( "j1=%d ", j );
                }
                j = j % wheel_len;
                if ( obfDebug > 1 ) {
                    printf( "j2=%d \n", j );
                }
                *out++ = ( char )wheel[j];
                found = 1;
                break;
            }
        }
        if ( found == 0 ) {
            if ( *my_in == '\0' ) {
                *out++ = '\0';
                return;
            }
            else {
                *out++ = *my_in;
            }
        }
        my_in++;
    }
}


int
obfiTimeCheck( int time1, int time2 ) {
    int fudge = 20;
    int delta;

    /*  printf("time1=%d time2=%d\n",time1, time2); */

    delta = time1 - time2;
    if ( delta < 0 ) {
        delta = 0 - delta;
    }
    if ( delta < fudge ) {
        return 0;
    }

    if ( time1 < 65000 ) {
        time1 += 65535;
    }
    if ( time2 < 65000 ) {
        time2 += 65535;
    }

    delta = time1 - time2;
    if ( delta < 0 ) {
        delta = 0 - delta;
    }
    if ( delta < fudge ) {
        return 0;
    }

    return 1;
}



int
obfiDecode( const char *in, char *out, int extra ) {
    int i;
    long seq;
    const char *p1;

    int rval;
    int wheel_len;
    int wheel[26 + 26 + 10 + 15];
    int j, addin, addin_i, kpos, found, nout = 0;
    char headstring[10];
    int ii, too_short;
    char *my_out;
    const char* my_in;
    int not_en, encodedTime;
#ifndef _WIN32
    int uid;
#endif

    wheel_len = 26 + 26 + 10 + 15;

    /*
     get uid to use as part of the key
    */
#ifndef _WIN32
    uid = getuid();
    uid = uid & 0xf5f; /* keep it fairly small and not exactly uid */
#endif

    /*
     Set up an array of characters that we will transpose.
    */
    j = 0;
    for ( i = 0; i < 10; i++ ) {
        wheel[j++] = ( int )'0' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'A' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'a' + i;
    }
    for ( i = 0; i < 15; i++ ) {
        wheel[j++] = ( int )'!' + i;
    }

    too_short = 0;
    for ( p1 = in, i = 0; i < 6; i++ ) {
        if ( *p1++ == '\0' ) {
            too_short = 1;
        }
    }

    kpos = 6;
    p1 = in;
    for ( i = 0; i < kpos; i++, p1++ ) {
        ;
    }
    rval = ( int ) * p1;
    rval = rval - 'e';

    if ( rval > 15 || rval < 0 || too_short == 1 ) { /* invalid key or too short */
        while ( ( *out++ = *in++ ) != '\0' ) {
            ;    /* return input string */
        }
        return AUTH_FILE_NOT_ENCRYPTED;
    }

    seq = 0;
    if ( rval == 0 ) {
        seq = 0xd768b678;
    }
    if ( rval == 1 ) {
        seq = 0xedfdaf56;
    }
    if ( rval == 2 ) {
        seq = 0x2420231b;
    }
    if ( rval == 3 ) {
        seq = 0x987098d8;
    }
    if ( rval == 4 ) {
        seq = 0xc1bdfeee;
    }
    if ( rval == 5 ) {
        seq = 0xf572341f;
    }
    if ( rval == 6 ) {
        seq = 0x478def3a;
    }
    if ( rval == 7 ) {
        seq = 0xa830d343;
    }
    if ( rval == 8 ) {
        seq = 0x774dfa2a;
    }
    if ( rval == 9 ) {
        seq = 0x6720731e;
    }
    if ( rval == 10 ) {
        seq = 0x346fa320;
    }
    if ( rval == 11 ) {
        seq = 0x6ffdf43a;
    }
    if ( rval == 12 ) {
        seq = 0x7723a320;
    }
    if ( rval == 13 ) {
        seq = 0xdf67d02e;
    }
    if ( rval == 14 ) {
        seq = 0x86ad240a;
    }
    if ( rval == 15 ) {
        seq = 0xe76d342e;
    }

    addin_i = 0;
    my_out = headstring;
    my_in = in;
    my_in++;   /* skip leading '.' */
    for ( ii = 0;; ) {
        ii++;
        if ( ii == 6 ) {
            not_en = 0;
            if ( *in != '.' ) {
                not_en = 1;  /* is not 'encrypted' */
            }
            if ( headstring[0] != 'S' - ( ( rval & 0x7 ) * 2 ) ) {
                not_en = 1;
                if ( obfDebug ) {
                    printf( "not s\n" );
                }
            }

            if ( timeVal == 0 ) {
                timeVal = obfiTimeval();
            }
            encodedTime = ( ( headstring[1] - 'a' ) << 4 ) + ( headstring[2] - 'a' ) +
                          ( ( headstring[3] - 'a' ) << 12 ) + ( ( headstring[4] - 'a' ) << 8 );

#ifndef _WIN32
            if ( obfiTimeCheck( encodedTime, timeVal ) ) {
                not_en = 1;
            }
#else    /* The file always contains an encrypted password in Windows. */
            not_en = 0;
#endif

            if ( obfDebug ) {
                printf( "timeVal=%d encodedTime=%d\n", timeVal, encodedTime );
            }

            my_out = out;   /* start outputing for real */
            if ( not_en == 1 ) {
                while ( ( *out++ = *in++ ) != '\0' ) {
                    ;    /* return input string */
                }
                return AUTH_FILE_NOT_ENCRYPTED;
            }
            my_in++;   /* skip key */
        }
        else {
            found = 0;
            addin = ( seq >> addin_i ) & 0x1f;
            addin += extra;
#ifndef _WIN32
            addin += uid;
            addin_i += 3;
#else
            addin_i += 2;
#endif
            if ( addin_i > 28 ) {
                addin_i = 0;
            }
            for ( i = 0; i < wheel_len; i++ ) {
                if ( *my_in == ( char )wheel[i] ) {
                    j = i - addin;
                    if ( obfDebug ) {
                        printf( "j=%d ", j );
                    }

                    while ( j < 0 ) {
                        j += wheel_len;
                    }
                    if ( obfDebug ) {
                        printf( "j2=%d \n", j );
                    }

                    *my_out++ = ( char )wheel[j];
                    nout++;
                    found = 1;
                    break;
                }
            }
            if ( found == 0 ) {
                if ( *my_in == '\0' ) {
                    *my_out++ = '\0';
                    return 0;
                }
                else {
                    *my_out++ = *my_in;
                    nout++;
                }
            }
            my_in++;
        }
    }
}

int
obfiGetEnvKey() {
    /* May want to do this someday, but at least not for now */
    return 0;
}

void
obfSetDefaultHashType( int type ) {
    defaultHashType = type;
    if ( obfDebug ) {
        printf( "hashType now %d\n", defaultHashType );
    }
}

int
obfGetDefaultHashType() {
    return defaultHashType;
}

/* Generate a hash string using MD5 or Sha1 */
void
obfMakeOneWayHash( int hashType,
                   unsigned const char *inBuf, int inBufSize, unsigned char *outHash ) {

    MD5_CTX md5Context;
    SHA1Context sha1Context;

    static char outBuf[50];

    if ( hashType == HASH_TYPE_SHA1 ||
            ( hashType == HASH_TYPE_DEFAULT && defaultHashType == HASH_TYPE_SHA1 ) ) {
        if ( obfDebug ) {
            printf( "obfMakeOneWayHash sha1\n" );
        }
        SHA1Reset( &sha1Context );
        SHA1Input( &sha1Context, inBuf, inBufSize );
        SHA1Result( &sha1Context );
        memcpy( ( void * )outHash, ( void * )&sha1Context.Message_Digest[0], 16 );
    }
    else {
        if ( obfDebug ) {
            printf( "obfMakeOneWayHash md5\n" );
        }
        MD5Init( &md5Context );
        MD5Update( &md5Context, inBuf, inBufSize );
        MD5Final( outHash, &md5Context );
    }
    sprintf( outBuf, "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
             outHash[0], outHash[1], outHash[2], outHash[3],
             outHash[4], outHash[5], outHash[6], outHash[7],
             outHash[8], outHash[9], outHash[10], outHash[11],
             outHash[12], outHash[13], outHash[14], outHash[15] );
}


/*
  Obfuscate a string using an input key
*/
void
obfEncodeByKey( const char *in, const char *key, char *out ) {
    /*
     Set up an array of characters that we will transpose.
    */

    int wheel_len = 26 + 26 + 10 + 15;
    int wheel[26 + 26 + 10 + 15];

    int i, j;
    int pc;    /* previous character */

    unsigned char buffer[65]; /* each digest is 16 bytes, 4 of them */
    unsigned char keyBuf[101];
    char *cpOut;
    const char *cpIn;
    unsigned char *cpKey;

    if ( obfDebug ) {
        printf( "obfEncodeByKey enter key:%s:in:%s\n", key, in );
    }

    j = 0;
    for ( i = 0; i < 10; i++ ) {
        wheel[j++] = ( int )'0' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'A' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'a' + i;
    }
    for ( i = 0; i < 15; i++ ) {
        wheel[j++] = ( int )'!' + i;
    }

    memset( keyBuf, 0, sizeof( keyBuf ) );
    snprintf( ( char* )keyBuf, sizeof( keyBuf ), "%s", key );
    memset( buffer, 0, 17 );

    /*
      Get the MD5/SHA1 digest of the key to get some bytes with many
      different values.
    */

    obfMakeOneWayHash( HASH_TYPE_DEFAULT, keyBuf, sizeof( keyBuf ) - 1, buffer );

    /* Hash of the hash */
    obfMakeOneWayHash( HASH_TYPE_DEFAULT, buffer, 16, buffer + 16 );

    /* Hash of 2 hashes */
    obfMakeOneWayHash( HASH_TYPE_DEFAULT, buffer, 32, buffer + 32 );

    /* Hash of 2 hashes */
    obfMakeOneWayHash( HASH_TYPE_DEFAULT, buffer, 32, buffer + 48 );

    cpIn = in;
    cpOut = out;
    if ( defaultHashType == HASH_TYPE_SHA1 ) {
        *cpOut++ = 's';
        *cpOut++ = 'h';
        *cpOut++ = 'a';
        *cpOut++ = '1';
    }

    cpKey = buffer;
    pc = 0;
    for ( ;; cpIn++ ) {
        int k, found;
        k = ( int ) * cpKey++;
        if ( cpKey > buffer + 60 ) {
            cpKey = buffer;
        }
        found = 0;
        for ( i = 0; i < wheel_len; i++ ) {
            if ( *cpIn == ( char )wheel[i] ) {
                j = i + k + pc;
                j = j % wheel_len;
                *cpOut++ = ( char )wheel[j];
                if ( cipherBlockChaining ) {
                    pc = *( cpOut - 1 );
                    pc = pc & 0xff;
                }
                found = 1;
                break;
            }
        }
        if ( found == 0 ) {
            if ( *cpIn == '\0' ) {
                *cpOut++ = '\0';
                if ( obfDebug ) printf( "obfEncodeByKey key:%s in:%s out:%s\n",
                                            key, in, out );
                return;
            }
            else {
                *cpOut++ = *cpIn;
            }
        }
    }
}

/*
  Obfuscate a string using an input key, version 2.  Version two is
  like the original but uses two key and a hash of them instead of the
  key itself (to keep the key itself even more undiscoverable).  The
  second key is a session signature (based on the challenge) (not
  secret but known to both client and server and unique for each
  connection).  It also uses a quasi-cipher-block-chaining algorithm
  and adds a random character (so the 'out' is different even with the
  same 'in' each time).
*/
#define V2_Prefix "A.ObfV2"

void
obfEncodeByKeyV2( const char *in, const char *key, const char *key2, char *out ) {
    struct timeval nowtime;
    char *myKey2;
    char myKey[200];
    char myIn[200];
    int rval;

    strncpy( myIn, V2_Prefix, 10 );
    strncat( myIn, in, 150 );

    strncpy( myKey, key, 90 );
    myKey[90] = '\0';
    strncat( myKey, key2, 100 );

    /*
     get a pseudo random number
    */
    ( void )gettimeofday( &nowtime, ( struct timezone * )0 );
    rval = nowtime.tv_usec & 0x1f;
    myIn[0] += rval; /* and add it to the leading character */


    myKey2 = obfGetMD5Hash( myKey );

    cipherBlockChaining = 1;
    obfEncodeByKey( myIn, myKey2, out );
    cipherBlockChaining = 0;
    return;
}


/*
  De-obfuscate a string using an input key
*/
void
obfDecodeByKey( const char *in, const char *key, char *out ) {
    /*
     Set up an array of characters that we will transpose.
    */
    int wheel_len = 26 + 26 + 10 + 15;
    int wheel[26 + 26 + 10 + 15];

    int i, j;
    int pc;
    unsigned char buffer[65]; /* each digest is 16 bytes, 4 of them */
    unsigned char keyBuf[101];
    char *cpOut;
    const char *cpIn;
    unsigned char *cpKey;

    int myHashType;

    if ( obfDebug ) {
        printf( "obfDecodeByKey enter key:%s: in:%s\n", key, in );
    }

    if ( strncmp( in, "sha1", 4 ) == 0 ) {
        in += 4;
        if ( obfDebug ) {
            printf( "using sha1 for decodebykey\n" );
        }
        myHashType = HASH_TYPE_SHA1;
    }
    else {
        if ( obfDebug ) {
            printf( "using md5 for decodebykey\n" );
        }
        myHashType = HASH_TYPE_MD5;
    }

    j = 0;
    for ( i = 0; i < 10; i++ ) {
        wheel[j++] = ( int )'0' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'A' + i;
    }
    for ( i = 0; i < 26; i++ ) {
        wheel[j++] = ( int )'a' + i;
    }
    for ( i = 0; i < 15; i++ ) {
        wheel[j++] = ( int )'!' + i;
    }

    memset( keyBuf, 0, sizeof( keyBuf ) );
    snprintf( ( char* )keyBuf, sizeof( keyBuf ), "%s", key );

    memset( buffer, 0, 65 );

    /*
      Get the MD5/SHA1 digest of the key to get some bytes with many
      different values.
    */
    obfMakeOneWayHash( myHashType, keyBuf, sizeof( keyBuf ) - 1, buffer );

    /* Hash of the hash */
    obfMakeOneWayHash( myHashType, buffer, 16, buffer + 16 );

    /* Hash of 2 hashes */
    obfMakeOneWayHash( myHashType, buffer, 32, buffer + 32 );

    /* Hash of 2 hashes */
    obfMakeOneWayHash( myHashType, buffer, 32, buffer + 48 );

    cpIn = in;
    cpOut = out;
    cpKey = buffer;
    pc = 0;
    for ( ;; cpIn++ ) {
        int k, found;
        k = ( int ) * cpKey++;
        if ( cpKey > buffer + 60 ) {
            cpKey = buffer;
        }
        found = 0;
        for ( i = 0; i < wheel_len; i++ ) {
            if ( *cpIn == ( char )wheel[i] ) {
                j = i - k - pc;
                while ( j < 0 ) {
                    j += wheel_len;
                }
                *cpOut++ = ( char )wheel[j];
                if ( cipherBlockChaining ) {
                    pc = *cpIn;
                    pc = pc & 0xff;
                }
                found = 1;
                break;
            }
        }
        if ( found == 0 ) {
            if ( *cpIn == '\0' ) {
                *cpOut++ = '\0';
                if ( obfDebug ) printf( "obfDecodeByKey key:%s: in:%s out: %s\n",
                                            key, in, out );
                return;
            }
            else {
                *cpOut++ = *cpIn;
            }
        }
    }
}

/* Version 2, undoes V2 encoding.
   If encoding is not V2, handles is at V1 (original)
*/
void
obfDecodeByKeyV2( const char *in, const char *key, const char *key2, char *out ) {
    char *myKey2;
    static char myOut[200];
    int i, len, matches;
    char match[60];
    char myKey[200];

    strncpy( myKey, key, 90 );
    myKey[90] = '\0';
    strncat( myKey, key2, 100 );

    myKey2 = obfGetMD5Hash( myKey );
    cipherBlockChaining = 1;
    obfDecodeByKey( in, myKey2, myOut );
    cipherBlockChaining = 0;

    strncpy( match, V2_Prefix, 10 );
    len = strlen( V2_Prefix );
    matches = 1;
    for ( i = 1; i < len; i++ ) {
        if ( match[i] != myOut[i] ) {
            matches = 0;
        }
    }
    if ( matches == 0 ) {
        obfDecodeByKey( in, key, out );
        return;
    }

    strncpy( out, myOut + len, MAX_PASSWORD_LEN ); /* skip prefix */
    return;
}

/*
  Hash an input string
*/
char *
obfGetMD5Hash( const char *stringToHash ) {
    /*
      Set up an array of characters that we will transpose.
    */
    unsigned char buffer[30]; /* the digest is 16 bytes for MD5, 20 SHA1 */
    unsigned char keyBuf[101];

    static char outBuf[50];

    memset( keyBuf, 0, sizeof( keyBuf ) );
    snprintf( ( char* )keyBuf, sizeof( keyBuf ), "%s", stringToHash );

    memset( buffer, 0, sizeof( buffer ) );
    /*
      Get the MD5 (or SHA1) digest of the key
    */
    obfMakeOneWayHash( HASH_TYPE_DEFAULT, keyBuf, sizeof( keyBuf ) - 1, buffer );

    sprintf( outBuf, "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
             buffer[0], buffer[1], buffer[2], buffer[3],
             buffer[4], buffer[5], buffer[6], buffer[7],
             buffer[8], buffer[9], buffer[10], buffer[11],
             buffer[12], buffer[13], buffer[14], buffer[15] );
    return outBuf;
}
