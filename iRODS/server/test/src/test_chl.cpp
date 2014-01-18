/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
   ICAT test program.
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "readServerConfig.hpp"
#include "irods_server_properties.hpp"

#include "rodsUser.hpp"

#include "icatHighLevelRoutines.hpp"
#include "icatMidLevelRoutines.hpp"

#include <string.h>
#include <string>

extern icatSessionStruct *chlGetRcs();


/*
  int testCml(rsComm_t *rsComm)
  {
  return(cmlTest(rsComm));
  }
*/

rodsEnv myEnv;

int testRegUser( rsComm_t *rsComm, char *name,
                 char *zone, char *userType, char *dn ) {
    userInfo_t userInfo;

    strncpy( userInfo.userName, name, sizeof userInfo.userName );
    strncpy( userInfo.rodsZone, zone, sizeof userInfo.rodsZone );
    strncpy( userInfo.userType, userType, sizeof userInfo.userType );
    strncpy( userInfo.authInfo.authStr, dn, sizeof userInfo.authInfo.authStr );

    /* no longer used   return(chlRegUser(rsComm, &userInfo)); */
    return( 0 );
}

int testRegRule( rsComm_t *rsComm, char *name ) {
    ruleExecSubmitInp_t ruleInfo;

    memset( &ruleInfo, 0, sizeof( ruleInfo ) );

    strncpy( ruleInfo.ruleName, name, sizeof ruleInfo.ruleName );

    strncpy( ruleInfo.reiFilePath, "../config/packedRei/rei.file1",
             sizeof ruleInfo.reiFilePath );
    strncpy( ruleInfo.userName, "Wayne", sizeof ruleInfo.userName );
    strncpy( ruleInfo.exeAddress, "Bermuda", sizeof ruleInfo.exeAddress );
    strncpy( ruleInfo.exeTime, "whenEver", sizeof ruleInfo.exeTime );
    strncpy( ruleInfo.exeFrequency, "every 2 days", sizeof ruleInfo.exeFrequency );
    strncpy( ruleInfo.priority, "high", sizeof ruleInfo.priority );
    strncpy( ruleInfo.estimateExeTime, "2 hours", sizeof ruleInfo.estimateExeTime );
    strncpy( ruleInfo.notificationAddr, "noone@nowhere.com",
             sizeof ruleInfo.notificationAddr );
    return( chlRegRuleExec( rsComm, &ruleInfo ) );
}

int testRename( rsComm_t *rsComm, char *id, char *newName ) {
    rodsLong_t intId;
    int status;
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    intId = strtoll( id, 0, 0 );
    status = chlRenameObject( rsComm, intId, newName );
    if ( status ) {
        return( status );
    }
    return( chlCommit( rsComm ) );
}

int testLogin( rsComm_t *rsComm, char *User, char *pw, char *pw1 ) {
    int status;
    rcComm_t *Conn;
    rErrMsg_t errMsg;

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );
    if ( Conn == NULL ) {
        printf( "rcConnect failure" );
        return -1;
    }

    status = clientLoginWithPassword( Conn, pw1 ); /* first login as self */
    if ( status == 0 ) {
        rstrcpy( Conn->clientUser.userName, User,
                 sizeof Conn->clientUser.userName );
        rstrcpy( Conn->clientUser.rodsZone, myEnv.rodsZone,
                 sizeof Conn->clientUser.rodsZone ); /* default to our zone */
        status = clientLoginWithPassword( Conn, pw ); /* then try other user */
    }

    rcDisconnect( Conn );

    return( status );
}

int testMove( rsComm_t *rsComm, char *id, char *destId ) {
    rodsLong_t intId, intDestId;
    int status;
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    intId = strtoll( id, 0, 0 );
    intDestId = strtoll( destId, 0, 0 );
    status = chlMoveObject( rsComm, intId, intDestId );
    if ( status ) {
        return( status );
    }
    return( chlCommit( rsComm ) );
}

int testTempPw( rsComm_t *rsComm ) {
    int status;
    char pwValueToHash[500];
    status = chlMakeTempPw( rsComm, pwValueToHash, "" );
    printf( "pwValueToHash: %s\n", pwValueToHash );

    return( status );
}

int testTempPwConvert( char *s1, char *s2 ) {
    char md5Buf[100];
    unsigned char digest[RESPONSE_LEN + 2];
    char digestStr[100];

    /*
       Calcuate the temp password: a hash of s1 (the user's main
       password) and s2 (the value returned by chlGenTempPw).
    */

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, s2, sizeof md5Buf );
    strncat( md5Buf, s1, sizeof md5Buf );

    obfMakeOneWayHash( HASH_TYPE_DEFAULT, ( unsigned char* )md5Buf, sizeof md5Buf,
                       digest );

    hashToStr( digest, digestStr );
    printf( "digestStr (derived temp pw)=%s\n", digestStr );

    return( 0 );
}

int
testGetLocalZone( rsComm_t *rsComm, char *expectedZone ) {
    std::string zone;
    chlGetLocalZone( zone );
    printf( "Zone is %s\n", zone.c_str() );
    if ( zone != expectedZone ) {
        return( -1 );
    }
    return( 0 );
}

int
testGetPamPw( rsComm_t *rsComm, char *username, char *testTime ) {
    char *irodsPamPassword;

    irodsPamPassword = ( char* )malloc( 100 );
    memset( irodsPamPassword, 0, 100 );

    int status = chlUpdateIrodsPamPassword( rsComm, username, 0, testTime,
                                            &irodsPamPassword );
    if ( status == 0 ) {
        printf( "status=%d pw=%s \n", status, irodsPamPassword );
    }
    else {
        printf( "status=%d\n", status );
    }
    return( 0 );
}

int testTempPwCombined( rsComm_t *rsComm, char *s1 ) {
    int status;
    char pwValueToHash[500];
    char md5Buf[100];
    unsigned char digest[RESPONSE_LEN + 2];
    char digestStr[100];

    status = chlMakeTempPw( rsComm, pwValueToHash, "" );
    if ( status ) {
        return( status );
    }

    printf( "pwValueToHash: %s\n", pwValueToHash );

    /*
       Calcuate the temp password: a hash of s1 (the user's main
       password) and the value returned by chlGenTempPw.
    */

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, pwValueToHash, sizeof md5Buf );
    strncat( md5Buf, s1, sizeof md5Buf );

    obfMakeOneWayHash( HASH_TYPE_DEFAULT, ( unsigned char* )md5Buf, sizeof md5Buf,
                       digest );

    hashToStr( digest, digestStr );
    printf( "digestStr (derived temp pw)=%s\n", digestStr );

    return( 0 );
}

int testTempPwForOther( rsComm_t *rsComm, char *s1, char *otherUser ) {
    int status;
    char pwValueToHash[500];
    char md5Buf[100];
    unsigned char digest[RESPONSE_LEN + 2];
    char digestStr[100];

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlMakeTempPw( rsComm, pwValueToHash, otherUser );
    if ( status ) {
        return( status );
    }

    printf( "pwValueToHash: %s\n", pwValueToHash );

    /*
       Calcuate the temp password: a hash of s1 (the user's main
       password) and the value returned by chlGenTempPw.
    */

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, pwValueToHash, sizeof md5Buf );
    strncat( md5Buf, s1, sizeof md5Buf );

    obfMakeOneWayHash( HASH_TYPE_DEFAULT, ( unsigned char* )md5Buf, sizeof md5Buf,
                       digest );

    hashToStr( digest, digestStr );
    printf( "digestStr (derived temp pw)=%s\n", digestStr );

    return( 0 );
}

int testCheckAuth( rsComm_t *rsComm, char *testAdminUser,  char *testUser,
                   char *testUserZone ) {
    /* Use an pre-determined user, challenge and resp */

    char response[RESPONSE_LEN + 2];
    char challenge[CHALLENGE_LEN + 2];
    int userPrivLevel;
    int clientPrivLevel;
    int status, i;
    char userNameAndZone[NAME_LEN * 2];

    strncpy( rsComm->clientUser.userName, testUser,
             sizeof rsComm->clientUser.userName );
    strncpy( rsComm->clientUser.rodsZone, testUserZone,
             sizeof rsComm->clientUser.rodsZone );

    for ( i = 0; i < CHALLENGE_LEN + 2; i++ ) {
        challenge[i] = ' ';
    }

    i = 0;
    response[i++] = 0xd6; /* found to be a valid response */
    response[i++] = 0x8a;
    response[i++] = 0xaf;
    response[i++] = 0xc4;
    response[i++] = 0x83;
    response[i++] = 0x46;
    response[i++] = 0x1b;
    response[i++] = 0xa2;
    response[i++] = 0x5c;
    response[i++] = 0x8c;
    response[i++] = 0x6d;
    response[i++] = 0xc5;
    response[i++] = 0xb1;
    response[i++] = 0x41;
    response[i++] = 0x84;
    response[i++] = 0xeb;
    response[i++] = 0x00;

    strncpy( userNameAndZone, testAdminUser, sizeof userNameAndZone );
    userNameAndZone[ sizeof( userNameAndZone ) - 1 ] = '\0'; // JMC cppcheck - dangerous use of strncpy
    strncat( userNameAndZone, "#", sizeof userNameAndZone );
    strncat( userNameAndZone, testUserZone, sizeof userNameAndZone );

    status = chlCheckAuth( rsComm, 0, challenge, response,
                           userNameAndZone,
                           &userPrivLevel, &clientPrivLevel );

    if ( status == 0 ) {
        printf( "clientPrivLevel=%d\n", clientPrivLevel );
    }
    return( status );

}

int testDelUser( rsComm_t *rsComm, char *name, char *zone ) {
    userInfo_t userInfo;

    strncpy( userInfo.userName, name, sizeof userInfo.userName );
    strncpy( userInfo.rodsZone, zone, sizeof userInfo.rodsZone );

    /* no more  return(chlDelUser(rsComm, &userInfo)); */
    return( 0 );
}

int testDelFile( rsComm_t *rsComm, char *name, char *replica ) {
    dataObjInfo_t dataObjInfo;
    keyValPair_t *condInput;

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
    if ( replica != NULL && *replica != 0 ) {
        int ireplica;
        ireplica = atoi( replica );
        if ( ireplica >= 0 ) {
            dataObjInfo.replNum = ireplica;
        }
        if ( ireplica == 999999 ) {
            dataObjInfo.replNum = -1;
        }
    }
    strncpy( dataObjInfo.objPath, name, sizeof dataObjInfo.objPath );

    memset( &condInput, 0, sizeof( condInput ) );

    return( chlUnregDataObj( rsComm, &dataObjInfo, condInput ) );
}

int testDelFilePriv( rsComm_t *rsComm, char *name, char *dataId,
                     char *replica ) {
    dataObjInfo_t dataObjInfo;
    keyValPair_t condInput;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    memset( &condInput, 0, sizeof( condInput ) );
    addKeyVal( &condInput, ADMIN_KW, " " );

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );

    if ( dataId != NULL && *dataId != 0 ) {
        rodsLong_t idataId;
        idataId = strtoll( dataId, NULL, 0 );
        if ( idataId >= 0 ) {
            dataObjInfo.dataId = idataId;
        }
    }
    dataObjInfo.replNum = -1;
    if ( replica != NULL && *replica != 0 ) {
        int ireplica;
        ireplica = atoi( replica );
        if ( ireplica >= 0 ) {
            dataObjInfo.replNum = ireplica;
        }
    }
    strncpy( dataObjInfo.objPath, name, sizeof dataObjInfo.objPath );

    return( chlUnregDataObj( rsComm, &dataObjInfo, &condInput ) );
}

int testDelFileTrash( rsComm_t *rsComm, char *name, char *dataId ) {
    dataObjInfo_t dataObjInfo;
    keyValPair_t condInput;

    char *replica = 0;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    memset( &condInput, 0, sizeof( condInput ) );
    addKeyVal( &condInput, ADMIN_RMTRASH_KW, " " );

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );

    if ( dataId != NULL && *dataId != 0 ) {
        rodsLong_t idataId;
        idataId = strtoll( dataId, NULL, 0 );
        if ( idataId >= 0 ) {
            dataObjInfo.dataId = idataId;
        }
    }
    dataObjInfo.replNum = -1;
    if ( replica != NULL && *replica != 0 ) {
        int ireplica;
        ireplica = atoi( replica );
        if ( ireplica >= 0 ) {
            dataObjInfo.replNum = ireplica;
        }
    }
    strncpy( dataObjInfo.objPath, name, sizeof dataObjInfo.objPath );

    return( chlUnregDataObj( rsComm, &dataObjInfo, &condInput ) );
}

int testRegColl( rsComm_t *rsComm, char *name ) {

    collInfo_t collInp;

    strncpy( collInp.collName, name, sizeof collInp.collName );

    return( chlRegColl( rsComm, &collInp ) );
}

int testDelColl( rsComm_t *rsComm, char *name ) {

    collInfo_t collInp;

    strncpy( collInp.collName, name, sizeof collInp.collName );

    return( chlDelColl( rsComm, &collInp ) );
}

int testDelRule( rsComm_t *rsComm, char *ruleName, char *userName ) {
    if ( userName != NULL && strlen( userName ) > 0 ) {
        rsComm->clientUser.authInfo.authFlag = LOCAL_USER_AUTH;
        rsComm->proxyUser.authInfo.authFlag = LOCAL_USER_AUTH;
        strncpy( rsComm->clientUser.userName, userName,
                 sizeof rsComm->clientUser.userName );
    }
    else {
        rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
        rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    }
    return( chlDelRuleExec( rsComm, ruleName ) );
}

int testRegDataObj( rsComm_t *rsComm, char *name,
                    char *dataType, char *filePath ) {
    dataObjInfo_t dataObjInfo;
    memset( &dataObjInfo, 0, sizeof( dataObjInfo_t ) );

    strcpy( dataObjInfo.objPath, name );
    dataObjInfo.replNum = 1;
    strcpy( dataObjInfo.version, "12" );
    strcpy( dataObjInfo.dataType, dataType );
    dataObjInfo.dataSize = 42;

    strcpy( dataObjInfo.rescName, "demoResc" );
    strcpy( dataObjInfo.filePath, filePath );

    dataObjInfo.replStatus = 5;

    return ( chlRegDataObj( rsComm, &dataObjInfo ) );
}

/*
  Do multiple data registrations.  If you comment out the commit in
  chlRegDataObj and then build this, it can add phony data-objects at
  about 8 times the speed of lots of iput's of small files.  This can
  come in handy for creating simulated large instances for DBMS
  performance testing and tuning.  In this source file, you might also
  want to change rodsLogLevel(LOG_NOTICE) to rodsLogLevel(LOG_ERROR)
  and comment out rodsLogSqlReq(1);.

  name is the objPath (collection/dataObj)
  objPath = /newZone/home/rods/ws/f3"
  filePath is the physical path
  filePath = "/home/schroeder/iRODS/Vault/home/rods/ws/f3"

  Example:
  bin/test_chl regmulti 1000 /newZone/home/rods/ws2/f1 generic /tmp/vault/f1
*/
int testRegDataMulti( rsComm_t *rsComm, char *count,
                      char *nameBase,  char *dataType, char *filePath ) {
    int status;
    int myCount = 100;
    int i;
    char myName[MAX_NAME_LEN];

    myCount = atoi( count );
    if ( myCount <= 0 ) {
        printf( "Invalid input: count\n" );
        return( USER_INPUT_OPTION_ERR );
    }

    for ( i = 0; i < myCount; i++ ) {
        snprintf( myName, sizeof myName, "%s.%d", nameBase, i );
        status = testRegDataObj( rsComm, myName, dataType, filePath );
        if ( status ) {
            return( status );
        }
    }

    status = chlCommit( rsComm );
    return( status );
}

int testModDataObjMeta( rsComm_t *rsComm, char *name,
                        char *dataType, char *filePath ) {
    dataObjInfo_t dataObjInfo;
    int status;
    keyValPair_t regParam;
    char tmpStr[LONG_NAME_LEN], tmpStr2[LONG_NAME_LEN];
    /*   int replStatus; */

    memset( &dataObjInfo, 0, sizeof( dataObjInfo_t ) );

    memset( &regParam, 0, sizeof( regParam ) );

    /*
      replStatus=1;
      snprintf (tmpStr, LONG_NAME_LEN, "%d", replStatus);
      addKeyVal (&regParam, "replStatus", tmpStr);
    */
    snprintf( tmpStr, sizeof tmpStr, "fake timestamp" );
    addKeyVal( &regParam, "dataCreate", tmpStr );

    snprintf( tmpStr2, sizeof tmpStr2, "test comment" );
    addKeyVal( &regParam, "dataComments", tmpStr2 );

    strcpy( dataObjInfo.objPath, name );
    /*   dataObjInfo.replNum=1; */
    dataObjInfo.replNum = 0;

    strcpy( dataObjInfo.version, "12" );
    strcpy( dataObjInfo.dataType, dataType );
    dataObjInfo.dataSize = 42;

    strcpy( dataObjInfo.rescName, "resc A" );

    strcpy( dataObjInfo.filePath, filePath );

    dataObjInfo.replStatus = 5;

    status = chlModDataObjMeta( rsComm, &dataObjInfo, &regParam );


    return( status );
}

int testModDataObjMeta2( rsComm_t *rsComm, char *name,
                         char *dataType, char *filePath ) {
    dataObjInfo_t dataObjInfo;
    int status;
    keyValPair_t regParam;
    char tmpStr[LONG_NAME_LEN], tmpStr2[LONG_NAME_LEN];

    memset( &dataObjInfo, 0, sizeof( dataObjInfo_t ) );

    memset( &regParam, 0, sizeof( regParam ) );

    snprintf( tmpStr, sizeof tmpStr, "whatever" );
    addKeyVal( &regParam, "all", tmpStr );

    snprintf( tmpStr2, sizeof tmpStr2, "42" );
    addKeyVal( &regParam, "dataSize", tmpStr2 );

    strcpy( dataObjInfo.objPath, name );
    dataObjInfo.replNum = 0;
    strcpy( dataObjInfo.version, "12" );
    strcpy( dataObjInfo.dataType, dataType );
    dataObjInfo.dataSize = 42;

    strcpy( dataObjInfo.rescName, "resc A" );

    strcpy( dataObjInfo.filePath, filePath );

    dataObjInfo.replStatus = 5;

    status = chlModDataObjMeta( rsComm, &dataObjInfo, &regParam );


    return( status );
}

int testModColl( rsComm_t *rsComm, char *name, char *type,
                 char *info1, char *info2 ) {
    int status;
    collInfo_t collInp;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    memset( &collInp, 0, sizeof( collInp ) );

    if ( name != NULL && strlen( name ) > 0 ) {
        strncpy( collInp.collName, name, sizeof collInp.collName );
    }
    if ( type != NULL && strlen( type ) > 0 ) {
        strncpy( collInp.collType, type, sizeof collInp.collType );
    }
    if ( info1 != NULL && strlen( info1 ) > 0 ) {
        strncpy( collInp.collInfo1, info1, sizeof collInp.collInfo1 );
    }
    if ( info2 != NULL && strlen( info2 ) > 0 ) {
        strncpy( collInp.collInfo2, info2, sizeof collInp.collInfo2 );
    }

    status = chlModColl( rsComm, &collInp );

    if ( status != 0 ) {
        return( status );
    }

    status = chlCommit( rsComm );
    return( status );
}

int testModRuleMeta( rsComm_t *rsComm, char *id,
                     char *attrName, char *attrValue ) {
    /*   ruleExecSubmitInp_t ruleInfo; */
    char ruleId[100];
    int status;
    keyValPair_t regParam;
    char tmpStr[LONG_NAME_LEN];

    /*   memset(&ruleInfo,0,sizeof(ruleExecSubmitInp_t)); */

    memset( &regParam, 0, sizeof( regParam ) );

    rstrcpy( tmpStr, attrValue, sizeof tmpStr );

    addKeyVal( &regParam, attrName, tmpStr );

    strcpy( ruleId, id );

    status = chlModRuleExec( rsComm, ruleId, &regParam );

    return( status );
}

int testModResourceFreeSpace( rsComm_t *rsComm, char *rescName,
                              char *numberString, char *option ) {
    int number, status;
    if ( *numberString == '\\' ) {
        numberString++;
    }
    number = atoi( numberString );
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    status = chlModRescFreeSpace( rsComm, rescName, number );
    if ( status != 0 ) {
        return( status );
    }
    if ( option != NULL && strcmp( option, "rollback" ) == 0 ) {
        status = chlRollback( rsComm );
    }
    if ( option != NULL && strcmp( option, "close" ) == 0 ) {
        status = chlClose();
        return( status );
    }
    status = chlCommit( rsComm );
    return( status );
}

int testRegReplica( rsComm_t *rsComm, char *srcPath, char *srcDataId,
                    char *srcReplNum, char *dstPath ) {
    dataObjInfo_t srcDataObjInfo;
    dataObjInfo_t dstDataObjInfo;
    keyValPair_t condInput;
    int status;

    memset( &srcDataObjInfo, 0, sizeof( dataObjInfo_t ) );
    memset( &dstDataObjInfo, 0, sizeof( dataObjInfo_t ) );
    memset( &condInput, 0, sizeof( condInput ) );

    strcpy( srcDataObjInfo.objPath, srcPath );
    srcDataObjInfo.dataId = atoi( srcDataId );
    srcDataObjInfo.replNum = atoi( srcReplNum );


    strcpy( dstDataObjInfo.rescName, "resc A" );
    strcpy( dstDataObjInfo.rescGroupName, "resc A" );
    strcpy( dstDataObjInfo.filePath, dstPath );

    dstDataObjInfo.replStatus = 5;

    status = chlRegReplica( rsComm, &srcDataObjInfo, &dstDataObjInfo,
                            &condInput );
    return( status );
}

int testSimpleQ( rsComm_t *rsComm, char *sql, char *arg1, char *format ) {
    char bigBuf[1000];
    int status;
    int control;
    int form;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    control = 0;
    form = 1;
    if ( format != NULL ) {
        form = atoi( format );
    }

    status = chlSimpleQuery( rsComm, sql, arg1, 0, 0, 0,
                             form, &control, bigBuf, 1000 );
    if ( status == 0 ) {
        printf( "%s", bigBuf );
    }

    while ( control && ( status == 0 ) ) {
        status = chlSimpleQuery( rsComm, sql, 0, 0, 0, 0,
                                 form, &control, bigBuf, 1000 );
        if ( status == 0 ) {
            printf( "%s", bigBuf );
        }
    }
    return( status );
}

int testChmod( rsComm_t *rsComm, char *user, char *zone,
               char *access, char *path ) {
    int status;
    status = chlModAccessControl( rsComm, 0, user, zone, access, path );

    return( status );
}

int testServerLoad( rsComm_t *rsComm, char *option ) {
    int status;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlRegServerLoad( rsComm, "host", "resc", option, "2", "3",
                               "4", "5", "6", "7" );
    return( status );
}

int testPurgeServerLoad( rsComm_t *rsComm, char *option ) {
    int status;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    if ( option == NULL ) {
        status = chlPurgeServerLoad( rsComm, "2000" );
    }
    else {
        status = chlPurgeServerLoad( rsComm, option );
    }

    return( status );
}

int testServerLoadDigest( rsComm_t *rsComm, char *option ) {
    int status;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlRegServerLoadDigest( rsComm, "resc", option );
    return( status );
}

int testPurgeServerLoadDigest( rsComm_t *rsComm, char *option ) {
    int status;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    if ( option == NULL ) {
        status = chlPurgeServerLoadDigest( rsComm, "2000" );
    }
    else {
        status = chlPurgeServerLoadDigest( rsComm, option );
    }

    return( status );
}

int testCheckQuota( rsComm_t *rsComm, char *userName, char *rescName,
                    char *expectedQuota, char *expectedStatus ) {
    int status;
    int quotaStatus;
    rodsLong_t userQuota;

    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlCheckQuota( rsComm, userName, rescName,
                            &userQuota, &quotaStatus );

    rodsLog( LOG_SQL,
             "chlCheckQuota status: userName:%s rescName:%s userQuota:%lld quotaStatus:%d\n",
             userName, rescName, userQuota, quotaStatus );

    if ( status == 0 ) {
        int iExpectedStatus;
        rodsLong_t iExpectedQuota;
        if ( expectedQuota != NULL && strlen( expectedQuota ) > 0 ) {
            rodsLong_t i;
            iExpectedQuota = atoll( expectedQuota );
            if ( expectedQuota[0] == 'm' ) {
                i = atoll( ( char * )&expectedQuota[1] );
                iExpectedQuota = -i;
            }
            if ( iExpectedQuota != userQuota ) {
                status = -1;
            }
        }
        if ( expectedStatus != NULL && strlen( expectedStatus ) > 0 ) {
            iExpectedStatus = atoi( expectedStatus );
            if ( iExpectedStatus != quotaStatus ) {
                status = -2;
            }
        }
    }
    return( status );
}

rodsLong_t
testCurrent( rsComm_t *rsComm ) {
    rodsLong_t status = 0;
    icatSessionStruct *icss;

    chlGetRcs( &icss );

// JMC    status = cmlGetCurrentSeqVal( icss );
    return( status );
}

int
testAddRule( rsComm_t *rsComm, char *baseName, char *ruleName,
             char *ruleHead, char *ruleCondition, char *ruleAction,
             char *ruleRecovery ) {
    int status;
    char ruleIdStr[200];
    char myTime[] = "01277237323";
    char priority[] = "1";
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlInsRuleTable( rsComm, baseName, priority, ruleName,
                              ruleHead, ruleCondition, ruleAction,
                              ruleRecovery, ( char * )&ruleIdStr, ( char * )&myTime );

    if ( status == 0 ) {
        printf( "ruleIdStr: %s\n", ruleIdStr );
    }
    return( status );
}

int
testVersionRuleBase( rsComm_t *rsComm, char *baseName ) {
    int status;
    char myTime[] = "01277237323";
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlVersionRuleBase( rsComm, baseName, ( char * )&myTime );

    return( status );
}

int
testVersionDvmBase( rsComm_t *rsComm, char *baseName ) {
    int status;
    char myTime[] = "01277237323";
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlVersionDvmBase( rsComm, baseName, ( char * )&myTime );

    return( status );
}

int
testInsFnmTable( rsComm_t *rsComm, char *arg1, char *arg2, char *arg3,
                 char *arg4 ) {
    int status;
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlInsFnmTable( rsComm, arg1, arg2, arg3, arg4 );

    return( status );
}

int
testInsMsrvcTable( rsComm_t *rsComm, char *arg1, char *arg2, char *arg3,
                   char *arg4, char *arg5, char *arg6, char *arg7, char *arg8,
                   char *arg9 ) {
    int status;
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlInsMsrvcTable( rsComm, arg1, arg2, arg3, arg4,
                               arg5, arg6, arg7, arg8, arg9, "0" );

    return( status );
}

int
testInsDvmTable( rsComm_t *rsComm,  char *arg1, char *arg2, char *arg3,
                 char *arg4 ) {
    int status;
    char myTime[] = "01277237323";
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlInsDvmTable( rsComm, arg1, arg2, arg3, arg4, myTime );
    return( status );
}

int
testVersionFnmBase( rsComm_t *rsComm, char *arg1 ) {
    int status;
    char myTime[] = "01277237323";
    rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    status = chlVersionFnmBase( rsComm, arg1, myTime );

    return( status );
}


int
main( int argc, char **argv ) {
    int status;
    rsComm_t *Comm;
    /*   rErrMsg_t errMsg;*/
    rodsArguments_t myRodsArgs;
    char *mySubName;
    char *myName;
    int didOne;


    Comm = ( rsComm_t* )malloc( sizeof( rsComm_t ) );
    memset( Comm, 0, sizeof( rsComm_t ) );

    parseCmdLineOpt( argc, argv, "", 0, &myRodsArgs );

    rodsLogLevel( LOG_NOTICE );

    rodsLogSqlReq( 1 );

    if ( argc < 2 ) {
        printf( "Usage: test_chl testName [args...]\n" );
        exit( 3 );
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    if ( strstr( myEnv.rodsDebug, "CAT" ) != NULL ) {
        chlDebug( myEnv.rodsDebug );
    }


    strncpy( Comm->clientUser.userName, myEnv.rodsUserName,
             sizeof Comm->clientUser.userName );

    strncpy( Comm->clientUser.rodsZone, myEnv.rodsZone,
             sizeof Comm->clientUser.rodsZone );

    /*
      char rodsUserName[NAME_LEN];
      char rodsZone[NAME_LEN];

      userInfo_t clientUser;
      char userName[NAME_LEN];
      char rodsZone[NAME_LEN];
    */


	// capture server properties
	irods::server_properties::getInstance().capture();

    if ( ( status = chlOpen() ) != 0 ) {

        rodsLog( LOG_SYS_FATAL,
                 "initInfoWithRcat: chlopen Error. Status = %d",
                 status );
        free( Comm ); // JMC cppcheck - leak
        return ( status );
    }

    didOne = 0;
    if ( strcmp( argv[1], "reg" ) == 0 ) {
        status = testRegDataObj( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }
    if ( strcmp( argv[1], "regmulti" ) == 0 ) {
        status = testRegDataMulti( Comm, argv[2], argv[3], argv[4], argv[5] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "mod" ) == 0 ) {
        status = testModDataObjMeta( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "mod2" ) == 0 ) {
        status = testModDataObjMeta2( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "modr" ) == 0 ) {
        status = testModRuleMeta( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "modc" ) == 0 ) {
        status = testModColl( Comm, argv[2], argv[3], argv[4], argv[5] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "rmrule" ) == 0 ) {
        status = testDelRule( Comm, argv[2], argv[3] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "modrfs" ) == 0 ) {
        status = testModResourceFreeSpace( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "rep" ) == 0 ) {
        if ( argc < 6 ) {
            printf( "too few arguments\n" );
            exit( 1 );
        }
        status = testRegReplica( Comm, argv[2], argv[3], argv[4], argv[5] );
        didOne = 1;
    }

    /*
      if (strcmp(argv[1],"cml")==0) {
      status = testCml(Comm);
      didOne=1;
      }
    */

    if ( strcmp( argv[1], "rmuser" ) == 0 ) {
        status = testDelUser( Comm, argv[2], argv[3] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "mkdir" ) == 0 ) {
        status = testRegColl( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "rmdir" ) == 0 ) {
        status = testDelColl( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "sql" ) == 0 ) {
        status = testSimpleQ( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "rm" ) == 0 ) {
        status = testDelFile( Comm, argv[2], argv[3] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "rmtrash" ) == 0 ) {
        status = testDelFileTrash( Comm, argv[2], argv[3] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "rmpriv" ) == 0 ) {
        status = testDelFilePriv( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "chmod" ) == 0 ) {
        status = testChmod( Comm, argv[2], argv[3], argv[4], argv[5] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "regrule" ) == 0 ) {
        status = testRegRule( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "rename" ) == 0 ) {
        status = testRename( Comm, argv[2], argv[3] );
        // JMC testCurrent( Comm );  // exercise this as part of rename;
        // testCurrent needs a SQL context
        didOne = 1;
    }

    if ( strcmp( argv[1], "login" ) == 0 ) {
        status = testLogin( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "move" ) == 0 ) {
        status = testMove( Comm, argv[2], argv[3] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "checkauth" ) == 0 ) {
        status = testCheckAuth( Comm, argv[2], argv[3], argv[4] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "temppw" ) == 0 ) {
        status = testTempPw( Comm );
        didOne = 1;
    }

    if ( strcmp( argv[1], "tpc" ) == 0 ) {
        status = testTempPwConvert( argv[2], argv[3] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "tpw" ) == 0 ) {
        status = testTempPwCombined( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "tpwforother" ) == 0 ) {
        status = testTempPwForOther( Comm, argv[2], argv[3] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "serverload" ) == 0 ) {
        status = testServerLoad( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "purgeload" ) == 0 ) {
        status = testPurgeServerLoad( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "serverdigest" ) == 0 ) {
        status = testServerLoadDigest( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "purgedigest" ) == 0 ) {
        status = testPurgeServerLoadDigest( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "checkquota" ) == 0 ) {
        if ( argc < 5 ) {
            status = testCheckQuota( Comm, argv[2], argv[3],
                                     NULL, NULL );
        }
        else {
            status = testCheckQuota( Comm, argv[2], argv[3],
                                     argv[4], argv[5] );
        }
        didOne = 1;
    }

    if ( strcmp( argv[1], "open" ) == 0 ) {
        int i;
        for ( i = 0; i < 3; i++ ) {
            status = chlClose();
            if ( status ) {
                printf( "close %d error", i );
            }

            if ( ( status = chlOpen() ) != 0 ) {

                rodsLog( LOG_SYS_FATAL,
                         "initInfoWithRcat: chlopen %d Error. Status = %d",
                         i, status );
                return ( status );
            }
        }
        didOne = 1;
    }

    if ( strcmp( argv[1], "addrule" ) == 0 ) {
        status = testAddRule( Comm, argv[2], argv[3],
                              argv[4], argv[5],
                              argv[6], argv[7] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "versionrulebase" ) == 0 ) {
        status = testVersionRuleBase( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "versiondvmbase" ) == 0 ) {
        status = testVersionDvmBase( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "versionfnmbase" ) == 0 ) {
        status = testVersionFnmBase( Comm, argv[2] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "insfnmtable" ) == 0 ) {
        status = testInsFnmTable( Comm, argv[2], argv[3], argv[4], argv[5] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "insdvmtable" ) == 0 ) {
        status = testInsDvmTable( Comm, argv[2], argv[3], argv[4], argv[5] );
        didOne = 1;
    }

    if ( strcmp( argv[1], "insmsrvctable" ) == 0 ) {
        status = testInsMsrvcTable( Comm, argv[2], argv[3], argv[4], argv[5],
                                    argv[6], argv[7], argv[8], argv[9], argv[10] );
        if ( status == 0 ) {
            /* do it a second time to test another logic path and
               different SQL.  Since no commit is part of the chl
               function, and there is not corresponding Delete call, this
               is an easy way to do this. */
            status = testInsMsrvcTable( Comm, argv[2], argv[3], argv[4], argv[5],
                                        argv[6], argv[7], argv[8], argv[9], argv[10] );
        }
        didOne = 1;
    }
    if ( strcmp( argv[1], "getlocalzone" ) == 0 ) {
        status = testGetLocalZone( Comm, argv[2] );
        didOne = 1;
    }
    if ( strcmp( argv[1], "getpampw" ) == 0 ) {
        status = testGetPamPw( Comm, argv[2], argv[3] );
        didOne = 1;
    }
    if ( status != 0 ) {
        /*
          if (Comm->rError) {
          rError_t *Err;
          rErrMsg_t *ErrMsg;
          int i, len;
          Err = Comm->rError;
          len = Err->len;
          for (i=0;i<len;i++) {
          ErrMsg = Err->errMsg[i];
          rodsLog(LOG_ERROR, "Level %d: %s",i, ErrMsg->msg);
          }
          }
        */
        myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "%s failed with error %d %s %s", argv[1],
                 status, myName, mySubName );
    }
    else {
        if ( didOne ) {
            printf( "Completed successfully\n" );
        }
    }

    if ( didOne == 0 ) {
        printf( "Unknown test type: %s\n", argv[1] );
    }

    exit( status );
}

/* This is a dummy version of icatApplyRule for this test program so
-   the rule-engine is not needed in this ICAT test. */
int
icatApplyRule( rsComm_t *rsComm, char *ruleName, char *arg1 ) {
    return( 0 );
}
