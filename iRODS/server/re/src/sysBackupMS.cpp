/**
 * @file sysBackupMS.c
 *
 * /brief System Backup Microservices.
 */


/*** Copyright (c), The DICE Foundation.									***
 *** For more information please refer to files in the COPYRIGHT directory	***/



#include <stdlib.h>
#include <dirent.h>
#include "rodsDef.hpp"
#include "sysBackupMS.hpp"
#include "resource.hpp"
#include "fileOpr.hpp"
#include "physPath.hpp"
#include "objMetaOpr.hpp"
#include "apiHeaderAll.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/system/error_code.hpp>


/*
 * \fn 		loadDirToLocalResc(ruleExecInfo_t *rei, char *dirPath, size_t offset,
 * 				char *resDirPath, char *timestamp, char *dbPath)
 *
 * \brief	recursive helper function for msiServerBackup that reads a local
 * 			directory and copies some of its content to the local resource
 *
 */
int loadDirToLocalResc( ruleExecInfo_t *rei, char *dirPath, size_t offset,
                        char *resDirPath, char *timestamp, char *dbPath ) {
    DIR *myDir;
    struct dirent *de;
    struct stat s;
    char absPath[MAX_NAME_LEN], *subPath;
    char sysCopyCmd[2 * MAX_NAME_LEN];
    int filecount = 0, status;
    char *dirname;

    /* Get name of directory */
    if ( ( dirname = strrchr( dirPath, '/' ) ) == NULL ) {
        rei->status = SYS_INVALID_FILE_PATH;
        return 0;
    }
    dirname += 1;

    /* Stuff we'll ignore */
    /* Make sure to skip the vault, which by default is
     * installed under the main iRODS home directory */
    if ( !strcmp( dirname, "." )
            || !strcmp( dirname, ".." )
//			|| !strcmp(dirname, ".svn")
            || !strcmp( dirPath, resDirPath )  /* the vault */ ) {
        return 0;
    }


    /* Also skip iCAT directory */
    if ( dbPath && !strcmp( dirPath, dbPath ) ) {
        return 0;
    }

//	/* For later, an option to create certain collections without content */
//	if ( !strcmp(dirname, "obj") || !strcmp(dirname, "log"))
//	{
//		return 0;
//	}


    /* Create new dir on resource */

    /* Separated for clarity. Typically this chunk is 'home/$username' */
    subPath = rei->rsComm->myEnv.rodsHome + strlen( rei->rsComm->myEnv.rodsZone ) + 2;

    /* The target directory should look like something like this:
     * '$resource_dir/home/$username/system_backups/$hostname_$timestamp/iRODS/...' */
    snprintf( sysCopyCmd, 2 * MAX_NAME_LEN, "mkdir -p \"%s/%s/%s/%s_%s/%s\"",
              resDirPath, subPath, BCKP_COLL_NAME, rei->rsComm->myEnv.rodsHost, timestamp,
              dirPath + offset );


    /* Make new directory on resource */
    status = system( sysCopyCmd );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "loadDirToLocalResc: mkdir error %d.", status );
        rei->status = UNIX_FILE_MKDIR_ERR;
    }


    /* Read our directory */
    myDir = opendir( dirPath );

    while ( ( de = readdir( myDir ) ) != NULL ) {
        /* Ignored */
        if ( !strcmp( de->d_name, ".DS_Store" ) ) {
            continue;
        }

        /* Stat our directory entry */
        snprintf( absPath, MAX_NAME_LEN, "%s/%s", dirPath, de->d_name );
        if ( lstat( absPath, &s ) != 0 ) {
            rodsLog( LOG_ERROR, "putDir error: cannot lstat %s, %s", absPath, strerror( errno ) );
            rei->status = UNIX_FILE_STAT_ERR;
            continue;
        }


        if ( S_ISDIR( s.st_mode ) ) {

            /* Recursively process directories */
            filecount += loadDirToLocalResc( rei, absPath, offset,
                                             resDirPath, timestamp, dbPath );

        }
        else {
            /* Copy file to new dir on resource */
            std::string destPath = boost::str( boost::format( "%s/%s/%s/%s_%s/%s/%s" ) %
                                               resDirPath %
                                               subPath %
                                               BCKP_COLL_NAME %
                                               rei->rsComm->myEnv.rodsHost %
                                               timestamp %
                                               ( dirPath + offset ) %
                                               de->d_name );

            boost::system::error_code error;
            boost::filesystem::copy( boost::filesystem::path( absPath ), boost::filesystem::path( destPath ), error );
            if ( error.value() ) {
                rodsLog( LOG_ERROR, "loadDirToLocalResc: cp error, status = %d.", error.message().c_str() );
                rei->status = FILE_OPEN_ERR;
            }

            filecount++;
        }

    }

    closedir( myDir );

    return filecount;
}



/*
 * \fn 		getDBHomeDir(rescInfo_t **rescInfo)
 *
 * \brief	Gets path of local iCAT DB from irods.config
 *
 *
 */
char *getDBHomeDir() {
    char configFilePath[MAX_PATH_ALLOWED + 1];
    char buf[LONG_NAME_LEN * 5];
    char *dbPath = NULL;
    FILE *configFile;

#ifndef RODS_CAT
    return NULL;
#endif

    /* Open server configuration file */
    snprintf( configFilePath, MAX_PATH_ALLOWED, "%s/config/%s", getenv( "irodsHomeDir" ), "irods.config" );
    configFile = fopen( configFilePath, "r" );
    if ( configFile == NULL ) {
        rodsLog( LOG_ERROR, "getDefaultLocalRescInfo: Cannot open configuration file %s",
                 configFilePath );
        return NULL;
    }

    /* Read one line at a time */
    while ( fgets( buf, LONG_NAME_LEN * 5, configFile ) != NULL ) {
        /* Find line that starts with $DATABASE_HOME */
        if ( strstr( buf, "$DATABASE_HOME" ) == buf ) {
            /* DB path starts after the first single quote */
            dbPath = strchr( buf, '\'' ) + 1;

            /* Replace 2d single quote with null char */
            strchr( dbPath, '\'' )[0] = '\0';

            break;
        }
    }

    fclose( configFile );

    if ( dbPath != NULL ) {
        return strdup( dbPath );
    }
    else {
        return NULL;
    }
}



/*
 * \fn 		getDefaultLocalRescInfo(rescInfo_t **rescInfo)
 *
 * \brief	Gets resource information for the resource listed in irods.config
 *
 *
 */
int getDefaultLocalRescInfo( rescInfo_t **rescInfo ) {
    char configFilePath[MAX_PATH_ALLOWED + 1];
    char buf[LONG_NAME_LEN * 5];
    char *rescName = NULL;
    FILE *configFile;

    /* Open server configuration file */
    snprintf( configFilePath, MAX_PATH_ALLOWED, "%s/config/%s", getenv( "irodsHomeDir" ), "irods.config" );
    configFile = fopen( configFilePath, "r" );
    if ( configFile == NULL ) {
        rodsLog( LOG_ERROR, "getDefaultLocalRescInfo: Cannot open configuration file %s",
                 configFilePath );
        return FILE_OPEN_ERR;
    }

    /* Read one line at a time */
    while ( fgets( buf, LONG_NAME_LEN * 5, configFile ) != NULL ) {
        /* Find line that starts with $RESOURCE_NAME */
        if ( strstr( buf, "$RESOURCE_NAME" ) == buf ) {
            /* Resource name starts after the first single quote */
            rescName = strchr( buf, '\'' ) + 1;

            /* Replace 2d single quote with null char */
            strchr( rescName, '\'' )[0] = '\0';

            break;
        }
    }

    fclose( configFile );

    if ( rescName == NULL ) {
        rodsLog( LOG_ERROR,
                 "getDefaultLocalRescInfo: Local resource not found in configuration file." );
        return SYS_CONFIG_FILE_ERR;
    }

    /* Resolve resource if resource name was found */

    if ( !( *rescInfo ) ) {
        *rescInfo = new rescInfo_t;
    }

    irods::resource_ptr resc;
    irods::error err = irods::get_resc_info( rescName, **rescInfo );
    if ( !err.ok() ) {
        std::stringstream msg;
        msg << "failed to resolve resource [";
        msg << rescName << "]";
        irods::log( PASSMSG( msg.str(), err ) );
        return err.code();
    }

    return 0;
}



/**
 * \fn msiServerBackup(msParam_t *options, msParam_t *keyValOut, ruleExecInfo_t *rei)
 *
 * \brief Copies iRODS server files to the local resource
 *
 * \module core
 *
 * \since 3.0.x
 *
 * \author  Antoine de Torcy
 * \date    2011-05-25
 *
 * \note  Copies server files to the local vault and registers them.
 *    Object (.o) files and binaries are not included.
 *
 * \note Files are stored in the Vault under a directory of the format: hostname_timestamp
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] options - Optional - a STR_MS_T that contains one of more options in
 *      the format keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *      A placeholder for now.
 * \param[out] keyValOut - a KeyValPair_MS_T with the number of files and bytes written.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified Some
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiServerBackup( msParam_t*, msParam_t* keyValOut, ruleExecInfo_t* rei ) {
    keyValPair_t *myKeyVal;						/* for storing results */
    collInp_t collInp;							/* for creating and opening collections */

    dataObjInp_t dataObjInp;					/* for collection registration */

    char tStr0[TIME_LEN], tStr[TIME_LEN];		/* for timestamp */

    char newDirPath[MAX_NAME_LEN];				/* physical path of new directory on resource */

    char *dbPath;								/* local iCAT home dir */

    char *rodsDirPath, *subPath;
    size_t offset;

    int fileCount, status;						/* counters, status, etc... */
    char fileCountStr[21];



    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiServerBackup" )

    /* Sanity checks */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiServerBackup: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /* Must be called from an admin account */
    if ( rei->uoic->authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        status = CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        rodsLog( LOG_ERROR, "msiServerBackup: User %s is not local admin. Status = %d",
                 rei->uoic->userName, status );
        return status;
    }


    /* Get icat home dir, if applicable */
    dbPath = getDBHomeDir();

    /* Get local resource info */
    rescInfo_t *rescInfo = NULL; // for local resource info
    status = getDefaultLocalRescInfo( &rescInfo );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "msiServerBackup: Could not resolve local resource, status = %d",
                 status );
        if ( dbPath ) {
            free( dbPath );
        }
        return status;
    }


    /* Get path of iRODS home directory */
    if ( ( rodsDirPath = getenv( "irodsHomeDir" ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiServerBackup: Cannot find directory to back up." );
        if ( dbPath ) {
            free( dbPath );
        }
        return USER_INPUT_PATH_ERR;
    }


    /***** Create target directory for copy, whose name is made
     ****** of the hostname and timestamp ********************/

    /* get timestamp */
    getNowStr( tStr0 );
    getLocalTimeFromRodsTime( tStr0, tStr );

    /**********************************/


    /* Prepare myKeyVal so that we can dump
     * data in it throughout the microservice */
    myKeyVal = ( keyValPair_t* ) malloc( sizeof( keyValPair_t ) );
    memset( myKeyVal, 0, sizeof( keyValPair_t ) );
    keyValOut->type = strdup( KeyValPair_MS_T );



    /* Store local and target directories in myKeyVal, along with other useful stuff */

    /* Calculate offset */
    offset = strrchr( rodsDirPath, '/' ) - rodsDirPath + 1;


    /*******************************************/


    /************ invoke loadDirToLocalResc ***************************/

    fileCount = loadDirToLocalResc( rei, rodsDirPath, offset, rescInfo->rescVaultPath, tStr, dbPath );

    /* get some cleanup out of the way */
    if ( dbPath ) {
        free( dbPath );
    }

    if ( rei->status < 0 ) {
        rodsLog( LOG_ERROR, "msiServerBackup: loadDirToLocalResc() error, status = %d",
                 rei->status );
        free( myKeyVal );
        return rei->status;
    }

    /******************************************************/


    /* We need to create a parent collection prior to registering our directory */
    /* set up collection creation input */
    memset( &collInp, 0, sizeof( collInp_t ) );
    addKeyVal( &collInp.condInput, RECURSIVE_OPR__KW, "" );

    /* build path of target collection */
    snprintf( collInp.collName, MAX_NAME_LEN, "%s/%s/%s_%s", rei->rsComm->myEnv.rodsHome,
              BCKP_COLL_NAME, rei->rsComm->myEnv.rodsHost, tStr );

    /* make target collection */
    rei->status = rsCollCreate( rei->rsComm, &collInp );
    if ( rei->status < 0 ) {
        rodsLog( LOG_ERROR, "msiServerBackup: rsCollCreate failed for %s, status = %d",
                 collInp.collName, rei->status );
        free( myKeyVal );
        return rei->status;
    }


    /* Register our new directory in the vault */

    /* Input setup */
    memset( &dataObjInp, 0, sizeof( dataObjInp_t ) );
    addKeyVal( &dataObjInp.condInput, COLLECTION_KW, "" );
    addKeyVal( &dataObjInp.condInput, DEST_RESC_NAME_KW, rescInfo->rescName );

    /* Separated for clarity. Typically this chunk is 'home/$username' */
    subPath = rei->rsComm->myEnv.rodsHome + strlen( rei->rsComm->myEnv.rodsZone ) + 2;

    /* Reconstruct path of new dir on resource */
    snprintf( newDirPath, MAX_NAME_LEN, "%s/%s/%s/%s_%s/%s", rescInfo->rescVaultPath,
              subPath, BCKP_COLL_NAME, rei->rsComm->myEnv.rodsHost, tStr, rodsDirPath + offset );

    addKeyVal( &dataObjInp.condInput, FILE_PATH_KW, newDirPath );

    /* Similarly, reconstruct iRODS path of (target) new collection */
    snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s/%s/%s_%s/%s", rei->rsComm->myEnv.rodsHome,
              BCKP_COLL_NAME, rei->rsComm->myEnv.rodsHost, tStr, rodsDirPath + offset );


    /* Registration happens here */
    rei->status = rsPhyPathReg( rei->rsComm, &dataObjInp );
    if ( rei->status < 0 ) {
        rodsLog( LOG_ERROR, "msiServerBackup: rsPhyPathReg() failed with status %d", rei->status );
        free( myKeyVal );
        return rei->status;
    }


    /* Add file count to myKeyVal */
    snprintf( fileCountStr, 21, "%d", fileCount );
    addKeyVal( myKeyVal, "object_count", fileCountStr ); // stub

    /* Return myKeyVal through keyValOut */
    keyValOut->inOutStruct = ( void* ) myKeyVal;


    /* Done! */
    return 0;
}
