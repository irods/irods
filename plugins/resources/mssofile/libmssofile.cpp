#include "rods.hpp"
#include "rsGlobal.hpp"
//#include "structFileDriver.hpp"
#include "execMyRule.hpp"
#include "apiHeaderAll.hpp"
#include "fileOpr.hpp"
#include "specColl.hpp"
#include "fileMkdir.hpp"
#include "irods_structured_object.hpp"
#include "irods_resource_backport.hpp"
#include "irods_stacktrace.hpp"


#define NUM_MSSO_SUB_FILE_DESC 1024
#define MSSO_CACHE_DIR_STR "mssoCacheDir"
#define MSSO_RUN_DIR_STR "runDir"
#define MSSO_RUN_FILE_STR "run"
#define MSSO_MPF_FILE_STR "mpf"

extern "C" {
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

    // =-=-=-=-=-=-=-
    // structures and defines
    typedef struct structFileDesc {
        int inuseFlag;
        rsComm_t *rsComm;
        specColl_t *specColl;
        int openCnt;
        char dataType[NAME_LEN]; // JMC - backport 4634
        char location[NAME_LEN];
    } structFileDesc_t;

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

    // =-=-=-=-=-=-=-
    // find the next free MssoStructFileDesc slot, mark it in use and return the index
    int alloc_struct_file_desc() {
        for ( int i = 1; i < NUM_STRUCT_FILE_DESC; i++ ) {
            if ( MssoStructFileDesc[i].inuseFlag == FD_FREE ) {
                MssoStructFileDesc[i].inuseFlag = FD_INUSE;
                return i;
            };
        } // for i

        return SYS_OUT_OF_FILE_DESC;

    } // alloc_struct_file_desc

    int free_struct_file_desc( int _idx ) {
        if ( _idx  < 0 || _idx  >= NUM_STRUCT_FILE_DESC ) {
            rodsLog( LOG_NOTICE,
                     "free_struct_file_desc: index %d out of range", _idx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        memset( &MssoStructFileDesc[ _idx ], 0, sizeof( structFileDesc_t ) );

        return 0;

    } // free_struct_file_desc

    int
    writeBufToLocalFile( char *fName, char *buf ) {
        FILE *fd;

        /*  fd = fopen(fName, "w"); */
        fd = fopen( fName, "a" );
        if ( fd == NULL )  {
            rodsLog( LOG_ERROR,
                     "Cannot open rule file %s. ernro = %d\n", fName, errno );
            return FILE_OPEN_ERR;
        }
        fprintf( fd, "%s", buf );
        fclose( fd );
        return 0;

    }


    int
    checkPhySafety( char* ) {
        /*  need to see if path is good. it should be only  a logical file */
        /* it may be file in another zone also!!! */
        return 0;
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
                if ( system( mvstr ) ) {}
                s++;
                while ( *s == ' ' ) {
                    s++;
                }
                t = s;
            }
            snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, t );
            checkPhySafety( mvstr );
            snprintf( mvstr, MAX_NAME_LEN, "rm -rf %s/%s", stageArea, t );
            if ( system( mvstr ) ) {}
            free( cleanOut[clnoutCnt] );
        }
        clnoutCnt = jj;
        return 0;

    }


    int checkSafety( char* ) {
        /*  need to see if path is good. it should be only  a logical file */
        /* it may be file in another zone also!!! */
        return 0;
    }


    int pushOnStackInfo() {
        int  i, k;

        if ( MssoSubFileStackTop == 0 ) {
            MssoSubFileStackTop++;
            return 0;
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

        snprintf( MssoSubFileStack[k].newRunDirName, sizeof( MssoSubFileStack[k].newRunDirName ), "%s", newRunDirName );
        snprintf( MssoSubFileStack[k].stageArea, sizeof( MssoSubFileStack[k].stageArea ), "%s", stageArea );
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

        return 0;
    }

    int popOutStackInfo() {
        int i, k;
        MssoSubFileStackTop--;
        if ( MssoSubFileStackTop == 0 ) {
            return 0;
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

        snprintf( newRunDirName, sizeof( newRunDirName ), "%s", MssoSubFileStack[k].newRunDirName );
        snprintf( stageArea, sizeof( stageArea ), "%s", MssoSubFileStack[k].stageArea );
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


        return 0;


    }

    int
    matchMssoStructFileDesc( specColl_t *specColl ) {
        int i;

        for ( i = 1; i < NUM_STRUCT_FILE_DESC; i++ ) {
            if ( MssoStructFileDesc[i].inuseFlag == FD_INUSE &&
                    MssoStructFileDesc[i].specColl != NULL &&
                    strcmp( MssoStructFileDesc[i].specColl->collection, specColl->collection )	    == 0 &&
                    strcmp( MssoStructFileDesc[i].specColl->objPath, specColl->objPath )
                    == 0 ) {
                return i;
            };
        }

        return SYS_OUT_OF_FILE_DESC;

    }


    int
    isFakeFile( char *coll, char*objPath ) {
        char s[MAX_NAME_LEN];

        sprintf( s, "%s/myFakeFile", coll );
        if ( !strcmp( objPath, s ) ) {
            return 1;
        }
        else {
            return 0;
        }
    }

    int mkMssoCacheDir( int _struct_inx ) {
        int i = 0;
        int status;
        fileMkdirInp_t fileMkdirInp;
        rsComm_t*   rsComm   = MssoStructFileDesc[_struct_inx].rsComm;
        specColl_t* specColl = MssoStructFileDesc[_struct_inx].specColl;

        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE, "mkMssoCacheDir: NULL specColl input" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        fileMkdirInp.mode = DEFAULT_DIR_MODE;
        rstrcpy( fileMkdirInp.addr.hostAddr,
                 MssoStructFileDesc[_struct_inx].location,
                 NAME_LEN );

        while ( 1 ) {
            snprintf( fileMkdirInp.dirName, MAX_NAME_LEN, "%s.%s%d",
                      specColl->phyPath, CACHE_DIR_STR, i );
            snprintf( fileMkdirInp.rescHier, sizeof( fileMkdirInp.rescHier ), "%s", specColl->rescHier );
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
                    return status;
                }
            }
        }

        rstrcpy( specColl->cacheDir, fileMkdirInp.dirName, MAX_NAME_LEN );

        return 0;
    }

    int getMpfFileName(
        irods::structured_object_ptr _fco,
        char*                        mpfFileName ) {
        char *p, *t, *s;
        char c;
        std::string sub_file_path;
        sub_file_path = _fco->sub_file_path();
        p = ( char* )sub_file_path.c_str();
        if ( ( t = strstr( p, ".mpf" ) ) == NULL && ( t = strstr( p, ".run" ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "getMpfFileName: path name of different type:%s", p );
            return SYS_STRUCT_FILE_PATH_ERR;
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
        return 0;
    }

    int mkMssoMpfRunFile(
        int                          _struct_inx,
        irods::structured_object_ptr _fco ) {
        int status, i;
        fileCreateInp_t fileCreateInp;
        fileCloseInp_t fileCloseInp;
        rsComm_t *rsComm = MssoStructFileDesc[_struct_inx].rsComm;
        specColl_t *specColl = MssoStructFileDesc[_struct_inx].specColl;
        char mpfFileName[NAME_LEN];

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering mkMssoMpfRunFile" );
#endif /* MSSO_DEBUG */

        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE, "mkMssoMpfRunFile: NULL specColl input" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );
        fileCreateInp.mode  = _fco->mode();
        fileCreateInp.flags = _fco->flags();
        rstrcpy( fileCreateInp.addr.hostAddr,
                 MssoStructFileDesc[_struct_inx].location,
                 NAME_LEN );

        status = getMpfFileName( _fco, mpfFileName );
        if ( status < 0 ) {
            return status;
        }

        snprintf( fileCreateInp.fileName, MAX_NAME_LEN, "%s/%s.%s",
                  specColl->cacheDir, mpfFileName, MSSO_RUN_FILE_STR );
        fileCreateInp.otherFlags = NO_CHK_PERM_FLAG;
        snprintf( fileCreateInp.resc_hier_, sizeof( fileCreateInp.resc_hier_ ), "%s", specColl->rescHier );

        fileCreateOut_t* create_out = NULL;
        status = rsFileCreate( rsComm, &fileCreateInp, &create_out );
        free( create_out );
        if ( status < 0 ) {
            i = UNIX_FILE_CREATE_ERR - status;
            if ( i == EEXIST ) {
                return 0;   /* file already exists */
            }
            rodsLog( LOG_ERROR,
                     "mkMssoMpfRunFile: rsFileCreate for %s error, status = %d",
                     fileCreateInp.fileName, status );
            return status;
        }
        fileCloseInp.fileInx = status;
        rsFileClose( rsComm, &fileCloseInp );


        return 0;
    }

    int
    mkMssoMpfRunDir(
        int                          structFileInx,
        irods::structured_object_ptr _fco,
        char*                        runDir ) {
        int i = 0;
        int status;
        rsComm_t*   rsComm   = MssoStructFileDesc[structFileInx].rsComm;
        specColl_t* specColl = MssoStructFileDesc[structFileInx].specColl;
        char mpfFileName[NAME_LEN];
        struct stat statbuf;


#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering mkMssoMpfRunDir" );
#endif /* MSSO_DEBUG */

        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE, "mkMssoMpfRunDir: NULL specColl input" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        fileMkdirInp_t fileMkdirInp;
        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        fileMkdirInp.mode = DEFAULT_DIR_MODE;
        rstrcpy( fileMkdirInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );

        status = getMpfFileName( _fco, mpfFileName );
        if ( status < 0 ) {
            return status;
        }

        snprintf( runDir, MAX_NAME_LEN, "%s/%s.%s",
                  specColl->cacheDir, mpfFileName, MSSO_RUN_DIR_STR );
        snprintf( fileMkdirInp.dirName, sizeof( fileMkdirInp.dirName ), "%s", runDir );
        oldRunDirTime = -1;
        newRunDirName[0] = '\0';
        snprintf( fileMkdirInp.rescHier, sizeof( fileMkdirInp.rescHier ), "%s", specColl->rescHier );
        status = rsFileMkdir( rsComm, &fileMkdirInp );
        if ( status >= 0 ) {
            return 0;
        }
        if ( getErrno( status ) != EEXIST ) {
            return status;
        }
        /* runDir exists */
        /* first get the time stamp for oldRunDir */
        status = stat( runDir, &statbuf );
        if ( status != 0 ) {
            rodsLog( LOG_ERROR,
                     "mkMssoMpfRunDir: stat error for %s, errno = %d", runDir, errno );
            return UNIX_FILE_STAT_ERR;
        }
        oldRunDirTime = ( int ) statbuf.st_mtime;
        if ( noVersions == 0 ) {
            while ( 1 ) {
                snprintf( fileMkdirInp.dirName, MAX_NAME_LEN, "%s/%s.%s%d",
                          specColl->cacheDir, mpfFileName, MSSO_RUN_DIR_STR , i );
                snprintf( fileMkdirInp.rescHier, sizeof( fileMkdirInp.rescHier ), "%s", specColl->rescHier );
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
                        return status;
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
        return 0;
    }

    int
    prepareForExecution(
        char*            inRuleFile,
        char*            inParamFile,
        char*            runDir,
        char*            showFiles,
        execMyRuleInp_t* execMyRuleInp,
        msParamArray_t*  msParamArray,
        int              structFileInx,
        irods::structured_object_ptr _fco ) {

        int status = pushOnStackInfo();
        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "PushOnStackInfo Error:%i\n", status );
            return status;
        }


        memset( execMyRuleInp, 0, sizeof( execMyRuleInp_t ) );
        memset( msParamArray, 0, sizeof( msParamArray_t ) );
        execMyRuleInp->inpParamArray = msParamArray;
        execMyRuleInp->condInput.len = 0;
        strcpy( execMyRuleInp->outParamDesc, "ruleExecOut" );
        FILE *fptr = fopen( inRuleFile, "r" );
        if ( fptr == NULL ) {
            rodsLog( LOG_ERROR,
                     "Cannot open rule file %s. ernro = %d\n", inRuleFile, errno );
            return FILE_OPEN_ERR;
        }
        int len;
        char buf[META_STR_LEN * 3];
        while ( ( len = getLine( fptr, buf, META_STR_LEN ) ) > 0 ) {
            if ( buf[0] == '#' ) {
                continue;
            }
            if ( startsWith( buf, "INPUT" ) || startsWith( buf, "input" ) ) {
                trimSpaces( trimPrefix( buf ) );
                if ( convertListToMultiString( buf, 1 ) != 0 ) {
                    rodsLog( LOG_ERROR,
                             "Input parameter list format error for %s\n", buf );
                    fclose( fptr );
                    return INPUT_ARG_NOT_WELL_FORMED_ERR;
                }
                parseMsParamFromIRFile( msParamArray, buf );
            }
            else if ( startsWith( buf, "OUTPUT" ) || startsWith( buf, "output" ) ) {
                trimSpaces( trimPrefix( buf ) );
                /*      if(convertListToMultiString(buf, 0)!=0) {
                        rodsLog (LOG_ERROR,
                        "Output parameter list format error for %s\n", myRodsArgs.fileString);
                        return INPUT_ARG_NOT_WELL_FORMED_ERR;
                        }
                        if (strcmp (buf, "null") != 0) {
                        rstrcpy (execMyRuleInp.outParamDesc, buf, LONG_NAME_LEN);
                        }
                 */
                continue;
            }
            else if ( startsWith( buf, "SHOW" ) || startsWith( buf, "show" ) ) {
                trimSpaces( trimPrefix( buf + 4 ) );
                rstrcpy( showFiles, buf + 4, MAX_NAME_LEN );
            }
            else {
                /*      snprintf (execMyRuleInp->myRule + strlen(execMyRuleInp->myRule),
                        META_STR_LEN - strlen(execMyRuleInp->myRule), "%s in file %s\n", buf, inRuleFile); */
                snprintf( execMyRuleInp->myRule + strlen( execMyRuleInp->myRule ),
                          META_STR_LEN - strlen( execMyRuleInp->myRule ), "%s", buf );
            }
        }
        fclose( fptr );

        fptr = fopen( inParamFile, "r" );
        if ( fptr == NULL ) {
            rodsLog( LOG_ERROR,
                     "Cannot open param file %s. ernro = %d\n", inParamFile, errno );
            return FILE_OPEN_ERR;
        }

        size_t paramCount = 0;
        char *inParamName[1024];
        stinCnt = 0;
        stoutCnt = 0;
        cpoutCnt = 0;
        cfcCnt = 0;
        clnoutCnt = 0;
        stageArea[0] = '\0';
        noVersions = 0;

        while ( ( len = getLine( fptr, buf, META_STR_LEN ) ) > 0 )     {
            if ( startsWith( buf, "INPARAM " ) || startsWith( buf, "inparam" ) )   {
                trimSpaces( trimPrefix( buf ) );
                char * tmpPtr;
                if ( ( tmpPtr = strstr( buf, "=" ) ) != NULL ) {
                    *tmpPtr = '\0';
                    tmpPtr++;
                    if ( msParam_t * mP = getMsParamByLabel( execMyRuleInp->inpParamArray, buf ) ) {
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
                inParamName[paramCount] = strdup( buf );
                paramCount++;
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
                snprintf( stageArea, sizeof( stageArea ), "%s", buf );
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

        status = mkMssoMpfRunDir( structFileInx, _fco, runDir );
        if ( status < 0 ) {
            return status;
        }


        /* prefix  file and directory names with  physical pathnames */
        for ( size_t i = 0; i < paramCount ; i++ ) {
            if ( msParam_t * mP = getMsParamByLabel( execMyRuleInp->inpParamArray, inParamName[i] ) ) {
                if ( mP->inOutStruct != NULL ) {

                    char * tmpPtr2 = ( char * ) mP->inOutStruct;

                    /* check for sefaty onlu  if this a fullpath value file. no prefixing needed */
                    if ( ( *tmpPtr2 == '\"' &&  *( tmpPtr2 + 1 ) == '/' ) || *tmpPtr2 == '/' ) {
                        status = checkSafety( tmpPtr2 );
                        if ( status < 0 )  {
                            return status;
                        }
                        continue;
                    }
                    len = strlen( tmpPtr2 );
                    char * tmpPtr = ( char * ) malloc( len + strlen( runDir ) + 10 );
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
                    char * tmpPtr = ( char * ) malloc( strlen( runDir ) + 10 );
                    sprintf( tmpPtr, "\"%s\"", runDir );
                    mP->inOutStruct = tmpPtr;
                }
            }
            /* removing truncation of inParamFile name */
            free( inParamName[i] );
        }
        return 0;
    }

    int extractMssoFile(
        int         structFileInx,
        irods::structured_object_ptr _fco,
        char*       runDir,
        char*       showFiles ) {
        int status, jj, i;
        specColl_t *specColl = MssoStructFileDesc[structFileInx].specColl;
        char mpfFileName[NAME_LEN];
        char mpfFilePath[MAX_NAME_LEN];
        execMyRuleInp_t execMyRuleInp;
        msParamArray_t msParamArray;
        rsComm_t *rsComm = MssoStructFileDesc[structFileInx].rsComm;
        msParamArray_t *outParamArray = NULL;
        msParam_t *mP;
        char *t;
        char mvstr[MAX_NAME_LEN];
        char stagefilename[MAX_NAME_LEN];
        dataObjInp_t dataObjInp;
        portalOprOut_t *portalOprOut = NULL;
        bytesBuf_t dataObjOutBBuf;
        int localTime;
        struct stat statbuf;
        int inputHasChanged = 0;
        char mvStr[MAX_NAME_LEN * 2];

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering extractMssoFile" );
#endif /* MSSO_DEBUG */

        if ( runDir == NULL ) {
            return SYS_MSSO_STRUCT_FILE_EXTRACT_ERR;
        }

        status = getMpfFileName( _fco, mpfFileName );
        if ( status < 0 ) {
            return status;
        }
        snprintf( mpfFilePath, MAX_NAME_LEN,  "%s/%s.mpf", specColl->cacheDir, mpfFileName );
        showFiles[0] = '\0';
        status = prepareForExecution( specColl->phyPath, mpfFilePath, runDir, showFiles,
                                      &execMyRuleInp, &msParamArray, structFileInx, _fco );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE, "Failure in extractMssoFile at prepareForExecution: %d\n",
                     status );
            return status;
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
                    rodsObjStat_t *rodsObjStatOut;
                    status  = rsObjStat( rsComm, &dataObjInp,  &rodsObjStatOut );
                    if ( status < 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "Failure in extractMssoFile at rsDataObjStat when stating files checking execution: %d\n",
                                 status );
                        freeRodsObjStat( rodsObjStatOut );
                        return status;
                    }
                    localTime = atoi( rodsObjStatOut->modifyTime );
                    freeRodsObjStat( rodsObjStatOut );
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
                        return UNIX_FILE_STAT_ERR;
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
                if ( system( mvStr ) ) {}
            }
            if ( strlen( showFiles ) != 0 ) {
                if ( strstr( showFiles, "stdout" ) != NULL ) {
                    //snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stdout", runDir );
                    std::string rdir( runDir );
                    rdir += "/stdout";
                    _fco->sub_file_path( rdir );
                }
                else if ( strstr( showFiles, "stderr" ) != NULL ) {
                    //snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stderr", runDir );
                    std::string rdir( runDir );
                    rdir += "/stderr";
                    _fco->sub_file_path( rdir );
                }
                else {
                    if ( ( mP = getMsParamByLabel( &msParamArray, showFiles ) ) != NULL ) {
                        //snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s", ( char* ) mP->inOutStruct );
                        _fco->sub_file_path( ( char* )mP->inOutStruct );
                    }
                }
            }
            else {
                snprintf( mvStr, sizeof mvStr, "%s/stdout", runDir );
                status = stat( runDir, &statbuf );
                if ( status == 0 )  {
                    //snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stdout", runDir );
                    std::string rdir( runDir );
                    rdir += "/stdout";
                    _fco->sub_file_path( rdir );
                }
                else {
                    snprintf( mvStr, sizeof mvStr, "%s/stderr", runDir );
                    status = stat( runDir, &statbuf );
                    if ( status == 0 )  {
                        //snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stderr", runDir );
                        std::string rdir( runDir );
                        rdir += "/stderr";
                        _fco->sub_file_path( rdir );
                    }
                }
            }
            return 0;
        }
        else { /* input has changed or new */
            /* do the moves if needed */
            if ( newRunDirName[0] != '\0' ) {
                snprintf( mvStr, sizeof mvStr, "mv -f %s/* %s", runDir, newRunDirName );
                if ( system( mvStr ) ) {}
            }
            else if ( oldRunDirTime > 0 ) {
                snprintf( mvStr, sizeof mvStr, "mkdir %s.backup", runDir );
                snprintf( mvStr, sizeof mvStr, "mv -f %s/* %s.backup", runDir, runDir );
                if ( system( mvStr ) ) {}
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
                if ( *t == '/' ) {
                    t++;    /*skip the slash if it exists */
                }
                snprintf( stagefilename, sizeof( stagefilename ), "%s", t );
            }
            else {
                *t = '\0';
                t++;    /* stage file name starts here */
                while ( *t == ' ' ) {
                    t++;    /* skip leading blanks */
                }
                snprintf( stagefilename, sizeof( stagefilename ), "%s", t );
            }
            if ( stageIn[stinCnt][0] == '/' ) { /* it is an irods collection ###### */
                memset( &dataObjInp, 0, sizeof( dataObjInp ) );
                memset( &dataObjOutBBuf, 0, sizeof( dataObjOutBBuf ) );
                snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s", stageIn[stinCnt] );
                dataObjInp.openFlags = O_RDONLY;
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, stagefilename );
                status  = rsDataObjGet( rsComm, &dataObjInp, &portalOprOut, &dataObjOutBBuf );
                if ( status < 0 ) {
                    if ( portalOprOut != NULL ) {
                        free( portalOprOut );
                    }
                    rodsLog( LOG_NOTICE,
                             "Failure in extractMssoFile at rsDataObjGet when copying files from iRODS into execution area: %d\n",
                             status );
                    return status;
                }
                if ( status == 0 || dataObjOutBBuf.len > 0 ) {
                    /* the buffer contains the file */
                    if ( FILE * fd = fopen( mvstr, "w" ) ) {
                        int bytes_written = fwrite( dataObjOutBBuf.buf, 1, dataObjOutBBuf.len, fd );
                        fclose( fd );
                        free( dataObjOutBBuf.buf );
                        if ( bytes_written != dataObjOutBBuf.len ) {
                            rodsLog( LOG_NOTICE,
                                     "extractMssoFile:  copy len error for file in stage area %s for writing:%d, status=%d\n", mvstr,
                                     dataObjOutBBuf.len, bytes_written );
                            return SYS_COPY_LEN_ERR;
                        }
                    }
                    else {
                        free( dataObjOutBBuf.buf );
                        rodsLog( LOG_NOTICE,
                                 "extractMssoFile:  could not open file in stage area %s for writing:%d\n", mvstr );
                        return FILE_OPEN_ERR;
                    }
                }
                else { /* file is too large!!! */
                    rodsLog( LOG_NOTICE,
                             "extractMssoFile:  copy file  too large to get into stage area %s for writing:%d\n", mvstr );
                    return USER_FILE_TOO_LARGE;
                }
            }
            else { /* local directory or file */
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", specColl->cacheDir, stageIn[stinCnt] );
                checkPhySafety( mvstr );
                snprintf( mvstr, MAX_NAME_LEN, "cp -rf  %s/%s %s/%s", specColl->cacheDir, stageIn[stinCnt], stageArea, stagefilename );
                if ( system( mvstr ) ) {}
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
            return status;
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
                if ( system( mvstr ) ) {}
            }
            else {
                *t = '\0';
                t++;
                while ( *t == ' ' ) {
                    t++;    /* skip blanks */
                }
                snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, stageOut[stoutCnt] );
                checkPhySafety( mvstr );
                if ( *t == '/' ) { /* target is an irods object */
                    /* #### */
                    /* now remove the file */
                    snprintf( mvstr, MAX_NAME_LEN, "rm -rf %s/%s", stageArea, stageOut[stoutCnt] );
                    if ( system( mvstr ) ) {}
                }
                else {
                    snprintf( mvstr, MAX_NAME_LEN, "mv -f %s/%s %s/%s", stageArea, stageOut[stoutCnt], runDir, t );
                    /* now move the file */
                    if ( system( mvstr ) ) {}
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
                if ( system( mvstr ) ) {}
            }
            else {
                *t = '\0';
                t++;
                while ( *t == ' ' ) {
                    t++;    /* skip blanks */
                }
                if ( *t == '/' ) { /* target is an irods object */
                    /* #### */
                }
                else {
                    snprintf( mvstr, MAX_NAME_LEN, "%s/%s", stageArea, copyToIrods[cpoutCnt] );
                    checkPhySafety( mvstr );
                    snprintf( mvstr, MAX_NAME_LEN, "cp -rf %s/%s %s/%s", stageArea, copyToIrods[cpoutCnt], runDir, t );
                    /* now copy the file */
                    if ( system( mvstr ) ) {}
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
                //snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stdout", runDir );
                std::string rdir( runDir );
                rdir += "/stdout";
                _fco->sub_file_path( rdir );
            }
            else if ( strstr( showFiles, "stderr" ) != NULL ) {
                //snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s/stderr", runDir );
                std::string rdir( runDir );
                rdir += "/stderr";
                _fco->sub_file_path( rdir );
            }
            else {
                if ( ( mP = getMsParamByLabel( outParamArray, showFiles ) ) != NULL ) {
                    // snprintf( subFile->subFilePath, MAX_NAME_LEN, "%s", ( char* ) mP->inOutStruct );
                    _fco->sub_file_path( ( char* )mP->inOutStruct );
                }
            }
        }

        /* what is the showfile */
        cleanOutStageArea( stageArea );
        i = popOutStackInfo();
        if ( i < 0 ) {
            rodsLog( LOG_ERROR, "PopOutStackInfo Error:%i\n", i );
            return i;
        }

        return 0;
    }




    int stageMssoStructFile(
        int                          _struct_inx,
        irods::structured_object_ptr _fco ) {

        int status;
        int fileType = 0; /* 1=mss file, 2=mpf file 3=datafile */
        char *t, *s;
        char runDir[MAX_NAME_LEN];
        char showFiles[MAX_NAME_LEN];

        rsComm_t*   rsComm   = MssoStructFileDesc[ _struct_inx ].rsComm;
        specColl_t* specColl = MssoStructFileDesc[ _struct_inx ].specColl;
        if ( specColl == NULL ) {
            rodsLog( LOG_NOTICE,
                     "stageMssoStructFile: NULL specColl input" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        if ( isFakeFile( specColl->collection, specColl->objPath ) == 1 ) {
            /* is fake file. make  t point to NULL */
            t = runDir;
            runDir[0] = '\0';
        }
        else {
            if ( ( t = strstr( ( char* )_fco->sub_file_path().c_str(),
                               MssoStructFileDesc[ _struct_inx ].specColl->collection ) ) == NULL ) {
                return SYS_STRUCT_FILE_PATH_ERR;
            }

            t = t + strlen( MssoStructFileDesc[ _struct_inx ].specColl->collection );
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
                    return 0;
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
            return SYS_STRUCT_FILE_PATH_ERR;
        }

        if ( fileType == 1 ) {
            if ( strlen( specColl->cacheDir ) == 0 ) {
                /* no cache. stage one. make the CacheDir first */
                status = mkMssoCacheDir( _struct_inx );
                if ( status < 0 ) {
                    return status;
                }
                /**********
                  status = extractMssoFile ( _struct_inx , subFile);
                  if (status < 0) {
                  rodsLog (LOG_NOTICE,
                  "stageMssoStructFile:extract error for %s in cacheDir %s,errno=%d",
                  specColl->objPath, specColl->cacheDir, errno);
                  return SYS_MSSO_STRUCT_FILE_EXTRACT_ERR - errno;
                  }
                 ******************/
                /* register the CacheDir */
                status = modCollInfo2( rsComm, specColl, 0 ); /*#######*/
                if ( status < 0 ) {
                    return status;
                }
            }
        }
        else if ( fileType == 2 ) {
            if ( strlen( specColl->cacheDir ) == 0 ) {
                /* no cache. stage one. make the CacheDir first */
                status = mkMssoCacheDir( _struct_inx );
                if ( status < 0 ) {
                    return status;
                }
                /* register the CacheDir */
                status = modCollInfo2( rsComm, specColl, 0 );
                if ( status < 0 ) {
                    return status;
                }
            }

            /* create a run file */
            status = mkMssoMpfRunFile( _struct_inx , _fco );
            return status;
        }
        else if ( fileType == 3 ) { /* run */
            /* perform the run */
            status = extractMssoFile( _struct_inx , _fco, runDir, showFiles );
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "stageMssoStructFile:extract error for %s in cacheDir %s,errno=%d",
                         specColl->objPath, runDir, errno );
                return SYS_MSSO_STRUCT_FILE_EXTRACT_ERR - errno;
            }
            return status;
        }
        else { /* fileType == 4   looking at run results */

        }
        return 0;
    }

    int rsMssoStructFileOpen(
        rsComm_t*                    _comm,
        irods::structured_object_ptr _fco,
        int                          _stage ) {
        int structFileInx;
        int status;

        specCollCache_t *specCollCache;

        specColl_t* spec_coll = _fco->spec_coll();
        if ( spec_coll == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsMssoStructFileOpen: NULL specColl input" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        /* look for opened MssoStructFileDesc */

        structFileInx = matchMssoStructFileDesc( spec_coll );

        if ( structFileInx > 0 && ( _stage != 1 ) ) {
            return structFileInx;
        }

        if ( structFileInx < 0 ) {
            if ( ( structFileInx = alloc_struct_file_desc() ) < 0 ) {
                return structFileInx;
            }

            /* Have to do this because specColl could come from a remote host */
            if ( ( status = getSpecCollCache( _comm, spec_coll->collection, 0,
                                              &specCollCache ) ) >= 0 ) {
                MssoStructFileDesc[structFileInx].specColl = &specCollCache->specColl;
                /* getSpecCollCache does not give phyPath nor resource */
                if ( strlen( spec_coll->phyPath ) > 0 ) {
                    rstrcpy( specCollCache->specColl.phyPath, spec_coll->phyPath,
                             MAX_NAME_LEN );
                }
                if ( strlen( specCollCache->specColl.resource ) == 0 ) {
                    rstrcpy( specCollCache->specColl.resource, spec_coll->resource,
                             NAME_LEN );
                }
            }
            else {
                MssoStructFileDesc[structFileInx].specColl = spec_coll;
            }

            MssoStructFileDesc[structFileInx].rsComm = _comm;

        }
        /* XXXXX need to deal with remote open here */
        /* do not stage unless it is a get or am open */
        if ( isFakeFile( spec_coll->collection, spec_coll->objPath ) == 0 ) {
            /* not a fake file */
            if ( _stage != 1 ) {
                return structFileInx;
            }
        }
        status = stageMssoStructFile( structFileInx, _fco );

        if ( status < 0 ) {
            free_struct_file_desc( structFileInx );
            return status;
        }

        return structFileInx;
    }




    int
    syncCacheDirToMssofile( int structFileInx, int oprType ) {
        int status;
        fileStatInp_t fileStatInp;
        rodsStat_t *fileStatOut = NULL;
        specColl_t *specColl = MssoStructFileDesc[structFileInx].specColl;
        rsComm_t *rsComm = MssoStructFileDesc[structFileInx].rsComm;

#ifdef MSSO_DEBUG
        rodsLog( LOG_NOTICE, "Entering syncCacheDirToMssofile" );
#endif /* MSSO_DEBUG */


        /* bundle called here */

        /* register size change */
        memset( &fileStatInp, 0, sizeof( fileStatInp ) );
        rstrcpy( fileStatInp.fileName, specColl->phyPath, MAX_NAME_LEN );
        rstrcpy( fileStatInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );

        snprintf( fileStatInp.rescHier, MAX_NAME_LEN, "%s", specColl->rescHier );
        status = rsFileStat( rsComm, &fileStatInp, &fileStatOut );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "syncCacheDirToMssofile: rsFileStat error for %s, status = %d",
                     specColl->phyPath, status );
            return status;
        }

        if ( ( oprType & NO_REG_COLL_INFO ) == 0 ) {
            /* for bundle opr, done at datObjClose */
            status = regNewObjSize( rsComm, specColl->objPath, specColl->replNum,
                                    fileStatOut->st_size );
        }

        free( fileStatOut );

        return status;
    }


    int
    initMssoSubFileDesc() {
        memset( MssoSubFileDesc, 0,
                sizeof( mssoSubFileDesc_t ) * NUM_MSSO_SUB_FILE_DESC );
        return 0;
    }

    int
    allocMssoSubFileDesc() {
        int i;

        for ( i = 1; i < NUM_MSSO_SUB_FILE_DESC; i++ ) {
            if ( MssoSubFileDesc[i].inuseFlag == FD_FREE ) {
                MssoSubFileDesc[i].inuseFlag = FD_INUSE;
                return i;
            };
        }

        rodsLog( LOG_NOTICE,
                 "allocMssoSubFileDesc: out of MssoSubFileDesc" );

        return SYS_OUT_OF_FILE_DESC;
    }

    int
    freeMssoSubFileDesc( int mssoSubFileInx ) {
        if ( mssoSubFileInx < 0 || mssoSubFileInx >= NUM_MSSO_SUB_FILE_DESC ) {
            rodsLog( LOG_NOTICE,
                     "freeMssoSubFileDesc: mssoSubFileInx %d out of range", mssoSubFileInx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        memset( &MssoSubFileDesc[mssoSubFileInx], 0, sizeof( mssoSubFileDesc_t ) );

        return 0;
    }


    /* getMssoSubStructFilePhyPath - get the phy path in the cache dir */
    int getMssoSubStructFilePhyPath(
        char*       phyPath,
        specColl_t* specColl,
        const char* subFilePath ) {
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
                return SYS_STRUCT_FILE_PATH_ERR;
            }

            snprintf( phyPath, MAX_NAME_LEN, "%s%s", specColl->cacheDir,
                      subFilePath + len );
        }

        if ( !strlen( phyPath ) ) {
            rodsLog( LOG_ERROR, "getMssoSubStructFilePhyPath :: phyPath is empty" );
            return SYS_STRUCT_FILE_PATH_ERR;
        }

        return 0;
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
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );
        int structFileInx = 0;
        int subInx        = 0;
        int status        = 0;

        specColl_t* specColl = fco->spec_coll();
        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( specColl->rescHier, location );
        if ( !ret.ok() ) {
            return PASSMSG( "msso_file_create_plugin - failed in get_loc_for_hier_string", ret );
        }

        snprintf( MssoStructFileDesc[structFileInx].location, NAME_LEN, "%s", location.c_str() );

        structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 1 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "mssoSubStructFileCreate: rsMssoStructFileOpen error for ["
                << specColl->objPath
                << " stat [" <<  structFileInx << "]";
            return ERROR( structFileInx, msg.str() );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        subInx = allocMssoSubFileDesc();

        if ( subInx < 0 ) {
            return ERROR( subInx, "alloc msso failed" );
        }

        MssoSubFileDesc[subInx].structFileInx = structFileInx;

        fileCreateInp_t fileCreateInp;
        memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );
        status = getMssoSubStructFilePhyPath( fileCreateInp.fileName, specColl,
                                              fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed" );
        }

        rstrcpy( fileCreateInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location, NAME_LEN );
        fileCreateInp.mode       = fco->mode();
        fileCreateInp.flags      = fco->flags();
        fileCreateInp.otherFlags = NO_CHK_PERM_FLAG;

        fileCreateOut_t* create_out = NULL;
        snprintf( fileCreateInp.resc_hier_, MAX_NAME_LEN, "%s", specColl->rescHier );
        status = rsFileCreate( _ctx.comm(), &fileCreateInp, &create_out );
        free( create_out );

        if ( status < 0 ) {
            std::stringstream msg;
            msg << "specCollCreate: rsFileCreate for ["
                << fileCreateInp.fileName << "]";
            return ERROR( status, msg.str() );
        }
        MssoSubFileDesc[subInx].fd = status;
        MssoStructFileDesc[structFileInx].openCnt++;
        fco->file_descriptor( subInx );
        return CODE( subInx );

    } // msso_file_create_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX open
    irods::error msso_file_open_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        int structFileInx = 0;
        int subInx        = 0;
        int status        = 0;

        specColl_t* specColl = fco->spec_coll();

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( specColl->rescHier, location );
        if ( !ret.ok() ) {
            return PASS( ret );
        }
        structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 1 );
        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "mssoSubStructFileOpen: rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }

        snprintf( MssoStructFileDesc[structFileInx].location, NAME_LEN, "%s", location.c_str() );

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        subInx = allocMssoSubFileDesc();

        if ( subInx < 0 ) {
            return ERROR( subInx, "allocMssoSubFileDesc" );
        }

        MssoSubFileDesc[subInx].structFileInx = structFileInx;

        fileOpenInp_t fileOpenInp;
        memset( &fileOpenInp, 0, sizeof( fileOpenInp ) );
        status = getMssoSubStructFilePhyPath(
                     fileOpenInp.fileName,
                     specColl,
                     fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath" );
        }

        rstrcpy( fileOpenInp.addr.hostAddr, location.c_str(), NAME_LEN );
        fileOpenInp.mode  = fco->mode();
        fileOpenInp.flags = fco->flags();
        snprintf( fileOpenInp.resc_hier_, MAX_NAME_LEN, "%s", fco->resc_hier().c_str() );
        status = rsFileOpen( _ctx.comm(), &fileOpenInp );

        if ( status < 0 ) {
            std::stringstream msg;
            msg << "specCollOpen: rsFileOpen for ["
                << fileOpenInp.fileName << "]";
            return ERROR( status, msg.str() );
        }
        else {
            MssoSubFileDesc[subInx].fd = status;
            MssoStructFileDesc[structFileInx].openCnt++;
            fco->file_descriptor( subInx );
            return CODE( subInx );
        }

    } // msso_file_open_plugin

    // =-=-=-=-=-=-=-
    // inteface for POSIX read
    irods::error msso_file_read_plugin(
        irods::resource_plugin_context& _ctx,
        void*                           buf,
        int                             len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );
        int subInx = fco->file_descriptor();


        if ( subInx < 1 ||
                subInx >= NUM_MSSO_SUB_FILE_DESC ||
                MssoSubFileDesc[subInx].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "mssoSubStructFileRead: subInx "
                << subInx << " out of range";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        fileReadInp_t fileReadInp;
        memset( &fileReadInp, 0, sizeof( fileReadInp ) );

        bytesBuf_t fileReadOutBBuf;
        memset( &fileReadOutBBuf, 0, sizeof( fileReadOutBBuf ) );
        fileReadInp.fileInx = MssoSubFileDesc[subInx].fd;
        fileReadInp.len = len;
        fileReadOutBBuf.buf = buf;
        int status = rsFileRead( _ctx.comm(), &fileReadInp, &fileReadOutBBuf );

        if ( status < 0 ) {
            return ERROR( status, "rsFileRead failed" );
        }
        else {
            return CODE( status );
        }

    } // msso_file_read_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX write
    irods::error msso_file_write_plugin(
        irods::resource_plugin_context& _ctx,
        void*                           buf,
        int                             len ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );
        int subInx = fco->file_descriptor();

        if ( subInx < 1 ||
                subInx >= NUM_MSSO_SUB_FILE_DESC ||
                MssoSubFileDesc[subInx].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "mssoSubStructFileWrite: subInx "
                << subInx << " out of range";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        fileWriteInp_t fileWriteInp;
        memset( &fileWriteInp, 0, sizeof( fileWriteInp ) );

        bytesBuf_t fileWriteOutBBuf;
        memset( &fileWriteOutBBuf, 0, sizeof( fileWriteOutBBuf ) );
        fileWriteInp.fileInx = MssoSubFileDesc[subInx].fd;
        fileWriteInp.len = fileWriteOutBBuf.len = len;
        fileWriteOutBBuf.buf = buf;
        int status = rsFileWrite( _ctx.comm(), &fileWriteInp, &fileWriteOutBBuf );

        if ( status > 0 ) {
            specColl_t *specColl;
            int status1;
            int structFileInx = MssoSubFileDesc[subInx].structFileInx;
            /* cache has been written */
            specColl = MssoStructFileDesc[structFileInx].specColl;
            if ( specColl->cacheDirty == 0 ) {
                specColl->cacheDirty = 1;
                status1 = modCollInfo2( _ctx.comm(), specColl, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "modCollInfo2 failed" );
                }
            }

            return CODE( status );
        }
        else {
            return ERROR( status, "rsFileWrite failed" );
        }

    } // msso_file_write_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX close
    irods::error msso_file_close_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );
        int subInx = fco->file_descriptor();


        if ( subInx < 1 ||
                subInx >= NUM_MSSO_SUB_FILE_DESC ||
                MssoSubFileDesc[subInx].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "mssoSubStructFileClose: subInx "
                << subInx << " out of range";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        fileCloseInp_t fileCloseInp;
        fileCloseInp.fileInx = MssoSubFileDesc[subInx].fd;
        int status = rsFileClose( _ctx.comm(), &fileCloseInp );

        int structFileInx = MssoSubFileDesc[subInx].structFileInx;
        MssoStructFileDesc[structFileInx].openCnt++;
        freeMssoSubFileDesc( subInx );

        if ( status < 0 ) {
            return ERROR( status, "rsFileClose failed" );
        }
        else {
            return CODE( status );
        }

    } // msso_file_close_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX unlink
    irods::error  msso_file_unlink_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl = fco->spec_coll();
        int structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        fileUnlinkInp_t fileUnlinkInp;
        memset( &fileUnlinkInp, 0, sizeof( fileUnlinkInp ) );
        int status = getMssoSubStructFilePhyPath(
                         fileUnlinkInp.fileName,
                         specColl,
                         fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed" );
        }

        rstrcpy( fileUnlinkInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );

        snprintf( fileUnlinkInp.rescHier, sizeof( fileUnlinkInp.rescHier ), "%s", specColl->rescHier );
        status = rsFileUnlink( _ctx.comm(), &fileUnlinkInp );
        if ( status >= 0 ) {
            specColl_t *specColl;
            int status1;
            /* cache has been written */
            specColl = MssoStructFileDesc[structFileInx].specColl;
            if ( specColl->cacheDirty == 0 ) {
                specColl->cacheDirty = 1;
                status1 = modCollInfo2( _ctx.comm(), specColl, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "modCollInfo2 failed" );
                }
            }

            return CODE( status );
        }
        else {
            return ERROR( status, "rsFileUnlink failed" );
        }

    } // msso_file_unlink_plugin

    irods::error msso_file_stat_plugin(
        irods::resource_plugin_context& _ctx,
        struct stat*                    _statbuf ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl = fco->spec_coll();
        int structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        fileStatInp_t fileStatInp;
        memset( &fileStatInp, 0, sizeof( fileStatInp ) );
        int status = getMssoSubStructFilePhyPath(
                         fileStatInp.fileName,
                         specColl,
                         fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed" );
        }

        rstrcpy( fileStatInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );
        snprintf( fileStatInp.rescHier, sizeof( fileStatInp.rescHier ), "%s", specColl->rescHier );

        rodsStat_t* rods_stat = 0;
        status = rsFileStat( _ctx.comm(), &fileStatInp, &rods_stat );

        if ( status < 0 ) {
            return ERROR( status, "rsFileStat failed" );
        }
        //if( strstr( &(subFile->subFilePath[strlen(subFile->subFilePath) -5]),
        //    ".run") != NULL && (*subStructFileStatOut)->st_size == 0 )
        const std::string& path = fco->sub_file_path();
        if ( std::string::npos != path.find( ".run", path.size() - 5 ) &&
                rods_stat->st_size == 0 ) {
            rods_stat->st_size = MAX_SZ_FOR_SINGLE_BUF - 20;
        }

        rodsStatToStat( _statbuf, rods_stat );
        free( rods_stat );

        return CODE( status );

    } // msso_file_stat_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek operation
    irods::error msso_file_lseek_plugin(
        irods::resource_plugin_context& _ctx,
        rodsLong_t                      offset,
        int                             whence ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        int subInx = fco->file_descriptor();

        if ( subInx < 1 ||
                subInx >= NUM_MSSO_SUB_FILE_DESC ||
                MssoSubFileDesc[subInx].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "sub index " << subInx << " is out of range";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        fileLseekInp_t fileLseekInp;
        memset( &fileLseekInp, 0, sizeof( fileLseekInp ) );
        fileLseekInp.fileInx = MssoSubFileDesc[subInx].fd;
        fileLseekInp.offset = offset;
        fileLseekInp.whence = whence;

        fileLseekOut_t *fileLseekOut = NULL;
        int status = rsFileLseek( _ctx.comm(), &fileLseekInp, &fileLseekOut );

        if ( status < 0 ) {
            return ERROR( status, "rsFileLseek Failed." );
        }
        else {
            rodsLong_t offset = fileLseekOut->offset;
            free( fileLseekOut );
            return CODE( offset );
        }

    } // msso_file_lseek_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX rename operation
    irods::error msso_file_rename_plugin(
        irods::resource_plugin_context& _ctx,
        char*                           newFileName ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl      = fco->spec_coll();
        int         structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "rsMssoStructFileOpen error for ["
                << specColl->objPath;
            return ERROR( structFileInx, msg.str() );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        fileRenameInp_t fileRenameInp;
        memset( &fileRenameInp, 0, sizeof( fileRenameInp ) );
        int status = getMssoSubStructFilePhyPath(
                         fileRenameInp.oldFileName,
                         specColl,
                         fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed" );
        }
        status = getMssoSubStructFilePhyPath(
                     fileRenameInp.newFileName,
                     specColl,
                     newFileName );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed" );
        }
        rstrcpy( fileRenameInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );
        fileRenameOut_t* rename_out = 0;
        snprintf( fileRenameInp.rescHier, MAX_NAME_LEN, "%s", specColl->rescHier );
        status = rsFileRename( _ctx.comm(), &fileRenameInp, &rename_out );
        // NOTE :: rename could possible change the name under the covers,
        //         need to consider how to handle this here.

        free( rename_out );
        if ( status >= 0 ) {
            int status1;
            /* use the specColl in  MssoStructFileDesc */
            specColl = MssoStructFileDesc[structFileInx].specColl;
            /* cache has been written */
            if ( specColl->cacheDirty == 0 ) {
                specColl->cacheDirty = 1;
                status1 = modCollInfo2( _ctx.comm(), specColl, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "modCollInfo2 failed" );
                }
            }

            return CODE( status );
        }
        else {
            return ERROR( status, "rsFileRename failed" );
        }

    } // msso_file_rename_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir operatoin
    irods::error msso_file_mkdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl      = fco->spec_coll();
        int         structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        fileMkdirInp_t fileMkdirInp;
        int status = getMssoSubStructFilePhyPath(
                         fileMkdirInp.dirName,
                         specColl,
                         fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed" );
        }

        rstrcpy( fileMkdirInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );
        fileMkdirInp.mode = fco->mode();
        snprintf( fileMkdirInp.rescHier, sizeof( fileMkdirInp.rescHier ), "%s", specColl->rescHier );
        status = rsFileMkdir( _ctx.comm(), &fileMkdirInp );

        if ( status >= 0 ) {
            int status1;
            /* use the specColl in  MssoStructFileDesc */
            specColl = MssoStructFileDesc[structFileInx].specColl;
            /* cache has been written */
            if ( specColl->cacheDirty == 0 ) {
                specColl->cacheDirty = 1;
                status1 = modCollInfo2( _ctx.comm(), specColl, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "modCollInfo2 failed" );
                }
            }

            return CODE( status );
        }
        else {
            return ERROR( status, "rsFileMkdir failed" );
        }

    } // msso_file_mkdir_plugin

    irods::error msso_file_rmdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl      = fco->spec_coll();
        int         structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        fileRmdirInp_t fileRmdirInp;
        int status = getMssoSubStructFilePhyPath(
                         fileRmdirInp.dirName,
                         specColl,
                         fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed." );
        }

        rstrcpy( fileRmdirInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );
        snprintf( fileRmdirInp.rescHier, sizeof( fileRmdirInp.rescHier ), "%s", specColl->rescHier );
        status = rsFileRmdir( _ctx.comm(), &fileRmdirInp );
        if ( status >= 0 ) {
            int status1;
            /* use the specColl in  MssoStructFileDesc */
            specColl = MssoStructFileDesc[structFileInx].specColl;
            /* cache has been written */
            if ( specColl->cacheDirty == 0 ) {
                specColl->cacheDirty = 1;
                status1 = modCollInfo2( _ctx.comm(), specColl, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "modCollInfo2 failed" );
                }
            }
            return CODE( status );
        }
        return ERROR( status, "rsFileRmdir failed" );

    } // msso_file_rmdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir operation
    irods::error msso_file_opendir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );


        specColl_t* specColl      = fco->spec_coll();
        int         structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << "rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }


        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        int subInx = allocMssoSubFileDesc();

        if ( subInx < 0 ) {
            return ERROR( subInx, "allocMssoSubFileDesc failed" );
        }

        MssoSubFileDesc[subInx].structFileInx = structFileInx;

        fileOpendirInp_t fileOpendirInp;
        memset( &fileOpendirInp, 0, sizeof( fileOpendirInp ) );
        int status = getMssoSubStructFilePhyPath(
                         fileOpendirInp.dirName,
                         specColl,
                         fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed." );
        }

        rstrcpy( fileOpendirInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );
        snprintf( fileOpendirInp.resc_hier_, MAX_NAME_LEN, "%s", specColl->rescHier );
        status = rsFileOpendir( _ctx.comm(), &fileOpendirInp );
        if ( status < 0 ) {
            std::stringstream msg;
            msg << "rsFileOpendir for ["
                << fileOpendirInp.dirName;
            return ERROR( status, msg.str() );
        }
        else {
            MssoSubFileDesc[subInx].fd = status;
            MssoStructFileDesc[structFileInx].openCnt++;
            return CODE( subInx );
        }

    } // msso_file_opendir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir operation
    irods::error msso_file_readdir_plugin(
        irods::resource_plugin_context& _ctx,
        rodsDirent_t**                  rodsDirent ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        int subInx = fco->file_descriptor();

        if ( subInx < 1 ||
                subInx >= NUM_MSSO_SUB_FILE_DESC ||
                MssoSubFileDesc[subInx].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "subInx " << subInx << " out of range";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        fileReaddirInp_t fileReaddirInp;
        fileReaddirInp.fileInx = MssoSubFileDesc[subInx].fd;
        int status = rsFileReaddir( _ctx.comm(), &fileReaddirInp, rodsDirent );

        if ( status < 0 ) {
            return ERROR( status, "rsFileRmdir failed" );
        }
        else {
            return CODE( status );
        }

    } // msso_file_readdir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir operation
    irods::error msso_file_closedir_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        int subInx = fco->file_descriptor();

        if ( subInx < 1 ||
                subInx >= NUM_MSSO_SUB_FILE_DESC ||
                MssoSubFileDesc[subInx].inuseFlag == 0 ) {
            std::stringstream msg;
            msg << "subInx " << subInx << " out of range";
            return ERROR( SYS_STRUCT_FILE_DESC_ERR, msg.str() );
        }

        fileClosedirInp_t fileClosedirInp;
        fileClosedirInp.fileInx = MssoSubFileDesc[subInx].fd;
        int status = rsFileClosedir( _ctx.comm(), &fileClosedirInp );

        int structFileInx = MssoSubFileDesc[subInx].structFileInx;
        MssoStructFileDesc[structFileInx].openCnt++;
        freeMssoSubFileDesc( subInx );

        if ( status < 0 ) {
            return ERROR( status, "rsFileClosedir failed" );
        }
        else {
            return CODE( status );
        }

    } // msso_file_closedir_plugin

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate operation
    irods::error msso_file_truncate_plugin(
        irods::resource_plugin_context& _ctx )  {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl      = fco->spec_coll();
        int         structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << " rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }

        /* use the cached specColl. specColl may have changed */
        specColl = MssoStructFileDesc[structFileInx].specColl;

        fileOpenInp_t fileTruncateInp;
        int status = getMssoSubStructFilePhyPath(
                         fileTruncateInp.fileName,
                         specColl,
                         fco->sub_file_path().c_str() );
        if ( status < 0 ) {
            return ERROR( status, "getMssoSubStructFilePhyPath failed" );
        }

        rstrcpy( fileTruncateInp.addr.hostAddr,
                 MssoStructFileDesc[structFileInx].location,
                 NAME_LEN );
        fileTruncateInp.dataSize = fco->offset();
        snprintf( fileTruncateInp.resc_hier_, sizeof( fileTruncateInp.resc_hier_ ), "%s", specColl->rescHier );
        status = rsFileTruncate( _ctx.comm(), &fileTruncateInp );

        if ( status >= 0 ) {
            int status1;
            /* use the specColl in  MssoStructFileDesc */
            specColl = MssoStructFileDesc[structFileInx].specColl;
            /* cache has been written */
            if ( specColl->cacheDirty == 0 ) {
                specColl->cacheDirty = 1;
                status1 = modCollInfo2( _ctx.comm(), specColl, 0 );
                if ( status1 < 0 ) {
                    return ERROR( status1, "modCollInfo2 failed" );
                }
            }

            return CODE( status );
        }
        else {
            return ERROR( status, "rsFileTruncate failed" );
        }

    } // msso_file_truncate_plugin

    // =-=-=-=-=-=-=-
    // interface for sync'ing the structured object
    irods::error msso_file_sync_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl      = fco->spec_coll();
        int         structFileInx = rsMssoStructFileOpen( _ctx.comm(), fco, 0 );

        if ( structFileInx < 0 ) {
            std::stringstream msg;
            msg << " rsMssoStructFileOpen error for ["
                << specColl->objPath << "]";
            return ERROR( structFileInx, msg.str() );
        }

        if ( MssoStructFileDesc[structFileInx].openCnt > 0 ) {
            return ERROR( SYS_STRUCT_FILE_BUSY_ERR, "open count > 0 " );
        }

        /* use the specColl in  MssoStructFileDesc. More up to date */
        int opr_type = fco->opr_type();
        specColl = MssoStructFileDesc[structFileInx].specColl;
        if ( ( opr_type & DELETE_STRUCT_FILE ) != 0 ) {
            /* remove cache and the struct file */
            free_struct_file_desc( structFileInx );
            return SUCCESS();
        }

        char* dataType = getValByKey( &fco->cond_input(), DATA_TYPE_KW );
        if ( NULL != dataType ) {
            rstrcpy( MssoStructFileDesc[structFileInx].dataType, dataType, NAME_LEN );
        }

        int status   = 0;
        if ( strlen( specColl->cacheDir ) > 0 ) {
            if ( specColl->cacheDirty > 0 ) {
                /* write the msso file and register no dirty */
                status = syncCacheDirToMssofile(
                             structFileInx,
                             opr_type );
                if ( status < 0 ) {
                    std::stringstream msg;
                    msg << "syncCacheDirToMssofile failed for [" << specColl->cacheDir << "]";
                    free_struct_file_desc( structFileInx );
                    return ERROR( status, msg.str() );
                }

                specColl->cacheDirty = 0;
                if ( ( opr_type & NO_REG_COLL_INFO ) == 0 ) {
                    status = modCollInfo2( _ctx.comm(), specColl, 0 );
                    if ( status < 0 ) {
                        free_struct_file_desc( structFileInx );
                        return ERROR( status, "failed in modCollInfo2" );
                    }
                }
            }

            if ( ( opr_type & PURGE_STRUCT_FILE_CACHE ) != 0 ) {
                /* unregister cache before remove */
                status = modCollInfo2( _ctx.comm(), specColl, 1 );
                if ( status < 0 ) {
                    free_struct_file_desc( structFileInx );
                    return ERROR( status, "failed in modCollInfo2" );
                }
                /* remove cache */
                fileRmdirInp_t fileRmdirInp;
                memset( &fileRmdirInp, 0, sizeof( fileRmdirInp ) );
                rstrcpy( fileRmdirInp.dirName,
                         specColl->cacheDir,
                         MAX_NAME_LEN );
                rstrcpy( fileRmdirInp.addr.hostAddr,
                         MssoStructFileDesc[structFileInx].location,
                         NAME_LEN );
                fileRmdirInp.flags = RMDIR_RECUR;
                snprintf( fileRmdirInp.rescHier, sizeof( fileRmdirInp.rescHier ), "%s", specColl->rescHier );
                status = rsFileRmdir( _ctx.comm(), &fileRmdirInp );
                if ( status < 0 ) {
                    std::stringstream msg;
                    msg << "rsFileRmdir failed for [" << specColl->cacheDir << "]";
                    free_struct_file_desc( structFileInx );
                    return ERROR( status, msg.str() );
                }
            }
        }

        free_struct_file_desc( structFileInx );
        return CODE( status );
    } // msso_file_sync_plugin

    // =-=-=-=-=-=-=-
    // intefrace to extract a structured object
    irods::error msso_file_extract_plugin(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // check incoming parameters
        irods::error chk_err = msso_check_params( _ctx );
        if ( !chk_err.ok() ) {
            return PASSMSG( "msso_file_create_plugin", chk_err );
        }

        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::structured_object_ptr fco = boost::dynamic_pointer_cast <
                                           irods::structured_object > ( _ctx.fco() );

        specColl_t* specColl = fco->spec_coll();

        int structFileInx = alloc_struct_file_desc();
        if ( structFileInx < 0 ) {
            return ERROR( structFileInx, "failed in alloc" );
        }

        MssoStructFileDesc[structFileInx].specColl = specColl;
        MssoStructFileDesc[structFileInx].rsComm   = _ctx.comm();

        char* dataType = 0;
        if ( ( dataType = getValByKey(
                              &fco->cond_input(),
                              DATA_TYPE_KW ) ) != NULL ) {
            rstrcpy( MssoStructFileDesc[structFileInx].dataType, dataType, NAME_LEN );
        }

        int status = extractMssoFile( structFileInx, fco, NULL, NULL );
        free_struct_file_desc( structFileInx );

        if ( status < 0 ) {
            std::stringstream msg;
            msg << "error for [" << specColl->objPath << "] in cacheDir ["
                << specColl->cacheDir << " with errno " << errno;
            /* XXXX may need to remove the cacheDir too */
            status = SYS_MSSO_STRUCT_FILE_EXTRACT_ERR - errno;
            return ERROR( status, msg.str() );
        }

        return CODE( status );

    } // msso_file_extract_plugin

    // =-=-=-=-=-=-=-
    // interface for getting freespace of the fs
    irods::error msso_file_getfsfreespace_plugin(
        irods::resource_plugin_context& ) {
        return ERROR( SYS_NOT_SUPPORTED, "msso_file_getfsfreespace_plugin is not implemented" );

    } // msso_file_getfsfreespace_plugin

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file registration
    irods::error msso_file_registered_plugin(
        irods::resource_plugin_context& ) {
        // NOOP
        return SUCCESS();
    }

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file unregistration
    irods::error msso_file_unregistered_plugin(
        irods::resource_plugin_context& ) {
        // NOOP
        return SUCCESS();
    }

    /// =-=-=-=-=-=-=-
    /// @brief interface to notify of a file modification
    irods::error msso_file_modified_plugin(
        irods::resource_plugin_context& ) {
        // NOOP
        return SUCCESS();
    }

    /// =-=-=-=-=-=-=-
    /// @brief  - code which would rebalance the subtree
    irods::error msso_file_rebalance_plugin(
        irods::resource_plugin_context& ) {
        return SUCCESS();

    } // msso_file_rebalance

    /// =-=-=-=-=-=-=-
    /// @brief - code which would notify the subtree of a change
    irods::error msso_file_notify_plugin(
        irods::resource_plugin_context& ,
        const std::string* ) {
        return SUCCESS();

    } // msso_file_notify_plugin

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle msso file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class mssofilesystem_resource : public irods::resource {
        public:
            mssofilesystem_resource(
                const std::string& _inst_name,
                const std::string& _context ) :
                irods::resource( _inst_name, _context ) {
            } // ctor

    }; // class mssofilesystem_resource

    // =-=-=-=-=-=-=-
    // start operation used to allocate the FileDesc tables
    void mssofilesystem_resource_start() {
        memset( MssoStructFileDesc, 0, sizeof( structFileDesc_t ) * NUM_STRUCT_FILE_DESC );
        memset( MssoSubFileDesc, 0, sizeof( mssoSubFileDesc_t ) * NUM_MSSO_SUB_FILE_DESC );
    }

    // =-=-=-=-=-=-=-
    // stop operation used to free the FileDesc tables
    void mssofilesystem_resource_stop() {
        memset( MssoStructFileDesc, 0, sizeof( structFileDesc_t ) * NUM_STRUCT_FILE_DESC );
        memset( MssoSubFileDesc, 0, sizeof( mssoSubFileDesc_t ) * NUM_MSSO_SUB_FILE_DESC );
    }



    // =-=-=-=-=-=-=-
    //
    irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

        // =-=-=-=-=-=-=-
        // 4a. create tarfilesystem_resource
        mssofilesystem_resource* resc = new mssofilesystem_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b1. set start and stop operations for alloc / free of tables
        resc->set_start_operation( "mssofilesystem_resource_start" );
        resc->set_stop_operation( "mssofilesystem_resource_stop" );

        // =-=-=-=-=-=-=-
        // 4b2. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( irods::RESOURCE_OP_CREATE,       "msso_file_create_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,         "msso_file_open_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READ,         "msso_file_read_plugin" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,        "msso_file_write_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,        "msso_file_close_plugin" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,       "msso_file_unlink_plugin" );
        resc->add_operation( irods::RESOURCE_OP_STAT,         "msso_file_stat_plugin" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,        "msso_file_lseek_plugin" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,        "msso_file_mkdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,        "msso_file_rmdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,      "msso_file_opendir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,     "msso_file_closedir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,      "msso_file_readdir_plugin" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,       "msso_file_rename_plugin" );
        resc->add_operation( irods::RESOURCE_OP_TRUNCATE,     "msso_file_truncate_plugin" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,    "msso_file_getfsfreespace_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,   "msso_file_registered_plugin" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED, "msso_file_unregistered_plugin" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,     "msso_file_modified_plugin" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,    "msso_file_rebalance_plugin" );
        resc->add_operation( irods::RESOURCE_OP_NOTIFY,       "msso_file_notify_plugin" );

        // =-=-=-=-=-=-=-
        // struct file specific operations
        resc->add_operation( "extract",      "msso_file_extract_plugin" );
        resc->add_operation( "sync",         "msso_file_sync_plugin" );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     irods::resource pointer
        return dynamic_cast<irods::resource*>( resc );

    } // plugin_factory

}; // extern "C"



