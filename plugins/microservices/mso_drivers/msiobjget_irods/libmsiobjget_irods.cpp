#include "rodsClient.h"
#include "reFuncDefs.hpp"
#include "rods.h"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <string>
#include "irods_server_properties.hpp"


static
int connectToRemoteiRODS(
        char*      inStr,
        rcComm_t** rcComm ) {
    rErrMsg_t errMsg;

    /*inStr of form: //irods:host[:port][:user[@zone][:pass]]/remotePath
      if port is not given default port 1247 is used
      if user@zone is not given ANONYMOUS_USER is used
      if pass is not given ANONYMOUS_USER is used
      */
    const std::string input( inStr );
    boost::smatch matches;
    boost::regex expression( "irods:([^:/]+)(:([0-9]+))?"           // irods:host[:port]
            "(:([^:/@]+)(@([^:/]+))?(:([^/]+))?)?" // [:user[@zone][:pass]]
            "/(.*)" );                             // /remotePath
    try {
        const bool matched = boost::regex_match( input, matches, expression );
        if ( !matched ) {
            return USER_INPUT_FORMAT_ERR;
        }
        std::string host = matches.str( 1 );
        int port = 1247;
        if ( !matches.str( 3 ).empty() ) {
            port = boost::lexical_cast<int>( matches.str( 3 ) );
        }
        std::string user( ANONYMOUS_USER );
        if ( !matches.str( 5 ).empty() ) {
            user = matches.str( 5 );
        }
        std::string zone = matches.str( 7 );
        printf( "MM: host=%s,port=%i,user=%s\n", host.c_str(), port, user.c_str() );
        *rcComm = rcConnect( host.c_str(), port, user.c_str(), zone.c_str(), 0, &errMsg );
        if ( *rcComm == NULL ) {
            return REMOTE_IRODS_CONNECT_ERR;
        }
        int status = clientLogin( *rcComm );
        if ( status != 0 ) {
            rcDisconnect( *rcComm );
        }
        return status;
    }
    catch ( const boost::bad_lexical_cast& ) {
        return INVALID_LEXICAL_CAST;
    }
    catch ( const boost::exception& ) {
        return SYS_INTERNAL_ERR;
    }

}

/**
 * \fn msiobjget_irods(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an iRODS object
 *
 * \module msoDrivers_irods
 *
 * \since 3.0
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] inRequestPath - a STR_MS_T request string to external resource
 * \param[in] inFileMode - a STR_MS_T mode of cache file creation
 * \param[in] inFileFlags - a STR_MS_T flags for cache file creation
 * \param[in] inCacheFilename - a STR_MS_T cache file name (full path)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/

int msiobjget_irods(
        msParam_t* inRequestPath,
        msParam_t* inFileMode,
        msParam_t* inFileFlags,
        msParam_t* inCacheFilename,
        ruleExecInfo_t* rei ) {

    char *locStr;
    int mode;
    char *cacheFilename;
    char *str, *t;
    int status, bytesRead, bytesWritten;
    int destFd, i;
    rcComm_t *rcComm = NULL;

    dataObjInp_t dataObjInp;
    int objFD;
    openedDataObjInp_t dataObjReadInp;
    openedDataObjInp_t dataObjCloseInp;
    bytesBuf_t readBuf;



    RE_TEST_MACRO( "    Calling msiobjget_irods" );

    /*  check for input parameters */
    if ( inRequestPath ==  NULL ||
            strcmp( inRequestPath->type , STR_MS_T ) != 0 ||
            inRequestPath->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( inFileMode ==  NULL ||
            strcmp( inFileMode->type , STR_MS_T ) != 0 ||
            inFileMode->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( inFileFlags ==  NULL ||
            strcmp( inFileFlags->type , STR_MS_T ) != 0 ||
            inFileFlags->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( inCacheFilename ==  NULL ||
            strcmp( inCacheFilename->type , STR_MS_T ) != 0 ||
            inCacheFilename->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    /*  coerce input to local variables */
    str = strdup( ( char * ) inRequestPath->inOutStruct );
    if ( ( t = strstr( str, ":" ) ) != NULL ) {
        locStr = t + 1;
        if ( ( t = strstr( locStr, "/" ) ) != NULL ) {
            locStr = t;
        }
        else {
            free( str );
            return USER_INPUT_FORMAT_ERR;
        }
    }

    else {
        free( str );
        return USER_INPUT_FORMAT_ERR;
    }


    cacheFilename = ( char * ) inCacheFilename->inOutStruct;
    mode  = atoi( ( char * ) inFileMode->inOutStruct );

    /* Do the processing */

    i = connectToRemoteiRODS( ( char * ) inRequestPath->inOutStruct, &rcComm );
    if ( i < 0 ) {
        printf( "msiputobj_irods: error connecting to remote iRODS: %s:%i\n",
                ( char * ) inRequestPath->inOutStruct, i );
        free( str );
        return i;
    }


    bzero( &dataObjInp, sizeof( dataObjInp ) );
    bzero( &dataObjReadInp, sizeof( dataObjReadInp ) );
    bzero( &dataObjCloseInp, sizeof( dataObjCloseInp ) );
    bzero( &readBuf, sizeof( readBuf ) );

    dataObjInp.openFlags = O_RDONLY;
    rstrcpy( dataObjInp.objPath, locStr, MAX_NAME_LEN );
    free( str );

    objFD = rcDataObjOpen( rcComm, &dataObjInp );
    if ( objFD < 0 ) {
        printf( "msigetobj_irods: Unable to open file %s:%i\n", dataObjInp.objPath, objFD );
        rcDisconnect( rcComm );
        return objFD;
    }

    destFd = open( cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode );
    if ( destFd < 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        printf(
                "msigetobj_irods: open error for cacheFilename %s, status = %d",
                cacheFilename, status );
        rcDisconnect( rcComm );
        return status;
    }

    dataObjReadInp.l1descInx = objFD;
    dataObjCloseInp.l1descInx = objFD;

    int single_buff_sz = 0;
    irods::error ret = irods::get_advanced_setting<int>(
            irods::CFG_MAX_SIZE_FOR_SINGLE_BUFFER,
            single_buff_sz );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        close( destFd );
        return ret.code();
    }
    single_buff_sz *= 1024 * 1024;
    readBuf.len = single_buff_sz;
    readBuf.buf = ( char * )malloc( readBuf.len );
    dataObjReadInp.len = readBuf.len;

    while ( ( bytesRead = rcDataObjRead( rcComm, &dataObjReadInp, &readBuf ) ) > 0 ) {
        bytesWritten = write( destFd, readBuf.buf, bytesRead );
        if ( bytesWritten != bytesRead ) {
            free( readBuf.buf );
            close( destFd );
            rcDataObjClose( rcComm, &dataObjCloseInp );
            rcDisconnect( rcComm );
            printf(
                    "msigetobj_irods: In Cache File %s bytesWritten %d != returned objLen %i\n",
                    cacheFilename, bytesWritten, bytesRead );
            return SYS_COPY_LEN_ERR;
        }
    }
    free( readBuf.buf );
    close( destFd );
    i = rcDataObjClose( rcComm, &dataObjCloseInp );
    rcDisconnect( rcComm );
    return i;

}

// =-=-=-=-=-=-=-
// plugin factory
extern "C"
irods::ms_table_entry*  plugin_factory( ) {
    // =-=-=-=-=-=-=-
    // instantiate a new msvc plugin
    irods::ms_table_entry* msvc = new irods::ms_table_entry( 4 );

    // =-=-=-=-=-=-=-
    // wire the implementation to the plugin instance
    msvc->add_operation<
        msParam_t*,
        msParam_t*,
        msParam_t*,
        msParam_t*,
        ruleExecInfo_t*>(
                "msiobjget_irods",
                std::function<int(
                    msParam_t*,
                    msParam_t*,
                    msParam_t*,
                    msParam_t*,
                    ruleExecInfo_t*)>(msiobjget_irods));

    // =-=-=-=-=-=-=-
    // hand it over to the system
    return msvc;

} // plugin_factory





