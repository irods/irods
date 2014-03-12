


//#include "rsApiHandler.hpp"
#include "rods.hpp"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"

#include <curl/curl.h>


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
extern "C" {

    int msiobjget_http(
        msParam_t*      inRequestPath,
        msParam_t*      inFileMode,
        msParam_t*      inFileFlags,
        msParam_t*      inCacheFilename,
        ruleExecInfo_t* rei ) {
        char *reqStr;
        //int mode, flags;
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
        //mode  = atoi((char *) inFileMode->inOutStruct);
        //flags = atoi((char *) inFileFlags->inOutStruct);
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
            fclose( destFd );
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
            printf(
                "msigetobj_http: Curl Error for %s:ErrNum=%i, Msg=%s\n",
                reqStr, res, curlErrBuf );
            curl_easy_cleanup( curl );
            free( reqStr );
            /***    return(MSO_OBJ_GET_FAILED);***/
            printf( "msigetobj_http:apiNumber:%d\n",
                    rei->rsComm->apiInx );
            if ( rei->rsComm->apiInx == 606 ) {
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

    } // msiobjget_http

    // =-=-=-=-=-=-=-
    // plugin factory
    irods::ms_table_entry*  plugin_factory( ) {
        // =-=-=-=-=-=-=-
        // instantiate a new msvc plugin
        irods::ms_table_entry* msvc = new irods::ms_table_entry( 4 );

        // =-=-=-=-=-=-=-
        // wire the implementation to the plugin instance
        msvc->add_operation( "msiobjget_http", "msiobjget_http" );

        // =-=-=-=-=-=-=-
        // hand it over to the system
        return msvc;

    } // plugin_factory

} // extern "C"




