

#include "rsApiHandler.hpp"
#include "rods.hpp"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"

#include <curl/curl.h>

#define MSO_OBJ_PUT_FAILED -1118000

/**
 * \fn msiobjget_http(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an object from an http/https/ftp data source
 *
 * \module msoDrivers_http
 *
 * \since 3.0
 *
 * \author  Arcot Rajasekar
 * \date    2011
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
int
msiobjget_http( msParam_t*  inRequestPath, msParam_t* inFileMode,
                msParam_t* inFileFlags, msParam_t* inCacheFilename,
                ruleExecInfo_t* rei ) {
#if defined(MSO_HTTP)
    char *reqStr;
    int mode, flags;
    char *cacheFilename;
    int status;
    FILE *destFd;
    char curlErrBuf[CURL_ERROR_SIZE];

    CURL *curl;
    CURLcode res;



    RE_TEST_MACRO( "    Calling msiobjget_http" );
    curlErrBuf[0] = '\0';

    /*  check for input parameters */
    if ( inRequestPath ==  NULL ||
            strcmp( inRequestPath->type , STR_MS_T ) != 0 ||
            inRequestPath->inOutStruct == NULL ) {
        return( USER_PARAM_TYPE_ERR );
    }

    if ( inFileMode ==  NULL ||
            strcmp( inFileMode->type , STR_MS_T ) != 0 ||
            inFileMode->inOutStruct == NULL ) {
        return( USER_PARAM_TYPE_ERR );
    }

    if ( inFileFlags ==  NULL ||
            strcmp( inFileFlags->type , STR_MS_T ) != 0 ||
            inFileFlags->inOutStruct == NULL ) {
        return( USER_PARAM_TYPE_ERR );
    }

    if ( inCacheFilename ==  NULL ||
            strcmp( inCacheFilename->type , STR_MS_T ) != 0 ||
            inCacheFilename->inOutStruct == NULL ) {
        return( USER_PARAM_TYPE_ERR );
    }

    /*  coerce input to local variables */
    cacheFilename = ( char * ) inCacheFilename->inOutStruct;
    mode  = atoi( ( char * ) inFileMode->inOutStruct );
    flags = atoi( ( char * ) inFileFlags->inOutStruct );
    reqStr = strdup( ( char * ) inRequestPath->inOutStruct );


    /* Do the processing */
    /* opening file and passing i to curl */
    destFd = fopen( cacheFilename, "wb" );
    if ( destFd ==  0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        printf(
            "msigetobj_http: open error for cacheFilename %s",
            cacheFilename );
        free( reqStr );
        return status;
    }
    printf( "CURL: msigetobj_http: Calling with %s\n", reqStr );
    curl = curl_easy_init();
    if ( !curl ) {
        printf( "Curl Error: msigetobj_http: Initialization failed\n" );
        free( reqStr );
        return( MSO_OBJ_GET_FAILED );
    }

    curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, curlErrBuf );
    curl_easy_setopt( curl, CURLOPT_URL, reqStr );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, NULL );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, destFd );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1 );
    /*   if (reqStr[0] != '\0' && reqStr[strlen(reqStr)-1] == '/')
    curl_easy_setopt(curl, CURLOPT_FTPLISTONLY, TRUE); */
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );

    res = curl_easy_perform( curl );
    fclose( destFd );
    if ( res != 0 ) {
        printf( " msigetobj_http: Curl Error for %s:ErrNum=%i, Msg=%s\n", reqStr, res, curlErrBuf );
        curl_easy_cleanup( curl );
        free( reqStr );
        /***    return(MSO_OBJ_GET_FAILED);***/
        printf( " msigetobj_http:apiNumber:%d\n", RsApiTable[rei->rsComm->apiInx].apiNumber );
        if ( RsApiTable[rei->rsComm->apiInx].apiNumber == 606 ) {
            return( 0 );
        }
        else {
            return( MSO_OBJ_GET_FAILED );
        }
    }
    curl_easy_cleanup( curl );

    printf( "CURL: get success with %s\n", reqStr );

    /* clean up */
    free( reqStr );

    /*return */
    return( 0 );
#else
    return( MICRO_SERVICE_OBJECT_TYPE_UNDEFINED );
#endif /* MSO_HTTP */

}





/**
 * \fn msiobjput_http(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts an http/https/ftp object file
 *
 * \module msoDrivers_http
 *
 * \since 3.0
 *
 * \author  Arcot Rajasekar
 * \date    2011
 *
 * \usage See clients/icommands/test/rules3.0/
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
int
msiobjput_http( msParam_t*  inMSOPath, msParam_t*  inCacheFilename,
                msParam_t*  inFileSize, ruleExecInfo_t* rei ) {
#if defined(MSO_HTTP)
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
        return( USER_PARAM_TYPE_ERR );
    }

    if ( inCacheFilename ==  NULL ||
            strcmp( inCacheFilename->type , STR_MS_T ) != 0 ||
            inCacheFilename->inOutStruct == NULL ) {
        return( USER_PARAM_TYPE_ERR );
    }

    if ( inFileSize ==  NULL ||
            strcmp( inFileSize->type , STR_MS_T ) != 0 ||
            inFileSize->inOutStruct == NULL ) {
        return( USER_PARAM_TYPE_ERR );
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


    printf( "CURL: msiputobj_http: Calling with %s and dataSize=%lld\n", reqStr, dataSize );
    curl = curl_easy_init();
    if ( !curl ) {
        printf( "Curl Error: msiputobj_http: Initialization failed\n" );
        free( reqStr );
        return( MSO_OBJ_PUT_FAILED );
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
        printf( "msiputobj_http: Curl Error for %s:ErrNum=%i, Msg=%s\n", reqStr, res, curlErrBuf );
        curl_easy_cleanup( curl );
        free( reqStr );
        return( MSO_OBJ_PUT_FAILED );
    }
    curl_easy_cleanup( curl );

    printf( "CURL: put success with %s\n", reqStr );

    /* clean up */
    free( reqStr );

    return( 0 );

#else
    return( MICRO_SERVICE_OBJECT_TYPE_UNDEFINED );
#endif /* MSO_HTTP */
}

