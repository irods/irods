


#include "rodsClient.hpp"
#include "rods.hpp"
#include "reGlobalsExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"




extern "C" {
    /**
     * \fn msiobjput_z3950(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
     *
     * \brief Puts an object into a z3950 server
     *
     * \module msoDrivers_z3950
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
    int msiobjput_z3950(
        msParam_t*  inMSOPath,
        msParam_t*  inCacheFilename,
        msParam_t*  inFileSize,
        ruleExecInfo_t* rei ) {

        char *cacheFilename;
        rodsLong_t dataSize;
        int status;
        int srcFd;
        char *myBuf;
        int bytesRead;


        RE_TEST_MACRO( "    Calling msiobjput_z3950" );

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
        cacheFilename = ( char * ) inCacheFilename->inOutStruct;
        dataSize  = atol( ( char * ) inFileSize->inOutStruct );



        /* Read the cache and Do the upload*/
        srcFd = open( cacheFilename, O_RDONLY, 0 );
        if ( srcFd < 0 ) {
            status = UNIX_FILE_OPEN_ERR - errno;
            printf( "msiputobj_z3950: open error for %s, status = %d\n",
                    cacheFilename, status );
            return status;
        }
        myBuf = ( char * ) malloc( dataSize );
        bytesRead = read( srcFd, ( void * ) myBuf, dataSize );
        close( srcFd );
        myBuf[dataSize - 1] = '\0';
        if ( bytesRead > 0 && bytesRead != dataSize ) {
            printf( "msiputobj_z3950: bytesRead %d != dataSize %lld\n",
                    bytesRead, dataSize );
            free( myBuf );
            return SYS_COPY_LEN_ERR;
        }

        rodsLog( LOG_NOTICE, "MSO_Z3950 file contains: %s\n", myBuf );
        free( myBuf );
        return( 0 );
    }

    // =-=-=-=-=-=-=-
    // plugin factory
    irods::ms_table_entry*  plugin_factory( ) {
        // =-=-=-=-=-=-=-
        // instantiate a new msvc plugin
        irods::ms_table_entry* msvc = new irods::ms_table_entry( 3 );

        // =-=-=-=-=-=-=-
        // wire the implementation to the plugin instance
        msvc->add_operation( "msiobjput_z3950", "msiobjput_z3950" );

        // =-=-=-=-=-=-=-
        // hand it over to the system
        return msvc;

    } // plugin_factory

} // extern "C"




