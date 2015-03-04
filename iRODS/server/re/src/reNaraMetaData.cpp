/**
 * @file reNaraMetaData.c
 *
 */

#include "reNaraMetaData.hpp"
#include "apiHeaderAll.hpp"
#include "irods_stacktrace.hpp"
#include "rodsConnect.h"

/**
 * \fn msiExtractNaraMetadata (ruleExecInfo_t *rei)
 *
 * \brief  This microservice extracts NARA style metadata from a local configuration file.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  DICE
 * \date    2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
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
msiExtractNaraMetadata( ruleExecInfo_t *rei ) {
    FILE *fp;
    char str[500];
    char *substring;
    int counter;
    int flag;
    char attr[100];
    char value[500];
    modAVUMetadataInp_t modAVUMetadataInp;
    int status;
    /* specify the location of the metadata file here */
    char metafile[MAX_NAME_LEN];

    snprintf( metafile, MAX_NAME_LEN, "%-s/reConfigs/%-s", getConfigDir(),
              NARA_META_DATA_FILE );

    if ( ( fp = fopen( metafile, "r" ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiExtractNaraMetadata: Cannot open the metadata file %s.", metafile );
        return UNIX_FILE_OPEN_ERR;
    }

    memset( &modAVUMetadataInp, 0, sizeof( modAVUMetadataInp ) );
    modAVUMetadataInp.arg0 = "add";

    while ( !feof( fp ) ) {
        counter = 0;
        flag = 0;
        if ( fgets( str, 500, fp ) ) {
            substring = strtok( str, "|" );
            while ( substring != NULL ) {
                if ( flag == 0 && strcmp( substring, rei->doi->objPath ) == 0 ) {
                    flag = 2;
                }

                if ( counter == 1 ) {
                    if ( strlen( substring ) >= sizeof( attr ) ) {
                        rodsLog( LOG_ERROR,
                                 "attr: [%s] is too long for attr, which may only be %ju characters in length.",
                                 ( uintmax_t )sizeof( attr ) );
                    }
                    snprintf( attr, sizeof( attr ), "%s", substring );
                }
                if ( flag == 2 && counter == 2 ) {
                    if ( strlen( substring ) >= sizeof( value ) ) {
                        rodsLog( LOG_ERROR,
                                 "value: [%s] is too long for value, which may only be %ju characters in length.",
                                 ( uintmax_t )sizeof( value ) );
                    }
                    snprintf( value, sizeof( value ), "%s", substring );
                    /*Call the function to insert metadata here.*/
                    modAVUMetadataInp.arg1 = "-d";
                    modAVUMetadataInp.arg2 = rei->doi->objPath;
                    modAVUMetadataInp.arg3 = attr;
                    modAVUMetadataInp.arg4 = value;
                    modAVUMetadataInp.arg5 = "";
                    status = rsModAVUMetadata( rei->rsComm, &modAVUMetadataInp );
                    if ( status < 0 ) {
                        irods::log( ERROR( status, "rsModAVUMetadata failed." ) );
                    }
                    rodsLog( LOG_DEBUG, "msiExtractNaraMetadata: %s:%s", attr, value );
                }
                substring = strtok( NULL, "|" );
                counter++;
            }
        }
    }
    fclose( fp );
    return 0;
}

