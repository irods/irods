//#include "rsApiHandler.hpp"
#include "rods.h"
#include "reFuncDefs.hpp"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"

#include <curl/curl.h>
#include "microservice.hpp"

/**
 * \fn msiobjget_http(msParam_t*  requestPath, msParam_t* fileMode, msParam_t* fileFlags, msParam_t* cacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an object from an http/https/ftp data source
 *
 * \module msoDrivers_http
 *
 * \since 3.0
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] requestPath - a STR_MS_T request string to external resource
 * \param[in] inFileMode - a STR_MS_T mode of cache file creation
 * \param[in] fileFlags - a STR_MS_T flags for cache file creation
 * \param[in] cacheFilename - a STR_MS_T cache file name (full path)
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
int msiobjget_http(
    msParam_t* requestPath,
    msParam_t*,
    msParam_t*,
    msParam_t* cacheFilename,
    ruleExecInfo_t* rei ) {

    int status;
    FILE *destFd;
    char curlErrBuf[CURL_ERROR_SIZE];
    char *reqStr = (char*)requestPath->inOutStruct;
    char *cacheStr = (char*)cacheFilename->inOutStruct;
    CURL *curl;
    CURLcode res;

    curlErrBuf[0] = '\0';

    /*  check for input parameters */
    if ( requestPath ==  NULL ||
            strcmp( requestPath->type , STR_MS_T ) != 0 ||
            requestPath->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( cacheFilename ==  NULL ||
            strcmp( cacheFilename->type , STR_MS_T ) != 0 ||
            cacheFilename->inOutStruct == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }

    /* Do the processing */
    /* opening file and passing i to curl */
    destFd = fopen( cacheStr, "wb" );
    if ( destFd ==  0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog( LOG_ERROR,
                 "msigetobj_http: open error for cacheFilename %s",
                 cacheStr );

        return status;
    }
    rodsLog( LOG_DEBUG, "CURL: msigetobj_http: Calling with %s\n", reqStr );
    curl = curl_easy_init();
    if ( !curl ) {
        rodsLog( LOG_ERROR, "Curl Error: msigetobj_http: Initialization failed\n" );
        fclose( destFd );
        return MSO_OBJ_GET_FAILED;
    }

    curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, curlErrBuf );
    curl_easy_setopt( curl, CURLOPT_URL, reqStr );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, NULL );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, destFd );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1 );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );

    res = curl_easy_perform( curl );
    fclose( destFd );
    if ( res != 0 ) {
        rodsLog( LOG_ERROR,
                 "msigetobj_http: Curl Error for %s:ErrNum=%i, Msg=%s\n",
                 reqStr, res, curlErrBuf );
        curl_easy_cleanup( curl );
        /***    return(MSO_OBJ_GET_FAILED);***/
        rodsLog( LOG_ERROR, "msigetobj_http:apiNumber:%d\n",
                 rei->rsComm->apiInx );
        if ( rei->rsComm->apiInx == 606 ) {
            return 0;
        }
        else {
            return MSO_OBJ_GET_FAILED;
        }
    }

    long http_code = 0;
    curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &http_code );
    curl_easy_cleanup( curl );

    if ( 200 != http_code ) {
        rodsLog( LOG_ERROR, "msigetobj_http: Curl Error for %s:ErrNum=%ld\n", reqStr, http_code );
        free( reqStr );
        return MSO_OBJ_GET_FAILED;

    }

    rodsLog( LOG_DEBUG, "CURL: get success with %s\n", reqStr );

    return 0;
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
            "msiobjget_http",
            std::function<int(
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*)>(msiobjget_http));

    // =-=-=-=-=-=-=-
    // hand it over to the system
    return msvc;

} // plugin_factory




