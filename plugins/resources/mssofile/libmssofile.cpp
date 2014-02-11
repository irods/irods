


#include "rods.hpp"
//#include "structFileDriver.hpp"
#include "execMyRule.hpp"


#define NUM_MSSO_SUB_FILE_DESC 1024
#define MSSO_CACHE_DIR_STR "mssoCacheDir"
#define MSSO_RUN_DIR_STR "runDir"
#define MSSO_RUN_FILE_STR "run"
#define MSSO_MPF_FILE_STR "mpf"

namespace irods {
    const int NUM_STRUCT_FILE_DESC = 16;

    typedef struct mssoSubFileDesc {
        int inuseFlag;
        int structFileInx;
        int fd;                         /* the fd of the opened cached subFile */
        char cacheFilePath[MAX_NAME_LEN];   /* the phy path name of the cached
                                             * subFile */
    } mssoSubFileDesc_t;

    typedef struct mssoSubFileStack {
        int structFileInx;
        int fd;                         /* the fd of the opened cached subFile */
        char cacheFilePath[MAX_NAME_LEN];
        char *stageIn[100];   /* stages from irods to execuion area */
        char *stageOut[100];  /* stages into irods from execution area */
        char *copyToIrods[100];  /* copy into irods from execution area */
        char *cleanOut[100];  /* clear from execution area */
        char *checkForChange[100];  /* check if these files are newer than the change directort */
        char stageArea[MAX_NAME_LEN]; /* is cmd/bin by default */
        int stinCnt;
        int stoutCnt;
        int cpoutCnt;
        int clnoutCnt;
        int cfcCnt;
        int noVersions;
        char newRunDirName[MAX_NAME_LEN]; /* name of new place for old  run dir  */
        int oldRunDirTime; /* time of old run dir which was moved to make place to new one */

    } mssoSubFileStack_t;

    int MssoSubFileStackTop = 0;
    structFileDesc_t   MssoStructFileDesc[ NUM_STRUCT_FILE_DESC   ];
    mssoSubFileDesc_t  MssoSubFileDesc   [ NUM_MSSO_SUB_FILE_DESC ];
    mssoSubFileStack_t MssoSubFileStack  [ NUM_MSSO_SUB_FILE_DESC ];

#define MSSO_DEBUG 1
    static char *stageIn[100];   /* stages from irods to execuion area */
    static char *stageOut[100];  /* stages into irods from execution area */
    static char *copyToIrods[100];  /* copy into irods from execution area */
    static char *cleanOut[100];  /* clear from execution area */
    static char *checkForChange[100];  /* check if these files are newer than the change directort */
    static char stageArea[MAX_NAME_LEN]; /* is cmd/bin by default */
    static int stinCnt;
    static int stoutCnt;
    static int cpoutCnt;
    static int clnoutCnt;
    static int cfcCnt;
    static int noVersions = 0;
    static char newRunDirName[MAX_NAME_LEN]; /* name of new place for old  run dir  */
    static int oldRunDirTime; /* time of old run dir which was moved to make place to new one */
    int checkSafety( char *file ) {
        /*  need to see if path is good. it should be only  a logical file */
        /* it may be file in another zone also!!! */
        return( 0 );
    }


    int pushOnStackInfo() {
        int  i, k;

        if ( MssoSubFileStackTop == 0 ) {
            MssoSubFileStackTop++;
            return( 0 );
        }
        k = MssoSubFileStackTop - 1;
        MssoSubFileStackTop++;

        /* push the  stuff */
        MssoSubFileStack[k].stinCnt = stinCnt;
        MssoSubFileStack[k].stoutCnt = stoutCnt;
        MssoSubFileStack[k].cpoutCnt = cpoutCnt;
        MssoSubFileStack[k].clnoutCnt = clnoutCnt;
        MssoSubFileStack[k].cfcCnt = cfcCnt;
        MssoSubFileStack[k].noVersions = noVersions;
        MssoSubFileStack[k].oldRunDirTime = oldRunDirTime;

        strncpy( MssoSubFileStack[k].newRunDirName, newRunDirName, MAX_NAME_LEN );
        strncpy( MssoSubFileStack[k].stageArea, stageArea, MAX_NAME_LEN );
        for ( i = 0; i < stinCnt; i++ ) {
            MssoSubFileStack[k].stageIn[i] = stageIn[i];
        }
        for ( i = 0; i < stoutCnt; i++ ) {
            MssoSubFileStack[k].stageOut[i] = stageOut[i];
        }
        for ( i = 0; i < cpoutCnt; i++ ) {
            MssoSubFileStack[k].copyToIrods[i] = copyToIrods[i];
        }
        for ( i = 0; i < clnoutCnt; i++ ) {
            MssoSubFileStack[k].cleanOut[i] = cleanOut[i];
        }
        for ( i = 0; i < cfcCnt; i++ ) {
            MssoSubFileStack[k].checkForChange[i] = checkForChange[i];
        }

        return( 0 );
    }

    int popOutStackInfo() {
        int i, k;
        MssoSubFileStackTop--;
        if ( MssoSubFileStackTop == 0 ) {
            return( 0 );
        }
        k = MssoSubFileStackTop - 1;

        /* pop the  stuff */
        stinCnt = MssoSubFileStack[k].stinCnt ;
        stoutCnt = MssoSubFileStack[k].stoutCnt ;
        cpoutCnt = MssoSubFileStack[k].cpoutCnt ;
        clnoutCnt = MssoSubFileStack[k].clnoutCnt ;
        cfcCnt = MssoSubFileStack[k].cfcCnt ;
        noVersions = MssoSubFileStack[k].noVersions ;
        oldRunDirTime = MssoSubFileStack[k].oldRunDirTime ;

        strncpy( newRunDirName, MssoSubFileStack[k].newRunDirName, MAX_NAME_LEN );
        strncpy( stageArea, MssoSubFileStack[k].stageArea, MAX_NAME_LEN );
        for ( i = 0; i < stinCnt; i++ ) {
            stageIn[i] =   MssoSubFileStack[k].stageIn[i] ;
        }
        for ( i = 0; i < stoutCnt; i++ ) {
            stageOut[i] =   MssoSubFileStack[k].stageOut[i] ;
        }
        for ( i = 0; i < cpoutCnt; i++ ) {
            copyToIrods[i] =   MssoSubFileStack[k].copyToIrods[i] ;
        }
        for ( i = 0; i < clnoutCnt; i++ ) {
            cleanOut[i] =   MssoSubFileStack[k].cleanOut[i] ;
        }
        for ( i = 0; i < cfcCnt; i++ ) {
            checkForChange[i] =   MssoSubFileStack[k].checkForChange[i] ;
        }


        return( 0 );


    }

    int
    checkPhySafety( char *file ) {
        /*  need to see if path is good. it should be only  a logical file */
        /* it may be file in another zone also!!! */
        return( 0 );
    }


    int
    matchMssoStructFileDesc( specColl_t *specColl ) {
        int i;

        for ( i = 1; i < NUM_STRUCT_FILE_DESC; i++ ) {
            if ( StructFileDesc[i].inuseFlag == FD_INUSE &&
                    StructFileDesc[i].specColl != NULL &&
                    strcmp( StructFileDesc[i].specColl->collection, specColl->collection )	    == 0 &&
                    strcmp( StructFileDesc[i].specColl->objPath, specColl->objPath )
                    == 0 ) {
                return ( i );
            };
        }

        return ( SYS_OUT_OF_FILE_DESC );

    }


    int
    isFakeFile( char *coll, char*objPath ) {
        char s[MAX_NAME_LEN];

        sprintf( s, "%s/myFakeFile", coll );
        if ( !strcmp( objPath, s ) ) {
            return( 1 );
        }
        else {
            return( 0 );
        }
    }


    int
    rsMssoStructFileOpen( rsComm_t *rsComm, specColl_t *specColl, subFile_t *subFile, int stage ) {
        int structFileInx;
        int status;
        specCollCache_t *specCollCache;

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering rsMssotructFileOpen with file:%s for Collection %s and specColl=%lld  and subFile=%lld and specColl->phyPath=%s and specColl->resource=%s and stage=%d", specColl->objPath, specColl->collection, ( long long int )( void * ) specColl, ( long long int )( void * ) subFile, specColl->phyPath,  specColl->resource, stage );
#endif /* MSSO_DEBUG */

        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsMssoStructFileOpen: NULL specColl input" );
            return ( SYS_INTERNAL_NULL_INPUT_ERR );
        }

        /* look for opened StructFileDesc */

        structFileInx = matchMssoStructFileDesc( specColl );
#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "In rsMssotructFileOpen with structFileInx :%d", structFileInx );
        if ( structFileInx > 0 )
            rodsLog( LOG_NOTICE, "In rsMssotructFileOpen with inuseFlag=%d and openCnt=%d",
                     StructFileDesc[structFileInx].inuseFlag, StructFileDesc[structFileInx].openCnt );
#endif /* MSSO_DEBUG */


        if ( structFileInx > 0 && ( stage != 1 ) ) { return structFileInx; }

        if ( structFileInx < 0 ) {
            if ( ( structFileInx = allocStructFileDesc() ) < 0 ) {
                return ( structFileInx );
            }

            /* Have to do this because specColl could come from a remote host */
            if ( ( status = getSpecCollCache( rsComm, specColl->collection, 0,
                                              &specCollCache ) ) >= 0 ) {
                StructFileDesc[structFileInx].specColl = &specCollCache->specColl;
                /* getSpecCollCache does not give phyPath nor resource */
                if ( strlen( specColl->phyPath ) > 0 ) {
                    rstrcpy( specCollCache->specColl.phyPath, specColl->phyPath,
                             MAX_NAME_LEN );
                }
                if ( strlen( specCollCache->specColl.resource ) == 0 ) {
                    rstrcpy( specCollCache->specColl.resource, specColl->resource,
                             NAME_LEN );
                }
            }
            else {
                StructFileDesc[structFileInx].specColl = specColl;
            }

            StructFileDesc[structFileInx].rsComm = rsComm;

            status = resolveResc( StructFileDesc[structFileInx].specColl->resource,
                                  &StructFileDesc[structFileInx].rescInfo );

            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "rsMssoStructFileOpen: resolveResc error for %s, status = %d",
                         specColl->resource, status );
                freeStructFileDesc( structFileInx );
                return ( status );
            }
        }
        /* XXXXX need to deal with remote open here */
        /* do not stage unless it is a get or am open */
        if ( isFakeFile( specColl->collection, specColl->objPath ) == 0 ) {
            /* not a fake file */
            if ( stage != 1 ) {
                return ( structFileInx );
            }
        }

        status = stageMssoStructFile( structFileInx, subFile );

        if ( status < 0 ) {
            freeStructFileDesc( structFileInx );
            return status;
        }

        return ( structFileInx );
    }


    int
    stageMssoStructFile( int structFileInx, subFile_t *subFile ) {
        int status;
        specColl_t *specColl;
        rsComm_t *rsComm;
        int fileType = 0; /* 1=mss file, 2=mpf file 3=datafile */
        char *t, *s;
        char runDir[MAX_NAME_LEN];
        char showFiles[MAX_NAME_LEN];

        rsComm = StructFileDesc[structFileInx].rsComm;
        specColl = StructFileDesc[structFileInx].specColl;
        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE,
                     "stageMssoStructFile: NULL specColl input" );
            return ( SYS_INTERNAL_NULL_INPUT_ERR );
        }

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering stageMssoSubStructFile with specColl->collection=%s and specColl->cacheDir=%s and subFile->subFilePath=%s and specColl->objPath=%s and specColl->phyPath=%s",
                 StructFileDesc[structFileInx].specColl->collection,
                 StructFileDesc[structFileInx].specColl->cacheDir,
                 subFile->subFilePath, specColl->objPath, specColl->phyPath );
#endif /* MSSO_DEBUG */


        if ( isFakeFile( specColl->collection, specColl->objPath ) == 1 ) {
            /* is fake file. make  t point to NULL */
            t = runDir;
            runDir[0] = '\0';
        }
        else {
            if ( ( t = strstr( subFile->subFilePath, StructFileDesc[structFileInx].specColl->collection ) ) == NULL ) {
                return( SYS_STRUCT_FILE_PATH_ERR );
            }

            t = t + strlen( StructFileDesc[structFileInx].specColl->collection );
        }
        if ( *t == '\0' ) {
            fileType = 1;
        }
        else if ( *t == '/' ) {
            t = t + 1; /* skipping a slash */
            if ( ( s = strstr( t, "/" ) )  == NULL ) { /*leaf node */
                if ( ( s = strstr( t, "." ) ) == NULL ) { /* no dot-extender something wrong here */
                    /***	  rodsLog (LOG_NOTICE,
                      "stageMssoStructFile: File seems to have no extension: %s", t);
                      return(SYS_STRUCT_FILE_PATH_ERR); ***/
                    return( 0 );
                }
                s++;
                if ( !strcmp( s, MSSO_MPF_FILE_STR ) ) {
                    fileType = 2;    /* mpf file */
                }
                else if ( !strcmp( s, MSSO_RUN_FILE_STR ) ) {
                    fileType = 3;    /* run file */
                }
                else {/* a directory lookup of a run directory */
                    fileType = 4;
                }
            }
            else { /* deeper node should be of type directory below */
                fileType = 4;
            }
        }
        else {
            return( SYS_STRUCT_FILE_PATH_ERR );
        }
#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "In 1 stageMssoSubStructFile:fileType=%d", fileType );
#endif /* MSSO_DEBUG */


        if ( fileType == 1 ) {
            if ( strlen( specColl->cacheDir ) == 0 ) {
                /* no cache. stage one. make the CacheDir first */
                status = mkMssoCacheDir( structFileInx, subFile );
                if ( status < 0 ) { return status; }
                /**********
                  status = extractMssoFile (structFileInx, subFile);
                  if (status < 0) {
                  rodsLog (LOG_NOTICE,
                  "stageMssoStructFile:extract error for %s in cacheDir %s,errno=%d",
                  specColl->objPath, specColl->cacheDir, errno);
                  return SYS_MSSO_STRUCT_FILE_EXTRACT_ERR - errno;
                  }
                 ******************/
                /* register the CacheDir */
                status = modCollInfo2( rsComm, specColl, 0 ); /*#######*/
                if ( status < 0 ) { return status; }
            }
        }
        else if ( fileType == 2 ) {
            if ( strlen( specColl->cacheDir ) == 0 ) {
                /* no cache. stage one. make the CacheDir first */
                status = mkMssoCacheDir( structFileInx, subFile );
                if ( status < 0 ) { return status; }
                /* register the CacheDir */
                status = modCollInfo2( rsComm, specColl, 0 ); /*#######*/
                if ( status < 0 ) { return status; }
            }

            /* create a run file */
            status = mkMssoMpfRunFile( structFileInx, subFile );
            return( status );
        }
        else if ( fileType == 3 ) { /* run */
            /* craete a run dir */
            /*** moved into prepareForExecution ****
              status = mkMssoMpfRunDir(structFileInx, subFile, runDir);
              if (status < 0)
              return(status);
             ***** RAJA Aug 16,2012 ****/
            /* perform the run */
            status = extractMssoFile( structFileInx, subFile, runDir, showFiles );
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "stageMssoStructFile:extract error for %s in cacheDir %s,errno=%d",
                         specColl->objPath, runDir, errno );
                return SYS_MSSO_STRUCT_FILE_EXTRACT_ERR - errno;
            }
            return( status );
            /* return any results */
            if ( strlen( showFiles ) != 0 ) { /* replace subFile by showFile */
                rstrcpy( subFile->subFilePath, showFiles, MAX_NAME_LEN );
            }
        }
        else { /* fileType == 4   looking at run results*/

        }
        return ( 0 );
    }


    int
    mkMssoMpfRunDir( int structFileInx, subFile_t *subFile, char *runDir ) {
        int i = 0;
        int status;
        fileMkdirInp_t fileMkdirInp;
        int rescTypeInx;
        rescInfo_t *rescInfo;
        rsComm_t *rsComm = StructFileDesc[structFileInx].rsComm;
        specColl_t *specColl = StructFileDesc[structFileInx].specColl;
        char mpfFileName[NAME_LEN];
        struct stat statbuf;


#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering mkMssoMpfRunDir" );
#endif /* MSSO_DEBUG */

        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE, "mkMssoMpfRunDir: NULL specColl input" );
            return ( SYS_INTERNAL_NULL_INPUT_ERR );
        }

        rescInfo = StructFileDesc[structFileInx].rescInfo;

        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        rescTypeInx = rescInfo->rescTypeInx;
        fileMkdirInp.fileType = UNIX_FILE_TYPE;     /* the only type for run */
        fileMkdirInp.mode = DEFAULT_DIR_MODE;
        rstrcpy( fileMkdirInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN );

        status = getMpfFileName( subFile, mpfFileName );
        if ( status < 0 ) {
            return( status );
        }

        snprintf( runDir, MAX_NAME_LEN, "%s/%s.%s",
                  specColl->cacheDir, mpfFileName, MSSO_RUN_DIR_STR );
        strncpy( fileMkdirInp.dirName, runDir, MAX_NAME_LEN );
        oldRunDirTime = -1;
        newRunDirName[0] = '\0';
        status = rsFileMkdir( rsComm, &fileMkdirInp );
        if ( status >= 0 ) {
            return( 0 );
        }
        if ( getErrno( status ) != EEXIST ) {
            return( status );
        }
        /* runDir exists */
        /* first get the time stamp for oldRunDir */
        status = stat( runDir, &statbuf );
        if ( status != 0 ) {
            rodsLog( LOG_ERROR,
                     "mkMssoMpfRunDir: stat error for %s, errno = %d", runDir, errno );
            return( UNIX_FILE_STAT_ERR );
        }
        oldRunDirTime = ( int ) statbuf.st_mtime;
        if ( noVersions == 0 ) {
            while ( 1 ) {
                snprintf( fileMkdirInp.dirName, MAX_NAME_LEN, "%s/%s.%s%d",
                          specColl->cacheDir, mpfFileName, MSSO_RUN_DIR_STR , i );
                status = rsFileMkdir( rsComm, &fileMkdirInp );
                if ( status >= 0 ) {
                    break;
                }
                else {
                    if ( getErrno( status ) == EEXIST ) {
                        i++;
                        continue;
                    }
                    else {
                        return ( status );
                    }
                }
            }
            /* version the files */
            /*** do this in extractMssoStructFile if new file is needed to be created
              snprintf(mvStr, sizeof mvStr, "mv -f %s/????* %s", runDir, fileMkdirInp.dirName);
              system(mvStr);
             ***/
            snprintf( newRunDirName,  sizeof newRunDirName, "%s", fileMkdirInp.dirName );
        }
        /*** do this in extractMssoStructFile if new file is needed to be created
          else {
          snprintf(mvStr, sizeof mvStr, "rm -rf %s/???*", runDir);
          system(mvStr);
          }
         ***/
        return( 0 );
    }

    int
    mkMssoMpfRunFile( int structFileInx, subFile_t *subFile ) {

        int status, i;
        fileCreateInp_t fileCreateInp;
        fileCloseInp_t fileCloseInp;
        int rescTypeInx;
        rescInfo_t *rescInfo;
        rsComm_t *rsComm = StructFileDesc[structFileInx].rsComm;
        specColl_t *specColl = StructFileDesc[structFileInx].specColl;
        char mpfFileName[NAME_LEN];

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering mkMssoMpfRunFile" );
#endif /* MSSO_DEBUG */

        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE, "mkMssoMpfRunFile: NULL specColl input" );
            return ( SYS_INTERNAL_NULL_INPUT_ERR );
        }

        rescInfo = StructFileDesc[structFileInx].rescInfo;

        memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );
        rescTypeInx = rescInfo->rescTypeInx;
        fileCreateInp.fileType = UNIX_FILE_TYPE;     /* the only type for run */
        fileCreateInp.mode = subFile->mode;
        fileCreateInp.flags = subFile->flags;
        rstrcpy( fileCreateInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN );

        status = getMpfFileName( subFile, mpfFileName );
        if ( status < 0 ) {
            return( status );
        }

        snprintf( fileCreateInp.fileName, MAX_NAME_LEN, "%s/%s.%s",
                  specColl->cacheDir, mpfFileName, MSSO_RUN_FILE_STR );
        fileCreateInp.otherFlags = NO_CHK_PERM_FLAG;
        status = rsFileCreate( rsComm, &fileCreateInp );
        if ( status < 0 ) {
            i = UNIX_FILE_CREATE_ERR - status;
            if ( i == EEXIST ) {
                return( 0 );    /* file already exists */
            }
            rodsLog( LOG_ERROR,
                     "mkMssoMpfRunFile: rsFileCreate for %s error, status = %d",
                     fileCreateInp.fileName, status );
            return status;
        }
        fileCloseInp.fileInx = status;
        rsFileClose( rsComm, &fileCloseInp );


        return( 0 );
    }

    int
    mkMssoCacheDir( int structFileInx, subFile_t *subFile ) {
        int i = 0;
        int status;
        fileMkdirInp_t fileMkdirInp;
        int rescTypeInx;
        rescInfo_t *rescInfo;
        rsComm_t *rsComm = StructFileDesc[structFileInx].rsComm;
        specColl_t *specColl = StructFileDesc[structFileInx].specColl;


#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering mkMssoCacheDir" );
#endif /* MSSO_DEBUG */

        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE, "mkMssoCacheDir: NULL specColl input" );
            return ( SYS_INTERNAL_NULL_INPUT_ERR );
        }

        rescInfo = StructFileDesc[structFileInx].rescInfo;

        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        rescTypeInx = rescInfo->rescTypeInx;
        fileMkdirInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        fileMkdirInp.mode = DEFAULT_DIR_MODE;
        rstrcpy( fileMkdirInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN );

        while ( 1 ) {
            snprintf( fileMkdirInp.dirName, MAX_NAME_LEN, "%s.%s%d",
                      specColl->phyPath, CACHE_DIR_STR, i );
            status = rsFileMkdir( rsComm, &fileMkdirInp );
            if ( status >= 0 ) {
                break;
            }
            else {
                if ( getErrno( status ) == EEXIST ) {
                    i++;
                    continue;
                }
                else {
                    return ( status );
                }
            }
        }
        rstrcpy( specColl->cacheDir, fileMkdirInp.dirName, MAX_NAME_LEN );

        return( 0 );
    }

    int
    writeBufToLocalFile( char *fName, char *buf ) {
        FILE *fd;

        /*  fd = fopen(fName, "w"); */
        fd = fopen( fName, "a" );
        if ( fd == NULL )  {
            rodsLog( LOG_ERROR,
                     "Cannot open rule file %s. ernro = %d\n", fName, errno );
            return( FILE_OPEN_ERR );
        }
        fprintf( fd, "%s", buf );
        fclose( fd );
        return( 0 );

    }

    int
    cleanOutStageArea( char *stageArea ) {
        int jj;
        char* t;
        char *s;
        char mvstr[MAX_NAME_LEN];
        /* cleaning files from "execution area can be blank separated" */

        jj = clnoutCnt;
        while ( clnoutCnt > 0 ) {
            clnoutCnt--;
            t = cleanOut[clnoutCnt];
            while ( ( s = strstr( t, " " ) ) !=  NULL ) {
                *s = '\0';
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, t );
                checkPhySafety( mvstr );
                snprintf( mvstr, MAX_NAME_LEN, "rm -rf %s/%s", stageArea, t );
                system( mvstr );
                s++;
                while ( *s == ' ' ) { s++; }
                t = s;
            }
            snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, t );
            checkPhySafety( mvstr );
            snprintf( mvstr, MAX_NAME_LEN, "rm -rf %s/%s", stageArea, t );
            system( mvstr );
            free( cleanOut[clnoutCnt] );
        }
        clnoutCnt = jj;
        return( 0 );

    }

    int
    extractMssoFile( int structFileInx, subFile_t *subFile, char *runDir, char *showFiles ) {

        int status, jj, i;
        specColl_t *specColl = StructFileDesc[structFileInx].specColl;
        char mpfFileName[NAME_LEN];
        char mpfFilePath[MAX_NAME_LEN];
        execMyRuleInp_t execMyRuleInp;
        msParamArray_t msParamArray;
        rsComm_t *rsComm = StructFileDesc[structFileInx].rsComm;
        msParamArray_t *outParamArray = NULL;
        msParam_t *mP;
        char *t;
        char mvstr[MAX_NAME_LEN];
        char stagefilename[MAX_NAME_LEN];
        dataObjInp_t dataObjInp;
        portalOprOut_t *portalOprOut = NULL;
        bytesBuf_t dataObjOutBBuf;
        rodsObjStat_t *rodsObjStatOut;
        int localTime;
        struct stat statbuf;
        int inputHasChanged = 0;
        char mvStr[MAX_NAME_LEN * 2];

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering extractMssoFile" );
#endif /* MSSO_DEBUG */

        if ( subFile == NULL || runDir == NULL ) {
            return( SYS_MSSO_STRUCT_FILE_EXTRACT_ERR );
        }

        status = getMpfFileName( subFile, mpfFileName );
        if ( status < 0 ) {
            return( status );
        }
        snprintf( mpfFilePath, MAX_NAME_LEN,  "%s/%s.mpf", specColl->cacheDir, mpfFileName );
        showFiles[0] = '\0';
        status = prepareForExecution( specColl->phyPath, mpfFilePath, runDir, showFiles,
                                      &execMyRuleInp, &msParamArray, structFileInx, subFile );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE, "Failure in extractMssoFile at prepareForExecution: %d\n",
                     status );
            return( status );
        }
        /*** Checking to see if one needs to really do run or use old files  ***/
        if ( cfcCnt > 0 ) {
            /* time of the oldRunDir is calculated elsewhere */
            jj = cfcCnt;
            while ( cfcCnt > 0 ) {
                cfcCnt--;
                if ( checkForChange[cfcCnt][0] == '/' ) { /* it is an irods collection ###### */
                    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
                    memset( &dataObjOutBBuf, 0, sizeof( dataObjOutBBuf ) );
                    snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s", checkForChange[cfcCnt] );
                    dataObjInp.openFlags = O_RDONLY;
                    snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, stagefilename );
                    status  = rsObjStat( rsComm, &dataObjInp,  &rodsObjStatOut );
                    if ( status < 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "Failure in extractMssoFile at rsDataObjStat when stating files checking execution: %d\n",
                                 status );
                        return ( status );
                    }
                    localTime = atoi( rodsObjStatOut->modifyTime );
                    rodsLog( LOG_NOTICE, "Timings for %s:rodsTime=%d and oldRunDirTime= %d",
                             dataObjInp.objPath, localTime, oldRunDirTime );
                    if ( localTime > oldRunDirTime ) {
                        inputHasChanged = 1;
                        break;
                    }
                }
                else { /* local directory or file */
                    snprintf( mvstr, MAX_NAME_LEN, "%s/%s", specColl->cacheDir, checkForChange[cfcCnt] );
                    checkPhySafety( mvstr );
                    status = stat( mvstr, &statbuf );
                    if ( status != 0 ) {
                        rodsLog( LOG_ERROR,
                                 "Failure in extractMssoFile at stat for %s, errno = %d", mvstr, errno );
                        return( UNIX_FILE_STAT_ERR );
                    }
                    localTime = ( int ) statbuf.st_mtime;
                    rodsLog( LOG_NOTICE, "Timings for %s:fileTime=%d and oldRunDirTime= %d",
                             mvstr, localTime, oldRunDirTime );
                    if ( localTime > oldRunDirTime ) {
                        inputHasChanged = 1;
                        break;
                    }
                }
            }
            cfcCnt = jj;
        }
        else {
            inputHasChanged = 1;
        }
        rodsLog( LOG_NOTICE, "Timings for InputChanged = %d and showFiles=%s", inputHasChanged, showFiles );
        if ( inputHasChanged == 0 ) {
            if ( newRunDirName[0] != '\0' ) {
                snprintf( mvStr, sizeof mvStr, "rmdir %s", newRunDirName );
                system( mvStr );
            }
            if ( strlen( showFiles ) != 0 ) {
                if ( strstr( showFiles, "stdout" ) != NULL ) {
                    snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stdout", runDir );
                }
                else if ( strstr( showFiles, "stderr" ) != NULL ) {
                    snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stderr", runDir );
                }
                else {
                    if ( ( mP = getMsParamByLabel( &msParamArray, showFiles ) ) != NULL ) {
                        snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s", ( char* ) mP->inOutStruct );
                    }
                }
            }
            else {
                snprintf( mvStr, sizeof mvStr, "%s/stdout", runDir );
                status = stat( runDir, &statbuf );
                if ( status == 0 )  {
                    snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stdout", runDir );
                }
                else {
                    snprintf( mvStr, sizeof mvStr, "%s/stderr", runDir );
                    status = stat( runDir, &statbuf );
                    if ( status == 0 )  {
                        snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stderr", runDir );
                    }
                }
            }
            return( 0 );
        }
        else { /* input has changed or new */
            /* do the moves if needed */
            if ( newRunDirName[0] != '\0' ) {
                snprintf( mvStr, sizeof mvStr, "mv -f %s/* %s", runDir, newRunDirName );
                system( mvStr );
            }
            else if ( oldRunDirTime > 0 ) {
                snprintf( mvStr, sizeof mvStr, "mkdir %s.backup", runDir );
                snprintf( mvStr, sizeof mvStr, "mv -f %s/* %s.backup", runDir, runDir );
                system( mvStr );
            }
        }
        /*** Checking to see if one needs to really do run or use old files  ***/


        if ( stageArea[0] == '\0' ) {
            strcpy( stageArea, CMD_DIR );
        }

        /* stage in files into execution area */
        jj = stinCnt;
        while ( stinCnt > 0 ) {
            stinCnt--;
            if ( ( t = strstr( stageIn[stinCnt], "=" ) ) == NULL ) { /* it is the last part of path */
                t = stageIn[stinCnt] + strlen( stageIn[stinCnt] ) - 1;
                while ( *t != '/' && t != stageIn[stinCnt] )  {
                    t--;
                }
                if ( *t == '/' ) { t++; } /*skip the slash if it exists */
                strncpy( stagefilename, t, MAX_NAME_LEN );
            }
            else {
                *t = '\0';
                t++;    /* stage file name starts here */
                while ( *t == ' ' ) { t++; } /* skip leading blanks */
                strncpy( stagefilename, t, MAX_NAME_LEN );
            }
            if ( stageIn[stinCnt][0] == '/' ) { /* it is an irods collection ###### */
                memset( &dataObjInp, 0, sizeof( dataObjInp ) );
                memset( &dataObjOutBBuf, 0, sizeof( dataObjOutBBuf ) );
                snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s", stageIn[stinCnt] );
                dataObjInp.openFlags = O_RDONLY;
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, stagefilename );
                status  = rsDataObjGet( rsComm, &dataObjInp, &portalOprOut, &dataObjOutBBuf );
                if ( status < 0 ) {
                    if ( portalOprOut != NULL ) { free( portalOprOut ); }
                    rodsLog( LOG_NOTICE,
                             "Failure in extractMssoFile at rsDataObjGet when copying files from iRODS into execution area: %d\n",
                             status );
                    return ( status );
                }
                if ( status == 0 || dataObjOutBBuf.len > 0 ) {
                    /* the buffer contains the file */
                    FILE *fd;
                    fd = fopen( mvstr, "w" );
                    if ( fd == NULL ) {
                        rodsLog( LOG_NOTICE,
                                 "extractMssoFile:  could not open file in stage area %s for writing:%d\n", mvstr );
                        if ( dataObjOutBBuf.buf != NULL ) {
                            free( dataObjOutBBuf.buf );
                        }
                        return( FILE_OPEN_ERR );
                    }
                    status = fwrite( dataObjOutBBuf.buf, 1, dataObjOutBBuf.len, fd );
                    if ( status != dataObjOutBBuf.len ) {
                        rodsLog( LOG_NOTICE,
                                 "extractMssoFile:  copy len error for  file in stage area %s for writing:%d, status=%d\n", mvstr,
                                 dataObjOutBBuf.len, status );
                        if ( dataObjOutBBuf.buf != NULL ) {
                            free( dataObjOutBBuf.buf );
                        }
                        return ( SYS_COPY_LEN_ERR );
                    }
                    fclose( fd );
                    if ( dataObjOutBBuf.buf != NULL ) {
                        free( dataObjOutBBuf.buf );
                    }
                }
                else { /* file is too large!!! */
                    rodsLog( LOG_NOTICE,
                             "extractMssoFile:  copy file  too large to get into stage area %s for writing:%d\n", mvstr );
                    return( USER_FILE_TOO_LARGE );
                }
            }
            else { /* local directory or file */
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", specColl->cacheDir, stageIn[stinCnt] );
                checkPhySafety( mvstr );
                snprintf( mvstr, MAX_NAME_LEN, "cp -rf  %s/%s %s/%s", specColl->cacheDir, stageIn[stinCnt], stageArea, stagefilename );
                system( mvstr );
            }
        }
        stinCnt = jj;
        /* stage in files into execution area */
#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Extracted Rule: %s\nIn Parameters:", execMyRuleInp.myRule );
        printMsParam( &msParamArray );
#endif /* MSSO_DEBUG */

        status = rsExecMyRule( rsComm, &execMyRuleInp, &outParamArray );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE, "Failure in extractMssoFile at rsExecMyRule: %d\n",
                     status );
            cleanOutStageArea( stageArea );
            return( status );
        }
#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Extracted Rule Finished: \nOut Parameters:" );
        printMsParam( outParamArray );
#endif /* MSSO_DEBUG */

        /* copy or stage out files begin */
        /* moving files from "execution area to irods" */
        jj = stoutCnt;
        while ( stoutCnt > 0 ) {
            stoutCnt--;
            if ( ( t = strstr( stageOut[stoutCnt], "=" ) ) == NULL ) {
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, stageOut[stoutCnt] );
                checkPhySafety( mvstr );
                snprintf( mvstr, MAX_NAME_LEN, "mv -f %s/%s %s/%s", stageArea, stageOut[stoutCnt], runDir, stageOut[stoutCnt] );
                /* now move the file */
                system( mvstr );
            }
            else {
                *t = '\0';
                t++;
                while ( *t == ' ' ) { t++; } /* skip blanks */
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, stageOut[stoutCnt] );
                checkPhySafety( mvstr );
                if ( *t == '/' ) { /* target is an irods object */
                    /* #### */
                    /* now remove the file */
                    snprintf( mvstr, MAX_NAME_LEN, "rm -rf %s/%s", stageArea, stageOut[stoutCnt] );
                    system( mvstr );
                }
                else {
                    snprintf( mvstr, MAX_NAME_LEN, "mv -f %s/%s %s/%s", stageArea, stageOut[stoutCnt], runDir, t );
                    /* now move the file */
                    system( mvstr );
                }
            }
            free( stageOut[stoutCnt] );
        }
        stoutCnt = jj;
        /* copying files from "execution area to irods" */
        jj = cpoutCnt;
        while ( cpoutCnt > 0 ) {
            cpoutCnt--;
            if ( ( t = strstr( copyToIrods[cpoutCnt], "=" ) ) == NULL ) {
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, copyToIrods[cpoutCnt] );
                checkPhySafety( mvstr );
                snprintf( mvstr, MAX_NAME_LEN, "cp -rf %s/%s %s/%s", stageArea, copyToIrods[cpoutCnt], runDir, copyToIrods[cpoutCnt] );
                /* now copy the file */
                system( mvstr );
            }
            else {
                *t = '\0';
                t++;
                while ( *t == ' ' ) { t++; } /* skip blanks */
                if ( *t == '/' ) { /* target is an irods object */
                    /* #### */
                }
                else {
                    snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, copyToIrods[cpoutCnt] );
                    checkPhySafety( mvstr );
                    snprintf( mvstr, MAX_NAME_LEN, "cp -rf %s/%s %s/%s", stageArea, copyToIrods[cpoutCnt], runDir, t );
                    /* now copy the file */
                    system( mvstr );
                }
            }
            free( copyToIrods[cpoutCnt] );
        }
        cpoutCnt = jj;
        /* copy or stage out files end */

        /* save stdout and stderr */
        if ( ( mP = getMsParamByLabel( outParamArray, "ruleExecOut" ) ) != NULL ) {
            execCmdOut_t *myExecCmdOut;
            char *buf;
            char fName[MAX_NAME_LEN];
            myExecCmdOut = ( execCmdOut_t* )mP->inOutStruct;
            buf = ( char* ) myExecCmdOut->stdoutBuf.buf;
            if ( buf != NULL && strlen( buf ) > 0 ) {
                sprintf( fName, "%s/stdout", runDir );
                writeBufToLocalFile( fName, buf );
                if ( strlen( showFiles ) == 0 ) {
                    strcpy( showFiles, "stdout" );
                }
            }
            buf = ( char * ) myExecCmdOut->stderrBuf.buf;
            if ( buf != NULL && strlen( buf ) > 0 ) {
                sprintf( fName, "%s/stderr", runDir );
                writeBufToLocalFile( fName, buf );
                if ( strlen( showFiles ) == 0 ) {
                    strcpy( showFiles, "stderr" );
                }
            }
        }
        if ( strlen( showFiles ) != 0 ) {
            if ( strstr( showFiles, "stdout" ) != NULL ) {
                snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stdout", runDir );
            }
            else if ( strstr( showFiles, "stderr" ) != NULL ) {
                snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stderr", runDir );
            }
            else {
                if ( ( mP = getMsParamByLabel( outParamArray, showFiles ) ) != NULL ) {
                    snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s", ( char* ) mP->inOutStruct );
                }
            }
        }


        /* what is the showfile */
        cleanOutStageArea( stageArea );
        i = popOutStackInfo();
        if ( i < 0 ) {
            rodsLog( LOG_ERROR, "PopOutStackInfo Error:%i\n", i );
            return( i );
        }


        return 0;
    }




    int
    syncCacheDirToMssofile( int structFileInx, int oprType ) {
        int status;
        int rescTypeInx;
        fileStatInp_t fileStatInp;
        rodsStat_t *fileStatOut = NULL;
        specColl_t *specColl = StructFileDesc[structFileInx].specColl;
        rsComm_t *rsComm = StructFileDesc[structFileInx].rsComm;

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering syncCacheDirToMssofile" );
#endif /* MSSO_DEBUG */


        /* bundle called here */

        /* register size change */
        memset( &fileStatInp, 0, sizeof( fileStatInp ) );
        rstrcpy( fileStatInp.fileName, specColl->phyPath, MAX_NAME_LEN );
        rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
        fileStatInp.fileType = ( fileDriverType_t )RescTypeDef[rescTypeInx].driverType;
        rstrcpy( fileStatInp.addr.hostAddr,
                 StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN );

        status = rsFileStat( rsComm, &fileStatInp, &fileStatOut );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "syncCacheDirToMssofile: rsFileStat error for %s, status = %d",
                     specColl->phyPath, status );
            return ( status );
        }

        if ( ( oprType & NO_REG_COLL_INFO ) == 0 ) {
            /* for bundle opr, done at datObjClose */
            status = regNewObjSize( rsComm, specColl->objPath, specColl->replNum,
                                    fileStatOut->st_size );
        }

        free( fileStatOut );

        return ( status );
    }


    int
    initMssoSubFileDesc() {
        memset( MssoSubFileDesc, 0,
                sizeof( mssoSubFileDesc_t ) * NUM_MSSO_SUB_FILE_DESC );
        return ( 0 );
    }

    int
    allocMssoSubFileDesc() {
        int i;

        for ( i = 1; i < NUM_MSSO_SUB_FILE_DESC; i++ ) {
            if ( MssoSubFileDesc[i].inuseFlag == FD_FREE ) {
                MssoSubFileDesc[i].inuseFlag = FD_INUSE;
                return ( i );
            };
        }

        rodsLog( LOG_NOTICE,
                 "allocMssoSubFileDesc: out of MssoSubFileDesc" );

        return ( SYS_OUT_OF_FILE_DESC );
    }

    int
    freeMssoSubFileDesc( int mssoSubFileInx ) {
        if ( mssoSubFileInx < 0 || mssoSubFileInx >= NUM_MSSO_SUB_FILE_DESC ) {
            rodsLog( LOG_NOTICE,
                     "freeMssoSubFileDesc: mssoSubFileInx %d out of range", mssoSubFileInx );
            return ( SYS_FILE_DESC_OUT_OF_RANGE );
        }

        memset( &MssoSubFileDesc[mssoSubFileInx], 0, sizeof( mssoSubFileDesc_t ) );

        return ( 0 );
    }


    /* getMssoSubStructFilePhyPath - get the phy path in the cache dir */
    int
    getMssoSubStructFilePhyPath( char *phyPath, specColl_t *specColl,
                                 char *subFilePath ) {
        int len;

        /* subFilePath is composed by appending path to the objPath which is
         * the logical path of the tar file. So we need to substitude the
         * objPath with cacheDir */

        len = strlen( specColl->cacheDir );
        if ( strncmp( specColl->cacheDir, subFilePath, len ) == 0 ) {
            snprintf( phyPath, MAX_NAME_LEN, "%s", subFilePath );
        }
        else {
            len = strlen( specColl->collection );
            if ( strncmp( specColl->collection, subFilePath, len ) != 0 ) {
                rodsLog( LOG_NOTICE,
                         "getMssoSubStructFilePhyPath: collection %s subFilePath %s mismatch",
                         specColl->collection, subFilePath );
                return ( SYS_STRUCT_FILE_PATH_ERR );
            }

            snprintf( phyPath, MAX_NAME_LEN, "%s%s", specColl->cacheDir,
                      subFilePath + len );
        }
#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Path generated by getMssoSubStructFilePhyPath:%s", phyPath );
#endif /* MSSO_DEBUG */


        return ( 0 );
    }

    int
    getMpfFileName( subFile_t *subFile, char *mpfFileName ) {

        char *p, *t, *s;
        char c;
        p = subFile->subFilePath;
        if ( ( t = strstr( p, ".mpf" ) ) == NULL && ( t = strstr( p, ".run" ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "getMpfFileName: path name of different type:%s", p );
            return( SYS_STRUCT_FILE_PATH_ERR );
        }
        c = *t;
        *t = '\0';
        s = t;
        while ( s != p ) {
            s--;
            if ( *s == '/' ) {
                break;
            }
        }
        s++;
        rstrcpy( mpfFileName, s, NAME_LEN );
        *t = c;
        return( 0 );
    }


    int
    prepareForExecution( char *inRuleFile, char *inParamFile, char *runDir, char *showFiles, execMyRuleInp_t *execMyRuleInp,
                         msParamArray_t *msParamArray, int structFileInx, subFile_t *subFile ) {

        FILE *fptr;
        int len, i, j, status;
        int gotRule = 0;
        char buf[META_STR_LEN * 3];
        char *inParamName[1024];
        char *tmpPtr, *tmpPtr2;
        msParam_t *mP;

        i = pushOnStackInfo();
        if ( i < 0 ) {
            rodsLog( LOG_ERROR, "PushOnStackInfo Error:%i\n", i );
            return( i );
        }


        stinCnt = 0;
        stoutCnt = 0;
        cpoutCnt = 0;
        cfcCnt = 0;
        clnoutCnt = 0;
        stageArea[0] = '\0';
        noVersions = 0;

        memset( execMyRuleInp, 0, sizeof( execMyRuleInp_t ) );
        memset( msParamArray, 0, sizeof( msParamArray_t ) );
        execMyRuleInp->inpParamArray = msParamArray;
        execMyRuleInp->condInput.len = 0;
        strcpy( execMyRuleInp->outParamDesc, "ruleExecOut" );
        fptr = fopen( inRuleFile, "r" );
        if ( fptr == NULL ) {
            rodsLog( LOG_ERROR,
                     "Cannot open rule file %s. ernro = %d\n", inRuleFile, errno );
            return( FILE_OPEN_ERR );
        }
        while ( ( len = getLine( fptr, buf, META_STR_LEN ) ) > 0 ) {
            gotRule = 0;
            if ( buf[0] == '#' ) {
                continue;
            }
            if ( startsWith( buf, "INPUT" ) || startsWith( buf, "input" ) ) {
                gotRule = 1;
                trimSpaces( trimPrefix( buf ) );
            }
            else if ( startsWith( buf, "OUTPUT" ) || startsWith( buf, "output" ) ) {
                gotRule = 2;
                trimSpaces( trimPrefix( buf ) );
            }
            else if ( startsWith( buf, "SHOW" ) || startsWith( buf, "show" ) ) {
                gotRule = 3;
            }

            if ( gotRule == 0 ) {
                /*      snprintf (execMyRuleInp->myRule + strlen(execMyRuleInp->myRule),
                        META_STR_LEN - strlen(execMyRuleInp->myRule), "%s in file %s\n", buf, inRuleFile); */
                snprintf( execMyRuleInp->myRule + strlen( execMyRuleInp->myRule ),
                          META_STR_LEN - strlen( execMyRuleInp->myRule ), "%s", buf );
            }
            else if ( gotRule == 1 ) {
                if ( convertListToMultiString( buf, 1 ) != 0 ) {
                    rodsLog( LOG_ERROR,
                             "Input parameter list format error for %s\n", buf );
                    return( INPUT_ARG_NOT_WELL_FORMED_ERR );
                }
                parseMsParamFromIRFile( msParamArray, buf );
            }
            else if ( gotRule == 2 ) {
                /*      if(convertListToMultiString(buf, 0)!=0) {
                        rodsLog (LOG_ERROR,
                        "Output parameter list format error for %s\n", myRodsArgs.fileString);
                        return(INPUT_ARG_NOT_WELL_FORMED_ERR);
                        }
                        if (strcmp (buf, "null") != 0) {
                        rstrcpy (execMyRuleInp.outParamDesc, buf, LONG_NAME_LEN);
                        }
                 */
                continue;
            }
            else if ( gotRule == 3 ) { /* SHOW */
                trimSpaces( trimPrefix( buf + 4 ) );
                rstrcpy( showFiles, buf + 4, MAX_NAME_LEN );
            }
            else {
                break;
            }
        }
        fclose( fptr );

        fptr = fopen( inParamFile, "r" );
        if ( fptr == NULL ) {
            rodsLog( LOG_ERROR,
                     "Cannot open param file %s. ernro = %d\n", inParamFile, errno );
            return( FILE_OPEN_ERR );
        }
        j = 0;
        while ( ( len = getLine( fptr, buf, META_STR_LEN ) ) > 0 )     {
            if ( startsWith( buf, "INPARAM " ) || startsWith( buf, "inparam" ) )   {
                trimSpaces( trimPrefix( buf ) );
                if ( ( tmpPtr = strstr( buf, "=" ) ) != NULL ) {
                    *tmpPtr = '\0';
                    tmpPtr++;
                    if ( ( mP = getMsParamByLabel( execMyRuleInp->inpParamArray, buf ) ) != NULL ) {
                        if ( mP->inOutStruct != NULL ) {
                            free( mP->inOutStruct );
                        }
                        mP->inOutStruct = strdup( tmpPtr );
                    }
                    else {
                        addMsParam( execMyRuleInp->inpParamArray, buf, STR_MS_T,
                                    strdup( tmpPtr ), NULL );
                    }
                }
            }
            else if ( startsWith( buf, "FILEPARAM" ) || startsWith( buf, "DIRPARAM" ) ||
                      startsWith( buf, "fileparam" ) || startsWith( buf, "dirparam" ) ) {
                trimSpaces( trimPrefix( buf ) );
                inParamName[j] = strdup( buf );
                j++;
            }
            else if ( startsWith( buf, "STAGEIN" ) || startsWith( buf, "stagein" ) ) {
                trimSpaces( trimPrefix( buf ) );
                stageIn[stinCnt] = strdup( buf );
                stinCnt++;
            }
            else if ( startsWith( buf, "STAGEOUT" ) || startsWith( buf, "stageout" ) ) {
                trimSpaces( trimPrefix( buf ) );
                stageOut[stoutCnt] = strdup( buf );
                stoutCnt++;
            }
            else if ( startsWith( buf, "COPYOUT" ) || startsWith( buf, "copyout" ) ) {
                trimSpaces( trimPrefix( buf ) );
                copyToIrods[cpoutCnt] = strdup( buf );
                cpoutCnt++;
            }
            else if ( startsWith( buf, "CLEANOUT" ) || startsWith( buf, "cleanout" ) ) {
                trimSpaces( trimPrefix( buf ) );
                cleanOut[clnoutCnt] = strdup( buf );
                clnoutCnt++;
            }
            else if ( startsWith( buf, "STAGEAREA" ) || startsWith( buf, "stagearea" ) ) {
                trimSpaces( trimPrefix( buf ) );
                strncpy( stageArea, buf, MAX_NAME_LEN );
            }
            else if ( startsWith( buf, "NOVERSIONS" ) || startsWith( buf, "noversions" ) ) {
                noVersions = 1;
            }
            else if ( startsWith( buf, "CHECKFORCHANGE" ) || startsWith( buf, "checkforchange" ) ) {
                trimSpaces( trimPrefix( buf ) );
                checkForChange[cfcCnt] = strdup( buf );
                cfcCnt++;
            }
        }
        fclose( fptr );

        status = mkMssoMpfRunDir( structFileInx, subFile, runDir );
        if ( status < 0 ) {
            return( status );
        }


        /* prefix  file and directory names with  physical pathnames */
        for ( i = 0; i < j ; i++ ) {
            if ( ( mP = getMsParamByLabel( execMyRuleInp->inpParamArray, inParamName[i] ) ) != NULL ) {
                if ( mP->inOutStruct != NULL ) {

                    tmpPtr2 = ( char * ) mP->inOutStruct;

                    /* check for sefaty onlu  if this a fullpath value file. no prefixing needed */
                    if ( ( *tmpPtr2 == '\"' &&  *( tmpPtr2 + 1 ) == '/' ) || *tmpPtr2 == '/' ) {
                        status = checkSafety( tmpPtr2 );
                        if ( status < 0 )  {
                            return( status );
                        }
                        continue;
                    }
                    len = strlen( tmpPtr2 );
                    tmpPtr = ( char * ) malloc( len + strlen( runDir ) + 10 );
                    if ( len != 0 ) {
                        tmpPtr2++;  /* skipping the leading double-quote */
                        sprintf( tmpPtr, "\"%s/%s", runDir, tmpPtr2 );
                    }
                    else {
                        sprintf( tmpPtr, "\"%s\"", runDir );
                    }
                    free( mP->inOutStruct );
                    mP->inOutStruct = tmpPtr;
                }
                else {
                    tmpPtr = ( char * ) malloc( strlen( runDir ) + 10 );
                    sprintf( tmpPtr, "\"%s\"", runDir );
                    mP->inOutStruct = tmpPtr;
                }
            }
            /* removing truncation of inParamFile name */
            free( inParamName[i] );
        }
        return( 0 );
    }

    // =-=-=-=-=-=-=-
    // helper function to check incoming parameters
    inline irods::error msso_check_params(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // ask the context if it is valid
        irods::error ret = _ctx.valid< irods::structured_object >();
        if ( !ret.ok() ) {
            return PASSMSG( "resource context is invalid", ret );

        }

        return SUCCESS();

    } // msso_check_params

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    irods::error msso_file_create_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = tar_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );
        int structFileInx = 0;
        int subInx        = 0;
        int rescTypeInx   = 0;
        int status        = 0;

#ifdef MSSO_DEBUG
        rodsLog(
            LOG_NOTICE,
            "Entering mssoSubStructFileCreate with file:%s",
            fco->spec_coll()->objPath );
#endif /* MSSO_DEBUG */


        specColl_t* specColl = fco->spec_coll();
        structFileInx = rsMssoStructFileOpen( _ctx->comm, specColl, subFile, 1 );

        if ( structFileInx < 0 ) {
            rodsLog( LOG_NOTICE,
                     "mssoSubStructFileCreate: rsMssoStructFileOpen error for %s, stat=%d",
                     specColl->objPath, structFileInx );
            return ( structFileInx );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = StructFileDesc[structFileInx].specColl;

        subInx = allocMssoSubFileDesc();

        if ( subInx < 0 ) { return subInx; }

        MssoSubFileDesc[subInx].structFileInx = structFileInx;

        fileCreateInp_t fileCreateInp;
        memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );
        status = getMssoSubStructFilePhyPath( fileCreateInp.fileName, specColl,
                                              subFile->subFilePath );
        if ( status < 0 ) { return status; }

        rescTypeInx = StructFileDesc[structFileInx].rescInfo->rescTypeInx;
        fileCreateInp.fileType = UNIX_FILE_TYPE;	/* the only type for cache */
        rstrcpy( fileCreateInp.addr.hostAddr,
                 StructFileDesc[structFileInx].rescInfo->rescLoc, NAME_LEN );
        fileCreateInp.mode = subFile->mode;
        fileCreateInp.flags = subFile->flags;
        fileCreateInp.otherFlags = NO_CHK_PERM_FLAG;

        status = rsFileCreate( _ctx->comm, &fileCreateInp );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "specCollCreate: rsFileCreate for %s error, status = %d",
                     fileCreateInp.fileName, status );
            return status;
        }
        else {
            MssoSubFileDesc[subInx].fd = status;
            StructFileDesc[structFileInx].openCnt++;
            return ( subInx );
        }

    } // msso_file_create_plugin


}; // namespace irods



