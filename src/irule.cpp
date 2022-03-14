/*
 * irule - The irods utility to execute user composed rules.
*/

#include <irods/rodsClient.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/getUtil.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/irods_configuration_keywords.hpp>

#include <boost/program_options.hpp>

void usage();

int
parseParameters( boost::program_options::variables_map _vm, int ruleGen, execMyRuleInp_t *execMyRuleInp, char *inBuf );

int
printMsParamNew( msParamArray_t *outParamArray, int output ) {
    int i, j;
    keyValPair_t *kVPairs;
    tagStruct_t *tagValues;

    if ( outParamArray == NULL ) {
        return 0;
    }
    for ( i = 0; i < outParamArray->len; i++ ) {
        msParam_t *msParam;

        msParam = outParamArray->msParam[i];

        if ( msParam->label != NULL && strcmp( msParam->label, "ruleExecOut" ) == 0 ) {
            continue;
        }

        if ( msParam->label != NULL &&
                msParam->type != NULL &&
                msParam->inOutStruct != NULL ) {
            if ( strcmp( msParam->type, STR_MS_T ) == 0 ) {
                printf( "%s = %s\n", msParam->label, ( char * ) msParam->inOutStruct );
            }
            else  if ( strcmp( msParam->type, INT_MS_T ) == 0 ) {
                printf( "%s = %i\n", msParam->label, *( int * ) msParam->inOutStruct );
            }
            else  if ( strcmp( msParam->type, DOUBLE_MS_T ) == 0 ) {
                printf( "%s = %f\n", msParam->label, *( double * ) msParam->inOutStruct );
            }
            else if ( strcmp( msParam->type, KeyValPair_MS_T ) == 0 ) {
                kVPairs = ( keyValPair_t * )msParam->inOutStruct;
                printf( "KVpairs %s: %i\n", msParam->label, kVPairs->len );
                for ( j = 0; j < kVPairs->len; j++ ) {
                    printf( "       %s = %s\n", kVPairs->keyWord[j],
                            kVPairs->value[j] );
                }
            }
            else if ( strcmp( msParam->type, TagStruct_MS_T ) == 0 ) {
                tagValues = ( tagStruct_t * ) msParam->inOutStruct;
                printf( "Tags %s: %i\n", msParam->label, tagValues->len );
                for ( j = 0; j < tagValues->len; j++ ) {
                    printf( "       AttName = %s\n", tagValues->keyWord[j] );
                    printf( "       PreTag  = %s\n", tagValues->preTag[j] );
                    printf( "       PostTag = %s\n", tagValues->postTag[j] );
                }
            }
            else if ( strcmp( msParam->type, ExecCmdOut_MS_T ) == 0 && output ) {
                execCmdOut_t *execCmdOut;
                execCmdOut = ( execCmdOut_t * ) msParam->inOutStruct;
                if ( execCmdOut->stdoutBuf.buf != NULL ) {
                    printf( "STDOUT = %s", ( char * ) execCmdOut->stdoutBuf.buf );
                }
                if ( execCmdOut->stderrBuf.buf != NULL ) {
                    printf( "STRERR = %s", ( char * ) execCmdOut->stderrBuf.buf );
                }
            }
            else {
                printf( "%s: %s\n", msParam->label, msParam->type );
            }
        }
        if ( msParam->inpOutBuf != NULL ) {
            printf( "    outBuf: buf length = %d\n", msParam->inpOutBuf->len );
        }

    }
    return 0;
}

irods::error parseProgramOptions(
    int _argc,
    char* _argv[],
    boost::program_options::variables_map &_vm ) {
    namespace po = boost::program_options;

    po::options_description opt_desc( "options" );

    opt_desc.add_options()
        ( "help,h",                              "Show command usage" )
        ( "test,t",                              "Test mode" )
        ( "string,s",                            "String mode" )
        ( "file,F", po::value< std::string >(),  "Rule file" )
        ( "list,l",                              "List file" )
        ( "verbose,v",                           "Verbose output" )
        ( "available,a",                         "List available rule engine instances" )
        ( "rule-engine-plugin-instance,r", po::value< std::string >(),
                                                 "Run rule on specified instance" )
        ( "parameters", po::value< std::vector< std::string > >()->multitoken(),
                                                 "Rule input/output parameters" );

    po::positional_options_description pos_desc;
    pos_desc.add( "parameters", -1 );

    try {
        po::store(
            po::command_line_parser(
            _argc, _argv ).options(
            opt_desc ).positional(
            pos_desc ).run(), _vm );
        po::notify( _vm );
    } catch (po::error& _e ) {
        rodsLog( LOG_ERROR, "Error in irule parseProgramOptions: [%s]", _e.what() );
        std::cerr << std::endl
                  << "Error: "
                  << _e.what()
                  << std::endl << std::endl;
        usage(/*std::cerr*/);
        return ERROR( SYS_INVALID_INPUT_PARAM, "Illegal command line argument" );
    }

    return SUCCESS();
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    boost::program_options::variables_map argsMap;
    irods::error err;
    bool useSaveFile = false;
    execMyRuleInp_t execMyRuleInp;
    msParamArray_t *outParamArray = NULL;
    msParamArray_t msParamArray;
    int rulegen;

    int connFlag = 0;
    char saveFile[MAX_NAME_LEN];
    char ruleFile[MAX_NAME_LEN];
    char cmdLineInput[MAX_NAME_LEN];
    err = parseProgramOptions( argc, argv, argsMap );

    if ( !err.ok() ) {
        std::cerr << "Error in parsing command line arguments" << std::endl;
        exit( 1 );
    }

    if ( argsMap.count( "help" ) ) {
        usage(/*std::cout*/);
        exit( 0 );
    }

    /* init data structures */
    memset( &execMyRuleInp, 0, sizeof( execMyRuleInp ) );
    memset( &msParamArray, 0, sizeof( msParamArray ) );
    execMyRuleInp.inpParamArray = &msParamArray;
    execMyRuleInp.condInput.len = 0;

    /* add key val for test mode */
    if ( argsMap.count( "test" ) ) {
        addKeyVal( &execMyRuleInp.condInput, "looptest", "true" );
    }
    /* add key val for specifying instance on which to run rule */
    if ( argsMap.count( "rule-engine-plugin-instance" ) ) {
        addKeyVal( &execMyRuleInp.condInput, irods::CFG_INSTANCE_NAME_KW.c_str(), argsMap["rule-engine-plugin-instance"].as<std::string>().c_str() );
    }

    load_client_api_plugins();

    /* Don't need to parse parameters if just listing available rule_engine_instances */
    if ( argsMap.count( "available" ) ) {
        /* add key val for listing available rule engine instances */
        addKeyVal( &execMyRuleInp.condInput, "available", "true" );
    }
    else {
        /* read rules from the input file */
        if ( argsMap.count( "file" ) ) {
            FILE *fptr;
            int len;
            int gotRule = 0;
            char buf[META_STR_LEN];
            const char* fileName;

            try {
                fileName = argsMap["file"].as< std::string >().c_str();
            } catch ( boost::bad_any_cast& e ) {
                std::cerr << "Bad filename provided to --file option\n";
                std::cerr << "Use -h or --help for help\n";
                exit( 10 );
            } catch ( std::out_of_range& e ) {
                std::cerr << "No filename provided to --file option\n";
                std::cerr << "Use -h or --help for help\n";
                exit( 10 );
            }

            /* if the input file name starts with "i:", the get the file from iRODS server */
            if ( !strncmp( fileName, "i:", 2 ) ) {
                status = getRodsEnv( &myEnv );

                if ( status < 0 ) {
                    rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
                    exit( 1 );
                }

                conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                        myEnv.rodsZone, 0, &errMsg );

                if ( conn == NULL ) {
                    exit( 2 );
                }

                status = clientLogin( conn );
                if ( status != 0 ) {
                    rcDisconnect( conn );
                    exit( 7 );
                }
                if ( status == 0 ) {
                    char *myargv[3];
                    int myargc, myoptind;
                    rodsPathInp_t rodsPathInp;
                    rodsArguments_t myRodsArgs;
                    connFlag = 1;

                    myargv[0] = strdup( fileName + 2 );
                    myargv[1] = saveFile;
                    myargc = 2;
                    myoptind = 0;
                    const char *fileType = strrchr( fileName, '.' );
                    if ( fileType == NULL ) {
                        printf( "Unsupported input file type\n" );
                        exit( 10 );
                    }
                    if ( strcmp( fileType, ".r" ) == 0 ) {
                        rulegen = 1;
                    }
                    else if ( strcmp( fileType, ".ir" ) == 0 || strcmp( fileType, ".irb" ) == 0 ) {
                        rulegen = 0;
                    }
                    else {
                        rodsLog( LOG_ERROR,
                                "Unsupported input file type %s\n", fileType );
                        exit( 10 );
                    }
                    snprintf( saveFile, MAX_NAME_LEN, "/tmp/tmpiruleFile.%i.%i.%s",
                            ( unsigned int ) time( 0 ), getpid(), rulegen ? "r" : "ir" );
                    status = parseCmdLinePath( myargc, myargv, myoptind, &myEnv,
                            UNKNOWN_OBJ_T, UNKNOWN_FILE_T, 0, &rodsPathInp );
                    status = getUtil( &conn, &myEnv, &myRodsArgs, &rodsPathInp );
                    if ( status < 0 ) {
                        rcDisconnect( conn );
                        exit( 3 );
                    }

                    useSaveFile = true;
                    connFlag = 1;
                }
            }

            if ( useSaveFile ) {
                rstrcpy( ruleFile, saveFile, MAX_NAME_LEN );
            } else {
                rstrcpy( ruleFile, fileName, MAX_NAME_LEN );
            }

            fptr = fopen( ruleFile, "r" );

            /* test if the file can be opened */
            if ( fptr == NULL ) {
                rodsLog( LOG_ERROR, "Cannot open input file %s. errno = %d\n",
                        ruleFile, errno );
                exit( 1 );
            }

            /* test if the file extension is supported */
            const char *fileType = strrchr( ruleFile, '.' );
            if ( fileType == NULL ) {
                printf( "Unsupported input file type\n" );
                exit( 10 );

            }
            else if ( strcmp( fileType, ".r" ) == 0 ) {
                rulegen = 1;
            }
            else if ( strcmp( fileType, ".ir" ) == 0 || strcmp( fileType, ".irb" ) == 0 ) {
                rulegen = 0;
            }
            else {
                rodsLog( LOG_ERROR,
                        "Unsupported input file type %s\n", fileType );
                exit( 10 );
            }

            /* add the @external directive in the rule if the input file is in the new rule engine syntax */
            if ( rulegen ) {
                rstrcpy( execMyRuleInp.myRule, "@external\n", META_STR_LEN );
            }

            while ( ( len = getLine( fptr, buf, META_STR_LEN ) ) > 0 ) {
                if ( argsMap.count( "list" ) ) {
                    puts( buf );
                }

                /* skip comments if the input file is in the old rule engine syntax */
                if ( !rulegen && buf[0] == '#' ) {
                    continue;
                }

                if ( rulegen ) {
                    if ( startsWith( buf, "INPUT" ) || startsWith( buf, "input" ) ) {
                        gotRule = 1;
                        trimSpaces( trimPrefix( buf ) );
                    }
                    else if ( startsWith( buf, "OUTPUT" ) || startsWith( buf, "output" ) ) {
                        gotRule = 2;
                        trimSpaces( trimPrefix( buf ) );
                    }
                }

                if ( gotRule == 0 ) {
                    if ( !rulegen ) {
                        /* the input is a rule */
                        snprintf( execMyRuleInp.myRule + strlen( execMyRuleInp.myRule ), META_STR_LEN - strlen( execMyRuleInp.myRule ), "%s\n", buf );
                    }
                    else {
                        snprintf( execMyRuleInp.myRule + strlen( execMyRuleInp.myRule ), META_STR_LEN - strlen( execMyRuleInp.myRule ), "%s\n", buf );
                    }
                }
                else if ( gotRule == 1 ) {
                    if ( rulegen ) {
                        if ( convertListToMultiString( buf, 1 ) != 0 ) {
                            rodsLog( LOG_ERROR,
                                "Input parameter list format error for %s\n", ruleFile );
                            exit( 10 );
                        }
                    }
                    parseParameters( argsMap, rulegen, &execMyRuleInp, buf );
                }
                else if ( gotRule == 2 ) {
                    if ( rulegen ) {
                        if ( convertListToMultiString( buf, 0 ) != 0 ) {
                            rodsLog( LOG_ERROR,
                                "Output parameter list format error for %s\n", ruleFile );
                            exit( 10 );
                        }
                    }
                    if ( strcmp( buf, "null" ) != 0 ) {
                        rstrcpy( execMyRuleInp.outParamDesc, buf, LONG_NAME_LEN );
                    }
                    break;
                }
                else {
                    break;
                }
                if ( !rulegen ) {
                    gotRule++;
                }
            }

            if ( argsMap.count( "list" ) ) {
                puts( "-----------------------------------------------------------------" );
            }

            if ( gotRule != 2 ) {
                rodsLog( LOG_ERROR, "Incomplete rule input for %s", ruleFile );
                //                     argsMap["file"].as<std::string>().c_str() );
                exit( 2 );
            }
            if ( connFlag == 1 ) {
                fclose( fptr );
                unlink( saveFile );
            }
        }
        else {	/* command line input */
            std::vector< std::string > parameters;
            try {
                parameters = argsMap["parameters"].as< std::vector< std::string> >();
            } catch ( boost::bad_any_cast& e ) {
                std::cerr << "Bad parameter list provided\n";
                std::cerr << "Use -h or --help for help\n";
                exit( 10 );
            } catch ( std::out_of_range& e ) {
                std::cerr << "No parameters list provided\n";
                std::cerr << "Use -h or --help for help\n";
                exit( 10 );
            }

            rulegen = 1;
            if ( parameters.size() < 3 ) {
                rodsLog( LOG_ERROR, "incomplete input" );
                fprintf(stderr, "Use -h for help.\n" );
                exit( 3 );
            }

            snprintf( execMyRuleInp.myRule, META_STR_LEN, "@external rule { %s }", parameters.at(0).c_str() );
            rstrcpy( cmdLineInput, parameters.at(1).c_str(), MAX_NAME_LEN );

            if (0 != parseParameters( argsMap, 1, &execMyRuleInp, cmdLineInput )) {
                rodsLog (LOG_ERROR, "Invalid input parameter list specification");
                fprintf( stderr, "Use -h for help.\n" );
                exit(10);
            }

            if ( parameters.at(2) != "null") {
                rstrcpy( execMyRuleInp.outParamDesc, parameters.at(2).c_str(), LONG_NAME_LEN );
            }
        }
    }

    if ( connFlag == 0 ) {
        status = getRodsEnv( &myEnv );

        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
            exit( 1 );
        }

        conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                          myEnv.rodsZone, 0, &errMsg );

        if ( conn == NULL ) {
            rodsLogError( LOG_ERROR, errMsg.status, "rcConnect failure %s",
                          errMsg.msg );
            exit( 2 );
        }

        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            exit( 7 );
        }
    }

    if ( argsMap.count( "verbose" ) ) {
        printf( "rcExecMyRule: %s\n", rulegen ? execMyRuleInp.myRule + 10 : execMyRuleInp.myRule );
        printf( "outParamDesc: %s\n", execMyRuleInp.outParamDesc );
    }

    status = rcExecMyRule( conn, &execMyRuleInp, &outParamArray );

    if ( argsMap.count( "test" ) ) {
        printErrorStack( conn->rError );
    }

    if ( status < 0 ) {
        msParam_t *mP;
        execCmdOut_t *execCmdOut;

        if ( !rulegen ) {
            rodsLogError( LOG_ERROR, status, "rcExecMyRule error. The rule engine is running under backward compatible mode. To run the rule(s) under normal mode, try renaming the file extension to \".r\". " );

        }
        else {
            rodsLogError( LOG_ERROR, status, "rcExecMyRule error. " );

        }
        printErrorStack( conn->rError );
        if ( ( mP = getMsParamByType( outParamArray, ExecCmdOut_MS_T ) ) != NULL ) {
            execCmdOut = ( execCmdOut_t * ) mP->inOutStruct;
            if ( execCmdOut->stdoutBuf.buf != NULL ) {
                fprintf( stdout, "%s", ( char * ) execCmdOut->stdoutBuf.buf );
            }
            if ( execCmdOut->stderrBuf.buf != NULL ) {
                fprintf( stderr, "%s", ( char * ) execCmdOut->stderrBuf.buf );
            }
        }
        rcDisconnect( conn );
        exit( 4 );
    }

    if ( argsMap.count( "verbose" ) ) {
        printf( "ExecMyRule completed successfully.    Output \n\n" );
        printMsParamNew( outParamArray, 1 );
    }
    else {
        printMsParamNew( outParamArray, 0 );
        msParam_t *mP;
        execCmdOut_t *execCmdOut;
        if ( ( mP = getMsParamByType( outParamArray, ExecCmdOut_MS_T ) ) != NULL ) {
            execCmdOut = ( execCmdOut_t * ) mP->inOutStruct;
            if ( execCmdOut->stdoutBuf.buf != NULL ) {
                fprintf( stdout, "%s", ( char * ) execCmdOut->stdoutBuf.buf );
            }
            if ( execCmdOut->stderrBuf.buf != NULL ) {
                fprintf( stderr, "%s", ( char * ) execCmdOut->stderrBuf.buf );
            }
        }
    }
    if ( argsMap.count( "verbose" ) && conn->rError != NULL ) {
        int i, len;
        rErrMsg_t *errMsg;
        len = conn->rError->len;
        for ( i = 0; i < len; i++ ) {
            errMsg = conn->rError->errMsg[i];
            printf( "%s\n", errMsg->msg );
        }
    }

    printErrorStack( conn->rError );
    rcDisconnect( conn );
    exit( 0 );

}

char *quoteString( const char *str, int string, int label ) {
    char *val = ( char * ) malloc( strlen( str ) * 2 + 2 );
    char *pVal = val;
    const char *pStr = str;
    if ( label ) {
        int prefixLen = strchr( str, '=' ) - str + 1;
        memcpy( val, str, prefixLen );
        pVal += prefixLen;
        pStr += prefixLen;
    }
    if ( !string || ( pStr[0] == '\"' && pStr[strlen( pStr ) - 1] == '\"' ) || ( pStr[0] == '\'' && pStr[strlen( pStr ) - 1] == '\'' ) ) {
        free( val );
        return strdup( str );
    }
    *pVal = '\"';
    pVal ++;
    while ( *pStr != '\0' ) {
        switch ( *pStr ) {
        case '\\':
        case '*':
        case '$':
        case '\'':
        case '\"':
            *pVal = '\\';
            pVal ++;
            break;
        }
        *pVal = *pStr;
        pVal ++;
        pStr ++;
    }
    pVal[0] = '\"';
    pVal[1] = '\0';
    return val;
}

int
parseParameters( boost::program_options::variables_map _vm, int ruleGen, execMyRuleInp_t *execMyRuleInp, char *inBuf ) {
    strArray_t strArray;
    int status, i, j;
    char *value;
    char line[MAX_NAME_LEN];
    int promptF = 0;
    int labelF = 0;

    if ( inBuf == NULL || strcmp( inBuf, "null" ) == 0 ) {
        execMyRuleInp->inpParamArray = NULL;
        return 0;
    }

    memset( &strArray, 0, sizeof( strArray ) );

    status = splitMultiStr( inBuf, &strArray );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "parseMsInputParam: parseMultiStr error, status = %d", status );
        execMyRuleInp->inpParamArray = NULL;
        return status;
    }

    status = 0;

    resizeStrArray( &strArray, MAX_NAME_LEN );
    value = strArray.value;

    if ( _vm.count( "file" ) ) {
        if ( _vm.count( "parameters" ) ) {
            std::vector< std::string > parameters;
            try {
                parameters = _vm["parameters"].as< std::vector< std::string > >();
            } catch ( boost::bad_any_cast& e ) {
                std::cerr << "Bad parameter list provided to parseParameters\n";
                std::cerr << "Use -h or --help for help\n";
                return -1;
            }

            for ( size_t inx = 0; inx < parameters.size(); ++inx ) {
                std::string param = parameters.at(inx);
                /* using the values from the input line following -F <filename> */
                /* each string is supposed to have to format label=value */

                if ( param == "prompt" ) {
                    promptF = 1;
                    break;
                }

                if ( param == "default" || param.length() == 0 ) {
                    continue;
                }
                else if ( param.at(0) == '*' ) {
                    size_t eqInx;
                    std::string tmpStr;
                    if ( inx > 0 && labelF == 0 ) {
                        return CAT_INVALID_ARGUMENT;
                    }
                    labelF = 1;
                    if ( ( eqInx = param.find( "=" ) ) == std::string::npos ) {
                        return CAT_INVALID_ARGUMENT;
                    }

                    tmpStr = param.substr( 0, eqInx );
                    for ( j = 0; j < strArray.len; j++ ) {
                        if ( strstr( &value[j * strArray.size], tmpStr.c_str() ) == &value[j * strArray.size] ) {
                            char *val = quoteString( param.c_str(), _vm.count( "string" ), 1 );
                            rstrcpy( &value[j * strArray.size], val, strArray.size );
                            free( val );
                            break;
                        }
                    }
                    if ( j == strArray.len ) {
                        printf( "Ignoring Argument \"%s\"\n", param.c_str() );
                    }
                } else {
                    char *valPtr = &value[inx * strArray.size];
                    char *tmpPtr;

                    if ( labelF == 1 ) {
                        return CAT_INVALID_ARGUMENT;
                    }
                    if ( ( tmpPtr = strstr( valPtr, "=" ) ) != NULL ) {
                        tmpPtr++;
                        char *val = quoteString( param.c_str(), _vm.count( "string" ), 0 );
                        rstrcpy( tmpPtr, val,
                                strArray.size - ( tmpPtr - valPtr + 1 ) );
                        free( val );
                    }
                }
            }
        }
    }

    for ( i = 0; i < strArray.len; i++ ) {
        char *valPtr = &value[i * strArray.size];
        char *tmpPtr;

        if ( ( tmpPtr = strstr( valPtr, "=" ) ) != NULL ) {
            *tmpPtr = '\0';
            tmpPtr++;
            if ( *tmpPtr == '$' ) {
                /* If $ is used as a value in the input file for label=value then
                 the remaining command line arguments are taken as values.
                 If no command line arguments are given then the user is prompted
                 for the input value
                 */
                printf( "Default %s=%s\n    New %s=", valPtr, tmpPtr + 1, valPtr );
                if ( fgets( line, MAX_NAME_LEN, stdin ) == NULL ) {
                    return CAT_INVALID_ARGUMENT;
                }
                size_t line_len = strlen( line );
                if ( line_len > 0 && '\n' == line[line_len - 1] ) {
                    line[line_len - 1] = '\0';
                    line_len--;
                }
                char *val = line_len > 0 ?
                            quoteString( line, _vm.count( "string" ) && ruleGen, 0 ) :
                            strdup( tmpPtr + 1 );
                addMsParam( execMyRuleInp->inpParamArray, valPtr, STR_MS_T,
                            val, NULL );
            }
            else if ( promptF == 1 ) {
                /* the user has asked for prompting */
                printf( "Current %s=%s\n    New %s=", valPtr, tmpPtr, valPtr );
                if ( fgets( line, MAX_NAME_LEN, stdin ) == NULL ) {
                    return CAT_INVALID_ARGUMENT;
                }
                size_t line_len = strlen( line );
                if ( line_len > 0 && '\n' == line[line_len - 1] ) {
                    line[line_len - 1] = '\0';
                    line_len--;
                }
                char *val = line_len > 0 ?
                            quoteString( line, _vm.count( "string" ) && ruleGen, 0 ) :
                            strdup( tmpPtr );
                addMsParam( execMyRuleInp->inpParamArray, valPtr, STR_MS_T,
                            val, NULL );
            }
            else if ( *tmpPtr == '\\' ) {
                /* first '\'  is skipped.
                 If you need to use '\' in the first letter add an additional '\'
                 if you have to use '$' in the first letter add a '\'  before that
                 */
                tmpPtr++;
                char *param = quoteString( tmpPtr, _vm.count( "string" ) && ruleGen, 0 );
                if ( !ruleGen ) {
                    trimQuotes( param );
                }

                addMsParam( execMyRuleInp->inpParamArray, valPtr, STR_MS_T,
                            param, NULL );
            }
            else {
                char *param = quoteString( tmpPtr, _vm.count( "string" ) && ruleGen, 0 );
                if ( !ruleGen ) {
                    trimQuotes( param );
                }

                addMsParam( execMyRuleInp->inpParamArray, valPtr, STR_MS_T,
                            param, NULL );
            }
        }
        else {
            rodsLog( LOG_ERROR,
                     "parseMsInputParam: inpParam %s format error", valPtr );
            status = CAT_INVALID_ARGUMENT;
        }
    }

    return status;
}

void
usage() {
    char *msgs[] = {
        "Usage: irule [--available]",
        "Usage: irule [--test] [-v] [-r instanceName] rule inputParam outParamDesc",
        "Usage: irule [--test] [-v] [-l] [-r instanceName] -F inputFile [prompt | arg_1 arg_2 ...]",
        " ",
        "Submit a user defined rule to be executed by an iRODS server.",
        " ",
        "The first form above requires 3 inputs: ",
        "    1) rule - This is the rule to be executed.",
        "    2) inputParam - The input parameters. The input values for the rule is",
        "       specified here. If there is no input, a string containing \"null\"",
        "       must be specified.",
        "    3) outParamDesc - Description for the set of output parameters to be ",
        "       returned. If there is no output, a string containing \"null\"",
        "       must be specified.",
        " ",
        "The second form above reads the rule and arguments from the file: inputFile",
        "    - Arguments following the inputFile are interpreted as input arguments for the rule.",
        "    - These arguments are interpreted in two ways:",
        "        1) The arguments require the \"label=value\" format.",
        "            - Only named pairs are replaced. Any remaining arguments",
        "                 get their values from the inputFile.  All labels start with *.",
        "        2) All arguments can be specified as inputs without any labels",
        "            - The keyword \"default\" can be given to use the inputFile value.",
        "            - Use \\ as the first letter in an argument as an escape.",
        " ",
        "The inputFile should contain 3 lines of non-comments:",
        "    1) The first line specifies the rule.",
        "    2) The second line specifies the input arguments as label=value pairs separated by %",
        "        - If % is needed in an input value, use %%.",
        "        - An input value can begin with $. In such a case the user will be prompted.",
        "             A default value can be listed after the $, as in *A=$40.",
        "             If prompted, the default value will be used if the user",
        "                presses return without giving a replacement.",
        "        - The input line can just be the word null.",
        "    3) The third line contains output parameters as labels separated by %.",
        "        - The built-in variable ruleExecOut stores output from the rule",
        "             generated by microservices.",
        "        - The output line can just be the word null.",
        " ",
        "'arg_1' is of the form *arg=value.  For example, using *A=$ as one of",
        "the arguments in line 2 of the inputFile, irule will prompt for *A or",
        "the user can enter it on the command line:",
        "    irule -F filename *A=23",
        "     (integer parameters can be unquoted)",
        "    or",
        "    irule -F filename \"*A='/path/of/interest'\" *B=24",
        "    irule -F filename \"*A=\\\"/path/of/interest\\\"\" *B=24",
        "     (string parameters must be quoted)",
        "     (your shell may require escaping and/or single quotes)",
        " ",
        "Example rules can be found at:",
        "    https://github.com/irods/irods_client_icommands/tree/master/test/rules",
        " ",
        "In either form, the 'rule' that is processed is either a rule name or a",
        "rule definition (which may be a complete rule or a subset).",
        " ",
        "To view the output parameters (outParamDesc), use the -v option.",
        " ",
        "Options are:",
        " --test,-t                         - enable test mode so that the microservices are not executed,",
        "                                     instead a loopback is performed",
        " --string,-s                       - enable string mode, in string mode all command line input arguments do not need",
        "                                     to be quoted and are automatically converted to strings",
        "                                     string mode does not affect input parameter values in rule files",
        " --file,-F                         - read the named inputFile",
        "                                     if the inputFile begins with the prefix \"i:\"",
        "                                     then the file is fetched from an iRODS server",
        " --list,-l                         - list file if -F option is used",
        " --verbose,-v                      - verbose",
        " --available,-a                    - list all available rule engine instances",
        " --rule-engine-plugin-instance,-r  - run rule on the specified rule engine instance",
        " --help,-h                         - this help",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "irule" );
}
