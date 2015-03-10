/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "rodsClient.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

void usage();

/*
 print the results of a general query.
 */
int
printGenQueryResults( rcComm_t *Conn, int status, genQueryOut_t *genQueryOut,
                      char *descriptions[], int doDashes ) {
    int printCount;
    int i, j;
    char localTime[TIME_LEN];
    printCount = 0;
    if ( status != 0 ) {
        printError( Conn, status, "rcGenQuery" );
    }
    else {
        if ( status != CAT_NO_ROWS_FOUND ) {
            for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
                if ( i > 0 && doDashes ) {
                    printf( "----\n" );
                }
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( descriptions != 0 && *descriptions[j] != '\0' ) {
                        if ( strstr( descriptions[j], "_ts" ) != 0 ) {
                            getLocalTimeFromRodsTime( tResult, localTime );
                            if ( atoll( tResult ) == 0 ) {
                                rstrcpy( localTime, "None", 20 );
                            }
                            printf( "%s: %s: %s\n", descriptions[j], tResult,
                                    localTime );
                        }
                        else {
                            printf( "%s: %s\n", descriptions[j], tResult );
                        }
                    }
                    else {
                        printf( "%s\n", tResult );
                    }
                    printCount++;
                }
            }
        }
    }
    return printCount;
}


/* perform the ls command */
int
doLs( rcComm_t *Conn, char *objPath, int longOption ) {

    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[30];
    int i1b[30] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i2a[30];
    char *condVal[10];
    char v1[MAX_NAME_LEN + 10];
    char v2[MAX_NAME_LEN + 10];
    int i, status;
    int printCount;

    char *columnNames[] = {
        "data_name",
        "data_id", "coll_id", "data_repl_num", "data_version",
        "data_type_name", "data_size", "resc_group_name", "resc_name",
        "data_path ", "data_owner_name", "data_owner_zone",
        "data_repl_status", "data_status",
        "data_checksum ", "data_expiry_ts (expire time)",
        "data_map_id", "r_comment", "create_ts", "modify_ts"
    };

    char logicalEndName[MAX_NAME_LEN];
    char logicalParentDirName[MAX_NAME_LEN];

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    printCount = 0;

    printf( "doing ls of %s\n", objPath );

    status = splitPathByKey( objPath,
                             logicalParentDirName, MAX_NAME_LEN, logicalEndName, MAX_NAME_LEN, '/' );

    i = 0;
    i1a[i++] = COL_DATA_NAME;
    if ( longOption ) {
        i1a[i++] = COL_D_DATA_ID;
        i1a[i++] = COL_D_COLL_ID;
        i1a[i++] = COL_DATA_REPL_NUM;
        i1a[i++] = COL_DATA_VERSION;
        i1a[i++] = COL_DATA_TYPE_NAME;
        i1a[i++] = COL_DATA_SIZE;
        i1a[i++] = COL_D_RESC_NAME;
        i1a[i++] = COL_D_DATA_PATH;
        i1a[i++] = COL_D_OWNER_NAME;
        i1a[i++] = COL_D_OWNER_ZONE;
        i1a[i++] = COL_D_REPL_STATUS;
        i1a[i++] = COL_D_DATA_STATUS;
        i1a[i++] = COL_D_DATA_CHECKSUM;
        i1a[i++] = COL_D_EXPIRY;
        i1a[i++] = COL_D_MAP_ID;
        i1a[i++] = COL_D_COMMENTS;
        i1a[i++] = COL_D_CREATE_TIME;
        i1a[i++] = COL_D_MODIFY_TIME;
    }
    else {
        columnNames[1] = "data_expiry_ts (expire time)";
        i1a[i++] = COL_D_EXPIRY;
    }

    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = i;

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    i2a[0] = COL_COLL_NAME;
    sprintf( v1, "='%s'", logicalParentDirName );
    condVal[0] = v1;
    i2a[1] = COL_DATA_NAME;
    sprintf( v2, "='%s'", logicalEndName );
    condVal[1] = v2;
    genQueryInp.sqlCondInp.len = 2;

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_R_RESC_INFO;
        genQueryInp.selectInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            printf( "None\n" );
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            printf( "DataObject %s does not exist.\n", objPath );
            return 0;
        }
    }

    printCount += printGenQueryResults( Conn, status, genQueryOut, columnNames,
                                        longOption );
    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 && longOption ) {
            printf( "----\n" );
        }
        printCount += printGenQueryResults( Conn, status, genQueryOut,
                                            columnNames, longOption );
    }

    return 1;
}

/* perform the list data-types command */
int
doListDataTypes( rcComm_t *Conn ) {

    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int select_inx[3];
    int select_value[3] = {0, 0, 0};
    int condition_inx[3];
    char *condition_value[10];
    char value1[MAX_NAME_LEN + 10] = "='data_type'";
    int i, status;
    int printCount;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    printCount = 0;

    printf( "Defined data-types:\n" );

    i = 0;
    select_inx[i++] = COL_TOKEN_NAME;

    genQueryInp.selectInp.inx = select_inx;
    genQueryInp.selectInp.value = select_value;
    genQueryInp.selectInp.len = i;

    genQueryInp.sqlCondInp.inx = condition_inx;
    genQueryInp.sqlCondInp.value = condition_value;
    condition_inx[0] = COL_TOKEN_NAMESPACE;
    condition_value[0] = value1;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 50;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        printf( "None exist.\n" );
        return 0;
    }

    printCount += printGenQueryResults( Conn, status, genQueryOut, 0,
                                        0 );
    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        printCount += printGenQueryResults( Conn, status, genQueryOut,
                                            0, 0 );
    }

    return 1;
}


/* perform the modify command */
int
doMod( rcComm_t *Conn, char *objPath, char *time ) {
    int status;

    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    dataObjInfo_t dataObjInfo;

    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, DATA_EXPIRY_KW, time );

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
    rstrcpy( dataObjInfo.objPath, objPath, MAX_NAME_LEN );

    modDataObjMetaInp.regParam = &regParam;
    modDataObjMetaInp.dataObjInfo = &dataObjInfo;

    status = rcModDataObjMeta( Conn, &modDataObjMetaInp );
    if ( status ) {
        rodsLogError( LOG_ERROR, status, "rcModDataObjMeta failure" );
    }
    return status;
}

/* perform the modify command to change the data_type*/
int
doModDatatype( rcComm_t *Conn, char *objPath, char *dataType ) {
    int status;

    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    dataObjInfo_t dataObjInfo;

    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, DATA_TYPE_KW, dataType );

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
    rstrcpy( dataObjInfo.objPath, objPath, MAX_NAME_LEN );

    modDataObjMetaInp.regParam = &regParam;
    modDataObjMetaInp.dataObjInfo = &dataObjInfo;

    status = rcModDataObjMeta( Conn, &modDataObjMetaInp );
    if ( status ) {
        rodsLogError( LOG_ERROR, status, "rcModDataObjMeta failure" );
    }
    return status;
}

/* perform the modify command to change the comment*/
int
doModComment( rcComm_t *Conn, char *objPath, int numRepl, char *theComment ) {
    int status;

    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    dataObjInfo_t dataObjInfo;

    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, DATA_COMMENTS_KW, theComment );

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
    rstrcpy( dataObjInfo.objPath, objPath, MAX_NAME_LEN );

    if ( numRepl > 0 ) {
        dataObjInfo.replNum = numRepl;
    }
    else {
        addKeyVal( &regParam, ALL_KW, theComment );
    }

    modDataObjMetaInp.regParam = &regParam;
    modDataObjMetaInp.dataObjInfo = &dataObjInfo;

    status = rcModDataObjMeta( Conn, &modDataObjMetaInp );
    if ( status ) {
        rodsLogError( LOG_ERROR, status, "rcModDataObjMeta failure" );
    }
    return status;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;
    int i, j, didOne;
    char objPath[MAX_NAME_LEN];
    int maxCmdTokens = 20;
    char *cmdToken[20];
    int argOffset;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    char *mySubName;
    char *myName;

    status = parseCmdLineOpt( argc, argv, "lvVh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }

    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    argOffset = myRodsArgs.optind;

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }
    snprintf( objPath, sizeof( objPath ), "%s", myEnv.rodsCwd );

    for ( i = 0; i < maxCmdTokens; i++ ) {
        cmdToken[i] = "";
    }
    j = 0;
    for ( i = argOffset; i < argc; i++ ) {
        cmdToken[j++] = argv[i];
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        myName = rodsErrorName( errMsg.status, &mySubName );
        rodsLog( LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
                 myName,
                 mySubName,
                 errMsg.status,
                 errMsg.msg );
        exit( 2 );
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        rcDisconnect( Conn );
        exit( 3 );
    }

    if ( strcmp( cmdToken[0], "ldt" ) == 0 ) {
        if ( *cmdToken[1] == '\0' ) {
            cmdToken[1] = " "; /* just for subsequent checks below */
        }
    }

    if ( *cmdToken[1] == '\0' ) {
        usage();
        exit( 1 );
    }

    if ( *cmdToken[1] == '/' ) {
        rstrcpy( objPath, cmdToken[1], MAX_NAME_LEN );
    }
    else {
        rstrcat( objPath, "/", MAX_NAME_LEN );
        rstrcat( objPath, cmdToken[1], MAX_NAME_LEN );
    }

    didOne = 0;
    if ( strcmp( cmdToken[0], "ls" ) == 0 ) {
        doLs( Conn, objPath, myRodsArgs.longOption || myRodsArgs.verbose );
        didOne = 1;
    }
    if ( strcmp( cmdToken[0], "ldt" ) == 0 ) {
        doListDataTypes( Conn );
        didOne = 1;
    }

    if ( strcmp( cmdToken[0], "mod" ) == 0 ) {
        char theTime[TIME_LEN + 20];

        if ( *cmdToken[2] == '\0' ) {
            usage();
            exit( 1 );
        }
        if ( strcmp( cmdToken[2], "datatype" ) == 0 ) {
            if ( *cmdToken[3] == '\0' ) {
                usage();
                exit( 1 );
            }
            status = doModDatatype( Conn, objPath, cmdToken[3] );
            didOne = 1;
        }
        else if ( strcmp( cmdToken[2], "comment" ) == 0 ) {
            if ( *cmdToken[3] == '\0' ) {
                usage();
                exit( 1 );
            }
            if ( *cmdToken[4] == '\0' ) {
                status = doModComment( Conn, objPath, -1, cmdToken[3] );
            }
            else {
                status = doModComment( Conn, objPath, atoi( cmdToken[3] ), cmdToken[4] );
            }

            didOne = 1;
        }
        else {
            rstrcpy( theTime, cmdToken[2], TIME_LEN );
            status = 0;
            if ( *cmdToken[2] == '+' ) {
                rstrcpy( theTime, cmdToken[2] + 1, TIME_LEN ); /* skip the + */
                status = checkDateFormat( theTime );    /* check and convert the time value */
                getOffsetTimeStr( theTime, theTime );   /* convert delta format to now + this*/
            }
            else {
                status = checkDateFormat( theTime );    /* check and convert the time value */
            }
            if ( status == 0 ) {
                status = doMod( Conn, objPath, theTime );
                didOne = 1;
            }
            else {
                printf( "Invalid time format input\n" );
            }
        }
    }

    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    if ( didOne == 0 ) {
        usage();
        exit( 1 );
    }

    exit( 0 );
}

void
printMsgs( char *msgs[] ) {
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            return;
        }
        printf( "%s\n", msgs[i] );
    }
}

void
usage() {
    char *Msgs[] = {
        "Usage: isysmeta [-lvVh] [command]",
        " Show or modify system metadata.",
        "Commands are:",
        " mod DataObjectName Time (modify expire time)",
        " mod DataObjectName datatype Type (modify data-type)",
        " mod DataObjectName comment [replNum] Comment (modify the comment of the replica replNum or 0 by default)",
        " ls [-lvV] Name (list dataObject, -l -v for long form)",
        " ldt (list data types (those available))",
        " ",
        "Time can be full or partial date/time: '2009-12-01' or '2009-12-11.12:03'",
        "etc, or a delta time '+1h' (one hour from now), etc.",
        " ",
        "Examples:",
        " isysmeta mod foo +1h (set the expire time for file 'foo' to an hour from now)",
        " isysmeta mod /tempZone/home/rods/foo 2009-12-01",
        " isysmeta mod /tempZone/home/rods/foo datatype 'tar file'",
        " isysmeta mod /tempZone/home/rods/foo comment 1 'my comment'",
        " isysmeta ls foo",
        " isysmeta -l ls foo",
        " isysmeta ls -l foo",
        " isysmeta ldt",
        ""
    };

    printMsgs( Msgs );
    printReleaseInfo( "isysmeta" );
}
