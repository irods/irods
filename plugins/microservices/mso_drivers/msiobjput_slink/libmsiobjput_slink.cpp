#include "rodsClient.h"
#include "reFuncDefs.hpp"
#include "rods.h"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "irods_server_properties.hpp"


/**
 * \fn msiobjput_slink(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts an SLINK object
 *
 * \module msoDrivers_slink
 *
 * \since 3.0
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] inMSOPath - a STR_MS_T path string to external resource
 * \param[in] inCacheFilename - a STR_MS_T cache file containing data to be written out
 * \param[in] inFileSize - a STR_MS_T size of inCacheFilename
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

int msiobjput_slink(
    msParam_t*      inMSOPath,
    msParam_t*      inCacheFilename,
    msParam_t*      inFileSize,
    ruleExecInfo_t* rei ) {

    RE_TEST_MACRO( "    Calling msiobjput_slink" );

    /*  check for input parameters */
    if ( inMSOPath ==  NULL ||
            strcmp( inMSOPath->type , STR_MS_T ) != 0 ||
            inMSOPath->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( inCacheFilename ==  NULL ||
            strcmp( inCacheFilename->type , STR_MS_T ) != 0 ||
            inCacheFilename->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( inFileSize ==  NULL ||
            strcmp( inFileSize->type , STR_MS_T ) != 0 ||
            inFileSize->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    /*  coerce input to local variables */
    char * str = strdup( ( char * ) inMSOPath->inOutStruct );
    char * reqStr = strstr( str, ":" );
    if ( reqStr == NULL ) {
        free( str );
        return USER_INPUT_FORMAT_ERR;
    }
    reqStr = reqStr + 1;

    dataObjInp_t dataObjInp;
    memset( &dataObjInp, 0, sizeof( dataObjInp_t ) );
    rstrcpy( dataObjInp.objPath, reqStr, MAX_NAME_LEN );
    addKeyVal( &dataObjInp.condInput, FORCE_FLAG_KW, "" );
    free( str );

    rsComm_t * rsComm = rei->rsComm;
    int outDesc = rsDataObjCreate( rsComm, &dataObjInp );
    if ( outDesc < 0 ) {
        printf( "msiputobj_slink: Unable to open file %s:%i\n", dataObjInp.objPath, outDesc );
        return outDesc;
    }

    /* Read the cache and Do the upload*/
    char * cacheFilename = ( char * ) inCacheFilename->inOutStruct;
    int srcFd = open( cacheFilename, O_RDONLY, 0 );
    if ( srcFd < 0 ) {
        int status = UNIX_FILE_OPEN_ERR - errno;
        printf( "msiputobj_slink: open error for %s, status = %d\n",
                cacheFilename, status );
        return status;
    }

    int single_buff_sz_in_mb = 0;
    irods::error ret = irods::get_advanced_setting<int>(
                           irods::CFG_MAX_SIZE_FOR_SINGLE_BUFFER,
                           single_buff_sz_in_mb );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        close( srcFd );
        return ret.code();
    }
    size_t single_buff_sz = single_buff_sz_in_mb * 1024 * 1024;

    size_t dataSize  = atol( ( char * ) inFileSize->inOutStruct );
    if ( dataSize > single_buff_sz ) {
        dataSize = single_buff_sz;
    }

    openedDataObjInp_t dataObjWriteInp;
    memset( &dataObjWriteInp, 0, sizeof( dataObjWriteInp ) );
    dataObjWriteInp.l1descInx = outDesc;

    openedDataObjInp_t dataObjCloseInp;
    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = outDesc;

    char * myBuf = ( char * ) malloc( dataSize );
    bytesBuf_t writeBuf;
    writeBuf.buf = myBuf;

    int bytesRead;
    for ( bytesRead = read( srcFd, ( void * ) myBuf, dataSize ); bytesRead > 0;
            bytesRead = read( srcFd, ( void * ) myBuf, dataSize ) ) {
        writeBuf.len = bytesRead;
        dataObjWriteInp.len = bytesRead;
        int bytesWritten = rsDataObjWrite( rsComm, &dataObjWriteInp, &writeBuf );
        if ( bytesWritten != bytesRead ) {
            free( myBuf );
            close( srcFd );
            rsDataObjClose( rsComm, &dataObjCloseInp );
            printf( "msiputobj_slink: Write Error: bytesRead %d != bytesWritten %d\n",
                    bytesRead, bytesWritten );
            return SYS_COPY_LEN_ERR;
        }
    }
    free( myBuf );
    close( srcFd );
    return rsDataObjClose( rsComm, &dataObjCloseInp );
}



// =-=-=-=-=-=-=-
// plugin factory
extern "C"
irods::ms_table_entry*  plugin_factory( ) {
    // =-=-=-=-=-=-=-
    // instantiate a new msvc plugin
    irods::ms_table_entry* msvc = new irods::ms_table_entry( 3 );

    // =-=-=-=-=-=-=-
    // wire the implementation to the plugin instance
    msvc->add_operation<
        msParam_t*,
        msParam_t*,
        msParam_t*,
        ruleExecInfo_t*>(
            "msiobjput_slink",
            std::function<int(
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*)>(msiobjput_slink));

    // =-=-=-=-=-=-=-
    // hand it over to the system
    return msvc;

} // plugin_factory




