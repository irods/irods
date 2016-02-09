#include "rsApiHandler.hpp"
#include "reFuncDefs.hpp"
#include "rods.h"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"

#include <curl/curl.h>

#define MSO_OBJ_PUT_FAILED -1118000



/**
 * \fn msiobjput_http(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts an http/https/ftp object file
 *
 * \module msoDrivers_http
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

int msiobjput_http(
    msParam_t*      inMSOPath,
    msParam_t*      inCacheFilename,
    msParam_t*      inFileSize,
    ruleExecInfo_t* rei ) {
    char *reqStr;
    char *cacheFilename;
    rodsLong_t dataSize;
    int status;
    FILE *srcFd;

    char curlErrBuf[CURL_ERROR_SIZE];
    CURL *curl;
    CURLcode res;



    RE_TEST_MACRO( "    Calling msiobjput_http" );

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
    reqStr = strdup( ( char * ) inMSOPath->inOutStruct );
    cacheFilename = ( char * ) inCacheFilename->inOutStruct;
    dataSize  = atol( ( char * ) inFileSize->inOutStruct );



    /* Read the cache and Do the upload*/
    srcFd = fopen( cacheFilename, "rb" );
    if ( srcFd == 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        printf( "msiputobj_http: open error for %s, status = %d\n",
                cacheFilename, status );
        free( reqStr );
        return status;
    }


    rodsLog( LOG_DEBUG, "CURL: msiputobj_http: Calling with %s and dataSize=%lld\n", reqStr, dataSize );
    curl = curl_easy_init();
    if ( !curl ) {
        printf( "Curl Error: msiputobj_http: Initialization failed\n" );
        free( reqStr );
        fclose( srcFd );
        return MSO_OBJ_PUT_FAILED;
    }

    curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, curlErrBuf );
    curl_easy_setopt( curl, CURLOPT_URL, reqStr );
    curl_easy_setopt( curl, CURLOPT_READFUNCTION, NULL );
    curl_easy_setopt( curl, CURLOPT_READDATA, srcFd );
    curl_easy_setopt( curl, CURLOPT_UPLOAD, 1L );
    /*  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);*/
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
    curl_easy_setopt( curl, CURLOPT_INFILESIZE_LARGE, ( curl_off_t ) dataSize );
    res = curl_easy_perform( curl );
    fclose( srcFd );
    if ( res != 0 ) {
        rodsLog( LOG_ERROR, "msiputobj_http: Curl Error for %s:ErrNum=%i, Msg=%s\n", reqStr, res, curlErrBuf );
        curl_easy_cleanup( curl );
        free( reqStr );
        return MSO_OBJ_PUT_FAILED;
    }

    long http_code = 0;
    curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &http_code );
    curl_easy_cleanup( curl );

    if ( 200 != http_code ) {
        rodsLog( LOG_ERROR, "msiputobj_http: Curl Error for %s:ErrNum=%ld\n", reqStr, http_code );
        free( reqStr );
        return MSO_OBJ_PUT_FAILED;

    }

    rodsLog( LOG_DEBUG, "CURL: put success with %s\n", reqStr );

    /* clean up */
    free( reqStr );

    return 0;

} // msiobjput_http

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
            "msiobjput_http",
            std::function<int(
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*)>(msiobjput_http));

    // =-=-=-=-=-=-=-
    // hand it over to the system
    return msvc;

} // plugin_factory




