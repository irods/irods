/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is an initial test program which performs some user-level
  operations.  This is a quick and easy way to test a few client API
  calls so I can test, debug, and refine the ICAT code on the server
  side.  I'm planning to add an 'ls' first (via the new general
  query), then something to stat a collection after the 'cd' (another
  general query), then maybe a 'ls -l' (another general query).

  Then I might add a 'mkdir' and 'rmdir' to test those calls, which
  again are primarily ICAT operations, and would allow me to refine
  and debug those.

  After that, we might want to try an actual 'put' and 'get', which
  would test a lot more of the iRODS system.

  But this simple little test program that is not intended to be an
  actual user interface program.  For those, we'd like to have
  separate programs for each of the operations (iinit, ipwd, ils,
  iput, etc).  But that will take more client-side infrastructure.
  Some of this code might be a basis for those utilities, but with
  much refinement.
*/

#include <stdio.h>

#include "rodsClient.h"
#include "parseCommandLine.h"

#define BIG_STR 200

int debug = 0;

char cwd[BIG_STR];
char prevCwd[BIG_STR];
#define MY_MODE          0750

int doGetTempPw( rcComm_t *Conn, char *userPassword ) {
    int status, len, i;
    getTempPasswordOut_t *getTempPasswordOut;
    char md5Buf[100];
    char digest[100];
    char tempPw[100];
    MD5_CTX context;

    if ( strlen( userPassword ) <= 0 ) {
        printf( "You need to include the user password\n" );
        return -1;
    }

    status = rcGetTempPassword( Conn, &getTempPasswordOut );
    if ( status ) {
        printError( Conn, status, "rcGetTempPassword" );
        return status;
    }
    printf( "stringToHashWith: %s\n", getTempPasswordOut->stringToHashWith );

    /* calcuate the temp password (a hash of the user's main pw and
       stringToHashWith) */

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, getTempPasswordOut->stringToHashWith, 100 );
    strncat( md5Buf, userPassword, 100 );

    MD5_Init( &context );
    MD5_Update( &context, md5Buf, 100 );
    MD5_Final( digest, &context );

    hashToStr( digest, tempPw );
    printf( "tempPw=%s\n", tempPw );
    return 0;
}

int doCheck( char *buf ) {
    MD5_CTX context;
    int len;
    unsigned char buffer[65]; /* each digest is 16 bytes, 4 of them */
    int status;
    int ints[30];
    int pid;
    struct timeval tv;
    static int count = 12348;
    int i, j;

    gettimeofday( &tv, 0 );
    pid = getpid();
    count++;

    ints[0] = 12349994;
    ints[1] = count;
    ints[2] = tv.tv_usec;
    MD5_Init( &context );
    MD5_Update( &context, ( char* )&ints[0], 100 );
    MD5_Final( buffer, &context );

    ints[0] = pid;
    ints[4] = ( int )buffer[10];
    MD5_Init( &context );
    MD5_Update( &context, ( char * )&ints[0], 100 );
    MD5_Final( buffer + 16, &context );

    MD5_Init( &context );
    MD5_Update( &context, ( char* )&ints[0], 100 );
    MD5_Final( buffer + 32, &context );

    MD5_Init( &context );
    MD5_Update( &context, buffer, 40 );
    MD5_Final( buffer + 48, &context );

    for ( i = 0; i < 64; i++ ) {
        if ( buffer[i] == '\0' ) {
            buffer[i]++;  /* make sure no nulls before end of 'string'*/
        }
    }
    buffer[64] = '\0';
    strncpy( buf, buffer, 65 );
}

int doAuth2( rcComm_t *Conn, char *userName ) {
    int status, len, i;
    authRequestOut_t *authReqOut;
    authResponseInp_t authRespIn;
    char pwBuf[MAX_PASSWORD_LEN + 1];
    char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
    char digest[RESPONSE_LEN + 2];
    MD5_CTX context;

    status = rcAuthRequest( Conn, &authReqOut );
    if ( status ) {
        printError( Conn, status, "rcAuthRequest" );
        return status;
    }

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, authReqOut->challenge, CHALLENGE_LEN );
    i = obfGetPw( md5Buf + CHALLENGE_LEN );
    if ( i != 0 ) {
        printf( "Enter password:" );
        fgets( md5Buf + CHALLENGE_LEN, MAX_PASSWORD_LEN, stdin );
        len = strlen( md5Buf );
        md5Buf[len - 1] = '\0'; /* remove trailing \n */
    }
    MD5_Init( &context );
    MD5_Update( &context, md5Buf, CHALLENGE_LEN + MAX_PASSWORD_LEN );
    MD5_Final( digest, &context );
    for ( i = 0; i < RESPONSE_LEN; i++ ) {
        if ( digest[i] == '\0' ) {
            digest[i]++;
        }  /* make sure 'string' doesn't
					    end early*/
    }
    authRespIn.response = digest;
    authRespIn.username = userName;
    status = rcAuthResponse( Conn, &authRespIn );
    if ( status ) {
        printError( Conn, status, "rcAuthResponse" );
        return status;
    }

    return 0;
}

/* test the encoding/decoding routines */
int
doEncode( char *textIn, char *key ) {
    char out[100], redone[100];
    char in[100];
    char rand[] =
        "ra31gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs26SoBXIv";
    char rand15[20];
    char *cp;
    int len, lcopy;
    strcpy( in, textIn );
    len = strlen( textIn );
    lcopy = 40 - len;
    if ( lcopy > 15 ) {
        strncat( in, rand, lcopy );
    }

    obfEncodeByKey( in, key, out );
    printf( "out=%s\n", out );
    obfDecodeByKey( out, key, redone );
    strncpy( rand15, rand, 15 );
    rand15[15] = '\0';
    cp = strstr( redone, rand15 );
    if ( cp != NULL ) {
        *cp = '\0';
    }
    printf( "reconverted=%s\n", redone );
}

/* test the rcCheckAuth call which is normally server to server */
int doAuthT( rcComm_t *Conn, char *userName ) {
    int status, len, i;
    authRequestOut_t *authReqOut;
    authCheckInp_t authCheckIn;
    authCheckOut_t *authCheckOut;
    char pwBuf[MAX_PASSWORD_LEN + 1];
    char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
    char digest[RESPONSE_LEN + 2];
    MD5_CTX context;

    status = rcAuthRequest( Conn, &authReqOut );  /* easy way to get a challenge
						  buffer */
    if ( status ) {
        printError( Conn, status, "rcAuthRequest" );
        return status;
    }

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, authReqOut->challenge, CHALLENGE_LEN );
    i = obfGetPw( md5Buf + CHALLENGE_LEN );
    if ( i != 0 ) {
        printf( "Enter password:" );
        fgets( md5Buf + CHALLENGE_LEN, MAX_PASSWORD_LEN, stdin );
        len = strlen( md5Buf );
        md5Buf[len - 1] = '\0'; /* remove trailing \n */
    }
    MD5_Init( &context );
    MD5_Update( &context, md5Buf, CHALLENGE_LEN + MAX_PASSWORD_LEN );
    MD5_Final( digest, &context );
    for ( i = 0; i < RESPONSE_LEN; i++ ) {
        if ( digest[i] == '\0' ) {
            digest[i]++;
        }  /* make sure 'string' doesn't
					    end early*/
    }

    authCheckIn.challenge = authReqOut->challenge;
    authCheckIn.response = digest;
    authCheckIn.username = userName;
    status = rcAuthCheck( Conn, &authCheckIn, &authCheckOut );
    if ( status ) {
        printError( Conn, status, "rcAuthCheck" );
        return status;
    }
    printf( "priv=%d\n", authCheckOut->privLevel );
    return 0;
}

int doRm( rcComm_t *Conn, char *file ) {
    int status;
    dataObjInp_t dataObjOprInp;

    memset( &dataObjOprInp, 0, sizeof( dataObjOprInp ) );
    if ( strlen( cwd ) > 1 ) {
        snprintf( dataObjOprInp.objPath, MAX_NAME_LEN, "%s/%s",
                  cwd, file );
    }
    else {
        snprintf( dataObjOprInp.objPath, MAX_NAME_LEN, "%s%s",
                  cwd, file );
    }

    status = rcDataObjUnlink( Conn, &dataObjOprInp );
    if ( status < 0 ) {
        printError( Conn, status, "rcDataObjUnlink" );
        return status;
    }
    return 0;
}

int doPut( rcComm_t *Conn, char *file, char *rescName ) {
    dataObjInp_t dataObjOprInp;
    bytesBuf_t dataObjInpBBuf;
    struct stat statbuf;
    int status;

    printf( "file=%s\n", file );
    status = stat( file, &statbuf );
    if ( status < 0 ) {
        fprintf( stderr, "stat of %s failed\n", file );
        return -1;
    }
    else {
        printf( "input file %s size = %d\n",
                file, statbuf.st_size );
    }

    memset( &dataObjOprInp, 0, sizeof( dataObjOprInp ) );
    memset( &dataObjInpBBuf, 0, sizeof( dataObjInpBBuf ) );

    snprintf( dataObjOprInp.objPath, MAX_NAME_LEN, "%s/%s",
              cwd, file );
    addKeyVal( &dataObjOprInp.condInput, DEST_RESC_NAME_KW, rescName );
    addKeyVal( &dataObjOprInp.condInput, RESC_NAME_KW, rescName );
    addKeyVal( &dataObjOprInp.condInput, DATA_TYPE_KW, "generic" );
    dataObjOprInp.createMode = MY_MODE;
    dataObjOprInp.openFlags = O_WRONLY;
    /*    dataObjOprInp.numThreads = 4; */
    dataObjOprInp.numThreads = 1;
    dataObjOprInp.dataSize = statbuf.st_size;

    status = rcChksumLocFile( file, VERIFY_CHKSUM_KW,
                              &dataObjOprInp.condInput );
    if ( status < 0 ) {
        printError( Conn, status, "rcChksumLocFile" );
        return status;
    }

    status = rcDataObjPut( Conn, &dataObjOprInp, file );

    if ( status < 0 ) {
        printError( Conn, status, "rcDataObjPut" );
        return status;
    }
    else {
        printf( "rcDataObjPut: status = %d\n", status );
    }

    return 0;
}

int
printGenQueryResults( rcComm_t *Conn, int status, genQueryOut_t *genQueryOut,
                      char *description ) {
    int printCount;
    int i;
    printCount = 0;
    if ( debug ) {
        printf( "chlGenQuery status=%d\n", status );
        if ( status == 0 ) {
            printf( "genQueryOut->rowCnt=%d\n", genQueryOut->rowCnt );
            printf( "genQueryOut->attriCnt=%d\n", genQueryOut->attriCnt );
            for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
                int j;
                printf( "genQueryOut->SqlResult[%d].attriInx=%d\n", i,
                        genQueryOut->sqlResult[i].attriInx );
                printf( "genQueryOut->SqlResult[%d].len=%d\n", i,
                        genQueryOut->sqlResult[i].len );
                for ( j = 0; j < genQueryOut->rowCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[i].value;
                    tResult += j * genQueryOut->sqlResult[i].len;
                    printf( "genQueryOut->SqlResult[%d].value=%s\n", i, tResult );
                }
                printf( "genQueryOut->continueInx=%d\n", genQueryOut->continueInx );
            }
        }
    }
    else {
        if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
            printError( Conn, status, "rcGenQuery" );
        }
        else {
            if ( status != CAT_NO_ROWS_FOUND ) {
                for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
                    int j;
                    for ( j = 0; j < genQueryOut->rowCnt; j++ ) {
                        char *tResult;
                        tResult = genQueryOut->sqlResult[i].value;
                        tResult += j * genQueryOut->sqlResult[i].len;
                        printf( "%s %s\n", description, tResult );
                        printCount++;
                    }
                }
            }
        }
    }

}

int doLs( rcComm_t *Conn ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    int done;
    int mode;
    char *condVal[2];
    char v1[BIG_STR];
    int i, status;
    int printCount;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    /* first get all the subcollections */
    printCount = 0;
    i1a[0] = COL_COLL_NAME;
    i1b[0] = 0; /* currently unused */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_COLL_PARENT_NAME;
    genQueryInp.sqlCondInp.inx = i2a;
    sprintf( v1, "='%s'", cwd );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    printCount += printGenQueryResults( Conn, status, genQueryOut, "coll:" );

    /* now get the files */
    i1a[0] = COL_DATA_NAME;
    i1b[0] = 0; /* currently unused */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_COLL_NAME;
    genQueryInp.sqlCondInp.inx = i2a;
    sprintf( v1, "='%s'", cwd );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    printCount += printGenQueryResults( Conn, status, genQueryOut, "file:" );

    if ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        printCount += printGenQueryResults( Conn, status, genQueryOut, "file:" );
    }

    if ( debug == 0 && printCount == 0 ) {
        printf( "no subcollections or files found\n" );
    }
    return 0;
}

int doLongLs( rcComm_t *Conn, char *inStr, char *inRepl ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[25];
    int i1b[25];
    int i2a[10];
    int done;
    int mode;
    char *condVal[5];
    char v1[BIG_STR];
    char v2[BIG_STR];
    char v3[BIG_STR];
    char v4[BIG_STR];
    char comboStr[BIG_STR];
    int i, status;
    int colNameIx;
    char *colNames[] = {"id", "name", "parent_name", "owner_name", "owner_zone",
                        "map_id", "inheritance", "comments", "create_time",
                        "modify_time"
                       };

    char *dataColNames[] = {"id", "coll_id", "name", "repl_num",
                            "version", "type_name",
                            "size", "resc_name", "data_path",
                            "owner_name", "owner_zone", "repl_status",
                            "data_status", "checksum", "expiry", "map_id",
                            "comments", "create_time", "modify_time"
                           };
    char *dataAccColNames[] = {"user", "has access"};
    char dataId[MAX_NAME_LEN];


    /* first get all the collection information (if it is one) */
    i1a[0] = COL_COLL_ID;
    i1a[1] = COL_COLL_NAME;
    i1a[2] = COL_COLL_PARENT_NAME;
    i1a[3] = COL_COLL_OWNER_NAME;
    i1a[4] = COL_COLL_OWNER_ZONE;
    i1a[5] = COL_COLL_MAP_ID;
    i1a[6] = COL_COLL_INHERITANCE;
    i1a[7] = COL_COLL_COMMENTS;
    i1a[8] = COL_COLL_CREATE_TIME;
    i1a[9] = COL_COLL_MODIFY_TIME;

    i1b[0] = 0; /* currently unused */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 10;

    i2a[0] = COL_COLL_NAME;
    i2a[1] = COL_DATA_NAME;
    genQueryInp.sqlCondInp.inx = i2a;
    strcpy( comboStr, cwd );
    if ( strlen( inStr ) > 0 ) {
        if ( comboStr[strlen( comboStr ) - 1] != '/' ) {
            strncat( comboStr, "/", BIG_STR );
        }
        strncat( comboStr, inStr, BIG_STR );
    }
    sprintf( v1, "='%s'", comboStr );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
        printError( Conn, status, "rcGenQuery" );
    }
    else {
        if ( status != CAT_NO_ROWS_FOUND ) {
            printf( "Collection: %s\n", comboStr );
            colNameIx = 0;
            for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
                int j;
                for ( j = 0; j < genQueryOut->rowCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[i].value;
                    tResult += j * genQueryOut->sqlResult[i].len;
                    printf( "%s: %s\n", colNames[colNameIx++], tResult );
                }
            }
            return 0;
        }
    }

    if ( strlen( inStr ) == 0 ) {
        printf( "%s is not a collection\n", comboStr );
        return 0;
    }

    /* now try as a file */
    i1a[0] = COL_D_DATA_ID;
    i1a[1] = COL_D_COLL_ID;
    i1a[2] = COL_DATA_NAME;
    i1a[3] = COL_DATA_REPL_NUM;
    i1a[4] = COL_DATA_VERSION;
    i1a[5] = COL_DATA_TYPE_NAME;
    i1a[6] = COL_DATA_SIZE;
    i1a[7] = COL_D_RESC_NAME;
    i1a[8] = COL_D_DATA_PATH;
    i1a[9] = COL_D_OWNER_NAME;
    i1a[10] = COL_D_OWNER_ZONE;
    i1a[11] = COL_D_REPL_STATUS;
    i1a[12] = COL_D_DATA_STATUS;
    i1a[13] = COL_D_DATA_CHECKSUM;
    i1a[14] = COL_D_EXPIRY;
    i1a[15] = COL_D_MAP_ID;
    i1a[16] = COL_D_COMMENTS;
    i1a[17] = COL_D_CREATE_TIME;
    i1a[18] = COL_D_MODIFY_TIME;

    i1b[0] = 0; /* currently unused */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 19;

    genQueryInp.sqlCondInp.inx = i2a;
    sprintf( v1, "='%s'", cwd );
    condVal[0] = v1;
    sprintf( v2, "='%s'", inStr );
    condVal[1] = v2;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;
    if ( strlen( inRepl ) > 0 ) {
        sprintf( v3, "='%s'", inRepl );
        condVal[2] = v3;
        i2a[2] = COL_DATA_REPL_NUM;
        genQueryInp.sqlCondInp.len++;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status != 0 ) {
        printError( Conn, status, "rcGenQuery" );
        printf( "%s does not exist (neither a collection or a file)\n", comboStr );
        return 0;
    }
    else {
        if ( status != CAT_NO_ROWS_FOUND ) {
            printf( "File: %s\n", comboStr );
            colNameIx = 0;
            for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
                int j;
                for ( j = 0; j < genQueryOut->rowCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[i].value;
                    tResult += j * genQueryOut->sqlResult[i].len;
                    printf( "%s: %s\n", dataColNames[colNameIx], tResult );
                    if ( colNameIx == 0 ) {
                        strncpy( dataId, tResult, MAX_NAME_LEN );
                    }
                }
                colNameIx++;
            }
        }
    }

    /* now get the ACL */
    i1a[0] = COL_USER_NAME;
    i1a[1] = COL_DATA_ACCESS_NAME;

    i1b[0] = 0; /* currently unused */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 2;

    i2a[0] = COL_DATA_ACCESS_DATA_ID;
    genQueryInp.sqlCondInp.inx = i2a;
    sprintf( v1, "='%s'", dataId );
    condVal[0] = v1;
    i2a[1] = COL_DATA_TOKEN_NAMESPACE;
    sprintf( v2, "='%s'", "access_type" );  /* Currently necessary since
					   other namespaces exist in
                                           the token table */
    condVal[1] = v2;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        printf( "No ACL)\n", comboStr );
        return 0;
    }
    if ( status != 0 ) {
        printError( Conn, status, "rcGenQuery" );
        return 0;
    }
    else {
        int j;
        colNameIx = 0;
        for ( j = 0; j < genQueryOut->rowCnt; j++ ) {
            for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
                char *tResult;
                tResult = genQueryOut->sqlResult[i].value;
                tResult += j * genQueryOut->sqlResult[i].len;
                printf( "%s: %s ", dataAccColNames[colNameIx++],
                        tResult );
            }
            colNameIx = 0;
            printf( "\n" );
        }
    }
    return 0;
}

int
doInsert( rcComm_t *Conn, char *tok1, char *tok2, char *tok3, char *tok4,
          char *tok5, char *tok6, char *tok7, char *tok8, char *tok9, char *tok10 ) {
    int status;
    generalRowInsertInp_t generalRowInsertInp;

    memset( &generalRowInsertInp, 0, sizeof( generalRowInsertInp_t ) );

    generalRowInsertInp.tableName = tok1;
    generalRowInsertInp.arg1 = tok2;
    generalRowInsertInp.arg2 = tok3;
    generalRowInsertInp.arg3 = tok4;
    generalRowInsertInp.arg4 = tok5;
    generalRowInsertInp.arg5 = tok6;
    generalRowInsertInp.arg6 = tok7;
    generalRowInsertInp.arg7 = tok8;
    generalRowInsertInp.arg8 = tok9;
    generalRowInsertInp.arg9 = tok10;

    status = rcGeneralRowInsert( Conn, &generalRowInsertInp );
    if ( status ) {
        rodsLogError( LOG_ERROR, status, "rcGeneralRowInsert failure" );
    }
    return status;
}

int
doPurge( rcComm_t *Conn, char *tok1, char *tok2 ) {
    int status;
    generalRowPurgeInp_t generalRowPurgeInp;

    memset( &generalRowPurgeInp, 0, sizeof( generalRowPurgeInp_t ) );

    generalRowPurgeInp.tableName = tok1;
    generalRowPurgeInp.secondsAgo = tok2;

    status = rcGeneralRowPurge( Conn, &generalRowPurgeInp );
    if ( status ) {
        rodsLogError( LOG_ERROR, status, "rcGeneralRowPurge failure" );
    }
    return status;
}

/* Check to see if the user has read or better access to a dataObj */
/* for example:
login rods
acc rods z2 /z2/home/rods foo 'read object' or
acc u2 z2 /z2/home/rods foo 'read object'
*/
int
doAccCheck( rcComm_t *Conn, char *user, char *zone, char *coll,
            char *dataObjName, char *access, char *access2 ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i, status;
    int printCount;
    char accStr[300];
    char condStr[MAX_NAME_LEN]; // JMC cppcheck - out of bounds snprintf

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    printCount = 0;

    snprintf( condStr, MAX_NAME_LEN, "%s", user );
    addKeyVal( &genQueryInp.condInput, USER_NAME_CLIENT_KW, condStr );

    snprintf( condStr, MAX_NAME_LEN, "%s", zone );
    addInxVal( &genQueryInp.condInput, RODS_ZONE_CLIENT_KW, condStr );

    snprintf( condStr, MAX_NAME_LEN, "%s %s", access, access2 );
    addKeyVal( &genQueryInp.condInput, ACCESS_PERMISSION_KW, condStr );

    snprintf( condStr, MAX_NAME_LEN, "='%s'", coll );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );

    snprintf( condStr, MAX_NAME_LEN, "='%s'", dataObjName );
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_NAME, condStr );

    /*   addInxIval (&genQueryInp.selectInp, COL_DATA_ACCESS_TYPE, 1); */
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_ID, 1 );

    /* acc rods tempZone /tempZone/home/rods t4 read object */

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    printCount += printGenQueryResults( Conn, status, genQueryOut, "testout:" );

    if ( printCount == 0 ) {
        printf( "no access\n" );
    }
    if ( printCount > 0 ) {
        printf( "Yes, access allowed\n" );
    }
    return 0;
}

main( int argc, char **argv ) {
    int status, i;
    rodsEnv myEnv;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    simpleQueryInp_t simpleQueryInp;
    simpleQueryOut_t *simpleQueryOut;
    char *myOutBuf;

    generalAdminInp_t generalAdminInp;

    rodsArguments_t myRodsArgs;
    char *mySubName;
    char *myName;

    int mode1, mode2;
    int argOffset;

    char ttybuf[BIG_STR];
    char rescName[100] = "";

    int lenstr;
    int ntokens;
    char *tok1;
    char *tok2;
    char *tok3;
    char *tok4;
    char *tok5;
    char *tok6;
    char *tok7;
    char *tok8;
    char *tok9;
    char *tok10;
    char *tok11;
    int blankFlag;
    int didOne;

    rodsLogLevel( LOG_ERROR ); /* This should be the default someday */

    parseCmdLineOpt( argc, argv, "vV", 0, &myRodsArgs );
    if ( myRodsArgs.verbose == True ) {
        rodsLogLevel( LOG_NOTICE );
    }
    if ( myRodsArgs.veryVerbose == True ) {
        rodsLogLevel( LOG_DEBUG1 );
    }

    argOffset = myRodsArgs.optind;

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        printError( Conn, status, "getRodsEnv" );
        exit( 1 );
    }

    strncpy( cwd, myEnv.rodsHome, BIG_STR );
    if ( strlen( cwd ) == 0 ) {
        strcpy( cwd, "/" );
    }


    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        myName = rodsErrorName( errMsg.status, &mySubName );
        rodsLogError( LOG_ERROR, errMsg.status, "rcConnect failure" );

        rodsLog( LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
                 myName,
                 mySubName,
                 errMsg.status,
                 errMsg.msg );

        exit( 2 );
    }

    for ( ;; ) {
        didOne = 0;
        putc( '>', stdout );
        fgets( ttybuf, BIG_STR, stdin );
        lenstr = strlen( ttybuf );
        ttybuf[lenstr - 1] = '\0'; /* trim off the trailing \n */
        lenstr--;
        tok1 = ttybuf;
        tok2 = "";
        tok3 = "";
        tok4 = "";
        tok5 = "";
        tok6 = "";
        tok7 = "";
        tok8 = "";
        tok9 = "";
        tok10 = "";
        tok11 = "";
        ntokens = 1;
        blankFlag = 0;
        for ( i = 0; i < lenstr; i++ ) {
            if ( ttybuf[i] == ' ' ) {
                ttybuf[i] = '\0';
                blankFlag = 1;
            }
            else {
                if ( blankFlag ) {
                    ntokens++;
                    if ( ntokens == 2 ) {
                        tok2 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 3 ) {
                        tok3 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 4 ) {
                        tok4 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 5 ) {
                        tok5 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 6 ) {
                        tok6 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 7 ) {
                        tok7 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 8 ) {
                        tok8 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 9 ) {
                        tok9 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 10 ) {
                        tok10 = ( char * )&ttybuf[i];
                    }
                    if ( ntokens == 11 ) {
                        tok11 = ( char * )&ttybuf[i];
                    }
                }
                blankFlag = 0;
            }
        }

        if ( strcmp( tok1, "exit" ) == 0 || strcmp( tok1, "quit" ) == 0 ||
                strcmp( tok1, "q" ) == 0 ) {
            rcDisconnect( Conn );
            exit( 0 );
        }
        if ( strcmp( tok1, "debug" ) == 0 ) {
            if ( debug == 1 ) {
                debug = 0;
            }
            else {
                debug = 1;
            }
            didOne = 1;
        }
        if ( strcmp( tok1, "pwd" ) == 0 ) {
            printf( "%s\n", cwd );
            didOne = 1;
        }
        if ( strncmp( tok1, "ls", 2 ) == 0 ) {
            if ( ntokens == 1 ) {
                doLs( Conn );
            }
            else {
                doLongLs( Conn, tok2, tok3 );
            }
            didOne = 1;
        }
        if ( strncmp( tok1, "resc", 4 ) == 0 ) {
            strncpy( rescName, tok2, 100 );
            didOne = 1;
        }
        if ( strncmp( tok1, "put", 3 ) == 0 ) {
            if ( strlen( rescName ) == 0 ) {
                printf( "Do 'resc name' to set a resource first\n" );
            }
            else {
                doPut( Conn, tok2, rescName );
            }
            didOne = 1;
        }
        if ( strncmp( tok1, "login", 5 ) == 0 ) {
            doAuth2( Conn, myEnv.rodsUserName );
            didOne = 1;
        }
        if ( strncmp( tok1, "logtest", 7 ) == 0 ) { /* test login */
            doAuthT( Conn, myEnv.rodsUserName );
            didOne = 1;
        }
        if ( strncmp( tok1, "obf", 3 ) == 0 ) {
            doEncode( tok2, tok3 );
            didOne = 1;
        }
        if ( strncmp( tok1, "ch", 2 ) == 0 ) {
            char buf[100];
            doCheck( buf );
            printf( "buf[0]=%d\n", ( int )buf[0] );
            didOne = 1;
        }
        if ( strncmp( tok1, "tpw", 3 ) == 0 ) {
            doGetTempPw( Conn, tok2 );
            didOne = 1;
        }
        if ( strncmp( tok1, "rm", 2 ) == 0 ) {
            doRm( Conn, tok2 );
            didOne = 1;
        }
        if ( strncmp( tok1, "ll", 2 ) == 0 ) {
            doLongLs( Conn, tok2, tok3 );
            didOne = 1;
        }
        if ( strncmp( tok1, "acc", 3 ) == 0 ) {
            doAccCheck( Conn, tok2, tok3, tok4, tok5, tok6, tok7 );
            didOne = 1;
        }
        if ( strncmp( tok1, "insert", 6 ) == 0 ) {
            doInsert( Conn, tok2, tok3, tok4, tok5, tok6, tok7, tok8, tok9,
                      tok10, tok11 );
            didOne = 1;
        }
        if ( strncmp( tok1, "purge", 5 ) == 0 ) {
            doPurge( Conn, tok2, tok3 );
            didOne = 1;
        }
        if ( strncmp( tok1, "cd", 2 ) == 0 ) {
            int i, j, k;
            didOne = 1;
            strcpy( prevCwd, cwd );
            if ( strncmp( tok2, "..", 2 ) == 0 ) {
                int done;
                i = strlen( cwd );
                done = 0;
                for ( j = i; j >= 0 && done == 0; j-- ) {
                    if ( cwd[j] == '/' ) {
                        if ( j == 0 ) {
                            cwd[j + 1] = '\0';
                        }
                        else {
                            cwd[j] = '\0';
                        }
                        done = 1;
                    }
                }
            }
            else {
                char *mycp;
                if ( *tok2 == '/' ) {
                    cwd[0] = '/';
                    cwd[1] = '\0';
                }
                i = strlen( cwd );
                if ( cwd[i - 1] != '/' ) {
                    rstrcat( cwd, "/", BIG_STR );
                }
                mycp = tok2;
                if ( *mycp == '/' ) {
                    mycp++;
                }
                k = strlen( tok2 );
                if ( *( tok2 + k ) == '/' ) {
                    k--;    /* back up past trailing / */
                }
                *( tok2 + k ) = '\0';
                rstrcat( cwd, mycp, BIG_STR );
            }
        }
        if ( didOne == 0 ) {
            printf( "Unknown command\n" );
        }
    }
}

