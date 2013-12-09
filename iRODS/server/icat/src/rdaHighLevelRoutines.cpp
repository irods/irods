/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**************************************************************************

  This file contains the RDA (Rule-oriented Database Access) high
  level functions.  These constitute the API between the RDA
  microservices (msiRda*.c) and the database.

  In a very general sense, the RDA provides something like the SRB DAI
  (Database Access Interface); it provides an interface to another
  database via calls that are part of the environment.  However, the
  RDA utilizes the iRODS rule engine and microservice infrastructure
  to create a simple, yet very flexible, powerful, and secure
  interface.  For more information, see the iRODS web site.

  These routines, like the icatHighLevelRoutines, layer on top of
  either icatLowLevelOdbc or icatLowLevelOracle.  RDA is not ICAT,
  but they both use the shared low level interface to the databases.
  RDA can be built as part of either an ICAT-enabled server or a
  non-ICAT-Enabled server.  In the later case, the Makefiles build the
  low level routines ICAT functions for use by RDA, even tho the other
  ICAT layers are not.


**************************************************************************/

#include "rods.hpp"
#include "rcMisc.hpp"

#include "rdaHighLevelRoutines.hpp"
#include "icatLowLevel.hpp"

#include "icatHighLevelRoutines.hpp"

/* For now, uncomment this line to build RDA
#define BUILD_RDA 1  */
/* Do the same in reRDA.c */

#define RDA_CONFIG_FILE "rda.config"
#define RDA_ACCESS_ATTRIBUTE "RDA_ACCESS"

#define BUF_LEN 500

#define MAX_RDA_NAME_LEN 200
#define BIG_STR 200

#define RESULT_BUF1_SIZE 2000

static char openRdaName[MAX_RDA_NAME_LEN + 2] = "";

int rdaLogSQL = 0;
int readRdaConfig( char *rdaName, char **DBUser, char**DBPasswd );

icatSessionStruct rda_icss = {0};

int rdaDebug( char *debugMode ) {
    if ( strstr( debugMode, "SQL" ) ) {
        rdaLogSQL = 1;
    }
    else {
        rdaLogSQL = 0;
    }
    return( 0 );
}



int rdaOpen( char *rdaName ) {
#if defined(BUILD_RDA)
    int i;
    int status;
    char *DBUser;
    char *DBPasswd;

    if ( rdaLogSQL ) {
        rodsLog( LOG_SQL, "rdaOpen" );
    }

    if ( strcmp( openRdaName, rdaName ) == 0 ) {
        return( 0 ); /* already open */
    }

    status =  readRdaConfig( rdaName, &DBUser, &DBPasswd );
    if ( status ) {
        return( status );
    }

    strncpy( rda_icss.databaseUsername, DBUser, DB_USERNAME_LEN );
    strncpy( rda_icss.databasePassword, DBPasswd, DB_PASSWORD_LEN );

    /* Initialize the rda_icss statement pointers */
    for ( i = 0; i < MAX_NUM_OF_CONCURRENT_STMTS; i++ ) {
        rda_icss.stmtPtr[i] = 0;
    }

    /*
     Set the DBMS type.  The Low Level now uses this instead of the ifdefs
     so it can interact with either at the same time (for the DBR/DBO
     feature).  But for RDA this is still just using an ifdef to select
     the type; DBR handles it on a per-DBR definition basis.
    */
    rda_icss->databaseType = DB_TYPE_POSTGRES;
#ifdef ORA_ICAT
    rda_icss->databaseType = DB_TYPE_ORACLE;
#endif
#ifdef MY_ICAT
    rda_icss->databaseType = DB_TYPE_MYSQL;
#endif

    /* Open Environment */
    i = cllOpenEnv( &rda_icss );
    if ( i != 0 ) {
        rodsLog( LOG_NOTICE, "rdaOpen cllOpen failure %d", i );
        return( RDA_ENV_ERR );
    }

    /* Connect to the DBMS */
    i = cllConnectRda( &rda_icss );
    if ( i != 0 ) {
        rodsLog( LOG_NOTICE, "rdaOpen cllConnectRda failure %d", i );
        return( RDA_CONNECT_ERR );
    }

    rda_icss.status = 1;
    strncmp( openRdaName, rdaName, MAX_RDA_NAME_LEN );

    return( 0 );
#else
    openRdaName[0] = '\0'; /* avoid warning */
    return( RDA_NOT_COMPILED_IN );
#endif
}

int rdaClose() {
#if defined(BUILD_RDA)
    int status, stat2;

    status = cllDisconnect( &rda_icss );

    stat2 = cllCloseEnv( &rda_icss );

    openRdaName[0] = '\0';

    if ( status ) {
        return( RDA_DISCONNECT_ERR );
    }
    if ( stat2 ) {
        return( RDA_CLOSE_ENV_ERR );
    }
    rda_icss.status = 0;
    return( 0 );
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}

int rdaCommit() {
#if defined(BUILD_RDA)
    int status;

    status = cllExecSqlNoResult( &rda_icss, "commit" );
    return( status );
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}

int rdaRollback() {
#if defined(BUILD_RDA)
    int status;

    status = cllExecSqlNoResult( &rda_icss, "rollback" );
    return( status );
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}

int rdaIsConnected() {
#if defined(BUILD_RDA)
    if ( rdaLogSQL ) {
        rodsLog( LOG_SQL, "rdaIsConnected" );
    }
    return( rda_icss.status );
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}

/*
  Check to see if the client user has access to this RDA
  by querying the user AVU.
 */
int rdaCheckAccess( char *rdaName, rsComm_t *rsComm ) {
#if defined(BUILD_RDA)
    genQueryInp_t genQueryInp;
    genQueryOut_t genQueryOut;
    int iAttr[10];
    int iAttrVal[10] = {0, 0, 0, 0, 0};
    int iCond[10];
    char *condVal[10];
    char v1[BIG_STR];
    char v2[BIG_STR];
    char v3[BIG_STR];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    iAttr[0] = COL_META_USER_ATTR_VALUE;
    genQueryInp.selectInp.inx = iAttr;
    genQueryInp.selectInp.value = iAttrVal;
    genQueryInp.selectInp.len = 1;

    iCond[0] = COL_USER_NAME;
    sprintf( v1, "='%s'", rsComm->clientUser.userName );
    condVal[0] = v1;

    iCond[1] = COL_META_USER_ATTR_NAME;
    sprintf( v2, "='%s'", RDA_ACCESS_ATTRIBUTE );
    condVal[1] = v2;

    iCond[2] = COL_META_USER_ATTR_VALUE;
    sprintf( v3, "='%s'", rdaName );
    condVal[2] = v3;

    genQueryInp.sqlCondInp.inx = iCond;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 3;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;
    status = chlGenQuery( genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        return( RDA_ACCESS_PROHIBITED );
    }

    return ( status ); /* any error, no access */
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}


int rdaSqlNoResults( char *sql, char *parm[], int nParms ) {
#if defined(BUILD_RDA)
    int i;
    for ( i = 0; i < nParms; i++ ) {
        cllBindVars[i] = parm[i];
    }
    cllBindVarCount = nParms;
    i = cllExecSqlNoResult( &rda_icss, sql );
    /*   if (i <= CAT_ENV_ERR) return(i); ? already an iRODS error code */
    printf( "i=%d\n", i );
    if ( i == CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        return( 0 );
    }
    if ( i ) {
        return( RDA_SQL_ERR );
    }
    return( 0 );
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}

int rdaSqlWithResults( char *sql, char *parm[], int nParms, char **outBuf ) {
#if defined(BUILD_RDA)
    int i, ii;
    int statement;
    int rowCount, nCols, rowSize = 0;
    int maxOutBuf;
    char *myBuf, *pbuf;
    static char resultBuf1[RESULT_BUF1_SIZE + 10];
    int totalRows, toMalloc;

    for ( i = 0; i < nParms; i++ ) {
        cllBindVars[i] = parm[i];
    }
    cllBindVarCount = nParms;
    i = cllExecSqlWithResult( &rda_icss, &statement, sql );
    printf( "i=%d\n", i );
    /* ?   if (i==CAT_SUCCESS_BUT_WITH_NO_INFO) return(0); */
    if ( i ) {
        return( RDA_SQL_ERR );
    }

    myBuf = resultBuf1; /* Initially, use this buffer */
    maxOutBuf = RESULT_BUF1_SIZE;
    memset( myBuf, 0, maxOutBuf );

    for ( rowCount = 0;; rowCount++ ) {
        i = cllGetRow( &rda_icss, statement );
        if ( i != 0 )  {
            ii = cllFreeStatement( &rda_icss, statement );
            if ( rowCount == 0 ) {
                return( CAT_GET_ROW_ERR );
            }
            *outBuf = myBuf;
            return( 0 );
        }

        if ( rda_icss.stmtPtr[statement]->numOfCols == 0 ) {
            i = cllFreeStatement( &rda_icss, statement );
            if ( rowCount == 0 ) {
                return( CAT_NO_ROWS_FOUND );
            }
            *outBuf = myBuf;
            return( 0 );
        }

        nCols = rda_icss.stmtPtr[statement]->numOfCols;
        if ( rowCount == 0 ) {
            for ( i = 0; i < nCols ; i++ ) {
                rstrcat( myBuf, rda_icss.stmtPtr[statement]->resultColName[i],
                         maxOutBuf );
                rstrcat( myBuf, "|", maxOutBuf );
            }
            rstrcat( myBuf, "\n", maxOutBuf );
        }
        for ( i = 0; i < nCols ; i++ ) {
            rstrcat( myBuf, rda_icss.stmtPtr[statement]->resultValue[i],
                     maxOutBuf );
            rstrcat( myBuf, "|", maxOutBuf );
        }
        rstrcat( myBuf, "\n", maxOutBuf );
        if ( rowSize == 0 ) {
            rowSize = strlen( myBuf );
            totalRows = cllGetRowCount( &rda_icss, statement );
            printf( "rowSize=%d, totalRows=%d\n", rowSize, totalRows );
            if ( totalRows < 0 ) {
                return( totalRows );
            }
            if ( totalRows == 0 ) {
                /* Unknown number of rows available (Oracle) */
                totalRows = 10; /* to start with */
            }
            toMalloc = ( ( totalRows + 1 ) * rowSize ) * 3;
            myBuf = malloc( toMalloc );
            if ( myBuf <= 0 ) {
                free( myBuf );    // JMC cppcheck - leak
                return( SYS_MALLOC_ERR );
            }
            maxOutBuf = toMalloc;
            memset( myBuf, 0, maxOutBuf );
            rstrcpy( myBuf, resultBuf1, RESULT_BUF1_SIZE );
        }
        if ( rowCount > totalRows ) {
            int oldTotalRows;
            oldTotalRows = totalRows;
            if ( totalRows < 1000 ) {
                totalRows = totalRows * 2;
            }
            else {
                totalRows = totalRows + 500;
            }
            pbuf = myBuf;
            toMalloc = ( ( totalRows + 1 ) * rowSize ) * 3;
            myBuf = malloc( toMalloc );
            if ( myBuf <= 0 ) {
                free( myBuf );    // JMC cppcheck - leak
                return( SYS_MALLOC_ERR );
            }
            maxOutBuf = toMalloc;
            memset( myBuf, 0, maxOutBuf );
            strcpy( myBuf, pbuf );
            free( pbuf );
        }
    }

    return( 0 ); /* never reached */
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}

char *
getRdaConfigDir() {
    char *myDir;

    if ( ( myDir = ( char * ) getenv( "irodsConfigDir" ) ) != ( char * ) NULL ) {
        return ( myDir );
    }
    return ( DEF_CONFIG_DIR );
}

int
readRdaConfig( char *rdaName, char **DBUser, char**DBPasswd ) {
    FILE *fptr;
    char buf[BUF_LEN];
    char *fchar;
    char *key;
    char *rdaConfigFile;
    static char foundLine[BUF_LEN];

    rdaConfigFile = ( char * ) malloc( ( strlen( getRdaConfigDir() ) +
                                         strlen( RDA_CONFIG_FILE ) + 24 ) );

    sprintf( rdaConfigFile, "%s/%s", getRdaConfigDir(),
             RDA_CONFIG_FILE );

    fptr = fopen( rdaConfigFile, "r" );

    if ( fptr == NULL ) {
        rodsLog( LOG_NOTICE,
                 "Cannot open RDA_CONFIG_FILE file %s. errno = %d\n",
                 rdaConfigFile, errno );
        free( rdaConfigFile );
        return ( RDA_CONFIG_FILE_ERR );
    }
    free( rdaConfigFile );

    buf[BUF_LEN - 1] = '\0';
    fchar = fgets( buf, BUF_LEN - 1, fptr );
    for ( ; fchar != '\0'; ) {
        if ( buf[0] == '#' || buf[0] == '/' ) {
            buf[0] = '\0'; /* Comment line, ignore */
        }
        key = strstr( buf, rdaName );
        if ( key == buf ) {
            int state, i;
            char *DBKey = 0;
            rstrcpy( foundLine, buf, BUF_LEN );
            state = 0;
            for ( i = 0; i < BUF_LEN; i++ ) {
                if ( foundLine[i] == ' ' || foundLine[i] == '\n' ) {
                    int endOfLine;
                    endOfLine = 0;
                    if ( foundLine[i] == '\n' ) {
                        endOfLine = 1;
                    }
                    foundLine[i] = '\0';
                    if ( endOfLine && state < 6 ) {
                        fclose( fptr ); // JMC cppcheck - resource
                        return( 0 );
                    }
                    if ( state == 0 ) {
                        state = 1;
                    }
                    if ( state == 2 ) {
                        state = 3;
                    }
                    if ( state == 4 ) {
                        state = 5;
                    }
                    if ( state == 6 ) {
                        static char unscrambledPw[NAME_LEN];
                        obfDecodeByKey( *DBPasswd, DBKey, unscrambledPw );
                        *DBPasswd = unscrambledPw;
                        fclose( fptr ); // JMC cppcheck - resource
                        return( 0 );
                    }
                }
                else {
                    if ( state == 1 ) {
                        state = 2;
                        *DBUser = &foundLine[i];
                    }
                    if ( state == 3 ) {
                        state = 4;
                        *DBPasswd = &foundLine[i];
                    }
                    if ( state == 5 ) {
                        state = 6;
                        DBKey = &foundLine[i];
                    }
                }
            }
        }
        fchar = fgets( buf, BUF_LEN - 1, fptr );
    }
    fclose( fptr );

    return( RDA_NAME_NOT_FOUND );
}
