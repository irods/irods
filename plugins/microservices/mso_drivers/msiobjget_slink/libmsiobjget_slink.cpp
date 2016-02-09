#include "rodsClient.h"
#include "rods.h"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "reFuncDefs.hpp"
#include "irods_server_properties.hpp"

#include <strings.h>

extern int rsDataObjWrite( rsComm_t *rsComm,
                           openedDataObjInp_t *dataObjWriteInp,
                           bytesBuf_t *dataObjWriteInpBBuf );
extern int rsDataObjRead( rsComm_t *rsComm,
                          openedDataObjInp_t *dataObjReadInp,
                          bytesBuf_t *dataObjReadOutBBuf );
extern int rsDataObjOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
extern int rsDataObjClose( rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp );
extern int rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp );




/**
 * \fn msiobjget_slink(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an SLINK object
 *
 * \module msoDrivers_slink
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

int msiobjget_slink(
    msParam_t*      inRequestPath,
    msParam_t*      inFileMode,
    msParam_t*      inFileFlags,
    msParam_t*      inCacheFilename,
    ruleExecInfo_t* rei ) {

    char *locStr;
    int mode;
    char *cacheFilename;
    char *str, *t;
    int status, bytesRead, bytesWritten;
    int destFd, i;
    rsComm_t *rsComm;

    dataObjInp_t dataObjInp;
    int objFD;
    openedDataObjInp_t dataObjReadInp;
    openedDataObjInp_t dataObjCloseInp;
    bytesBuf_t readBuf;


    RE_TEST_MACRO( "    Calling msiobjget_slink" );

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
    }
    else {
        free( str );
        return USER_INPUT_FORMAT_ERR;
    }


    cacheFilename = ( char * ) inCacheFilename->inOutStruct;
    mode  = atoi( ( char * ) inFileMode->inOutStruct );
    rsComm = rei->rsComm;

    /* Do the processing */
    bzero( &dataObjInp, sizeof( dataObjInp ) );
    bzero( &dataObjReadInp, sizeof( dataObjReadInp ) );
    bzero( &dataObjCloseInp, sizeof( dataObjCloseInp ) );
    bzero( &readBuf, sizeof( readBuf ) );

    dataObjInp.openFlags = O_RDONLY;
    rstrcpy( dataObjInp.objPath, locStr, MAX_NAME_LEN );
    free( str );

    objFD = rsDataObjOpen( rsComm, &dataObjInp );
    if ( objFD < 0 ) {
        printf( "msigetobj_slink: Unable to open file %s:%i\n", dataObjInp.objPath, objFD );
        return objFD;
    }

    destFd = open( cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode );
    if ( destFd < 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        printf(
            "msigetobj_slink: open error for cacheFilename %s, status = %d",
            cacheFilename, status );
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

    while ( ( bytesRead = rsDataObjRead( rsComm, &dataObjReadInp, &readBuf ) ) > 0 ) {
        bytesWritten = write( destFd, readBuf.buf, bytesRead );
        if ( bytesWritten != bytesRead ) {
            free( readBuf.buf );
            close( destFd );
            rsDataObjClose( rsComm, &dataObjCloseInp );
            printf(
                "msigetobj_slink: In Cache File %s bytesWritten %d != returned objLen %i\n",
                cacheFilename, bytesWritten, bytesRead );
            return SYS_COPY_LEN_ERR;
        }
    }
    free( readBuf.buf );
    close( destFd );
    i = rsDataObjClose( rsComm, &dataObjCloseInp );

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
            "msiobjget_slink",
            std::function<int(
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*)>(msiobjget_slink));

    // =-=-=-=-=-=-=-
    // hand it over to the system
    return msvc;

} // plugin_factory





