//#include "rsApiHandler.hpp"
#include "rods.h"
#include "reFuncDefs.hpp"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"

#include <curl/curl.h>
#include "microservice.hpp"

/**
 * \fn msiobjget_http(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an object from an http/https/ftp data source
 *
 * \module msoDrivers_http
 *
 * \since 3.0
 *
 *
 * \usage See clients/icommands/test/rules3.0/
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

MICROSERVICE_BEGIN(
    msiobjget_http,
    STR,      requestPath, INPUT,
    STR,      fileMode, INPUT,
    STR,      fileFlags, INPUT,
    STR,      cacheFilename, INPUT )

int status;
FILE *destFd;
char curlErrBuf[CURL_ERROR_SIZE];
char *reqStr = requestPath;
CURL *curl;
CURLcode res;
( void ) fileMode;
( void ) fileFlags;

curlErrBuf[0] = '\0';

/* Do the processing */
/* opening file and passing i to curl */
destFd = fopen( cacheFilename, "wb" );
if ( destFd ==  0 ) {
    status = UNIX_FILE_OPEN_ERR - errno;
    rodsLog( LOG_ERROR,
             "msigetobj_http: open error for cacheFilename %s",
             cacheFilename );

    RETURN( status );
}
rodsLog( LOG_DEBUG, "CURL: msigetobj_http: Calling with %s\n", reqStr );
curl = curl_easy_init();
if ( !curl ) {
    rodsLog( LOG_ERROR, "Curl Error: msigetobj_http: Initialization failed\n" );
    fclose( destFd );
    RETURN( MSO_OBJ_GET_FAILED );
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
        RETURN( 0 );
    }
    else {
        RETURN( MSO_OBJ_GET_FAILED );
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

// cppcheck-suppress syntaxError
MICROSERVICE_END



