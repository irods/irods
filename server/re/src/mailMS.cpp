/**
 * @file	mailMS.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
//#include "reGlobalsExtern.hpp"
//#include "reFuncDefs.hpp"
#include "icatHighLevelRoutines.hpp"

#include "irods_log.hpp"
#include "irods_re_structs.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>


/**
 * \fn msiSendMail(msParam_t* xtoAddr, msParam_t* xsubjectLine, msParam_t* xbody, ruleExecInfo_t *)
 *
 * \brief Sends email
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note   This microservice sends e-mail using the mail command in the unix system. No attachments are supported. The sender of the e-mail is the unix user-id running the irodsServer.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] xtoAddr - a msParam of type STR_MS_T which is an address of the receiver.
 * \param[in] xsubjectLine - a msParam of type STR_MS_T which is a subject of the message.
 * \param[in] xbody - a msParam of type STR_MS_T which is a body of the message.
 * \param[in,out] - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect An e-mail sent to the specified recipient.
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int msiSendMail( msParam_t* xtoAddr, msParam_t* xsubjectLine, msParam_t* xbody, ruleExecInfo_t* ) {

    const char * toAddr = ( char * ) xtoAddr->inOutStruct;
    const char * subjectLine = xsubjectLine->inOutStruct ? ( char * ) xsubjectLine->inOutStruct : "";
    const char * body = xbody->inOutStruct ? ( char * ) xbody->inOutStruct : "";

    if ( int status = checkStringForEmailAddress( toAddr ) ) {
        rodsLog( LOG_NOTICE, "checkStringForEmailAddress failed for [%s]", toAddr );
        return status;
    }
    if ( int status = checkStringForSystem( subjectLine ) ) {
        rodsLog( LOG_NOTICE, "checkStringForSystem failed for [%s]", subjectLine );
        return status;
    }

    char fName[] = "/tmp/irods_mailFileXXXXXXXXXX";
    int fd = mkstemp( fName );
    if (fd < 0) {
        rodsLog(LOG_ERROR, "msiSendMail: mkstemp() failed [%s] %d - %d", fName, fd, errno);
        return FILE_OPEN_ERR;
    }

    FILE* file = fdopen(fd, "w");
    if ( file == NULL ) {
        rodsLog( LOG_ERROR, "failed to create file [%s] errno %d", fName, errno );
        return FILE_CREATE_ERROR;
    }
#ifdef solaris_platform
    if ( strlen( subjectLine ) > 0 ) {
        fprintf( file, "Subject:%s\n\n", subjectLine );
    }
#endif
    const char * t1 = body;
    while ( const char * t2 = strstr( t1, "\\n" ) ) {
        fwrite( t1, sizeof( *t1 ), t2 - t1, file );
        fprintf( file, "\n" );
        t1 = t2 + 2;
    }
    fprintf( file, "%s\n", t1 );
    fclose( file );
    char * mailStr = ( char* )malloc( strlen( toAddr ) + strlen( subjectLine ) + 100 );
    if ( mailStr == NULL ) {
        return SYS_MALLOC_ERR;
    }

#ifdef solaris_platform
    sprintf( mailStr, "cat %s| mail  '%s'", fName, toAddr );
#else /* tested for linux - not sure how other platforms operate for subject */
    if ( strlen( subjectLine ) > 0 ) {
        sprintf( mailStr, "cat %s| mail -s '%s'  '%s'", fName, subjectLine, toAddr );
    }
    else {
        sprintf( mailStr, "cat %s| mail  '%s'", fName, toAddr );
    }
#endif
    int ret = 0;
    ret = system( mailStr );
    if ( ret ) {
        irods::log( ERROR( ret, "mailStr command returned non-zero status" ) );
    }
    sprintf( mailStr, "rm %s", fName );
    ret = system( mailStr );
    if ( ret ) {
        irods::log( ERROR( ret, "mailStr command returned non-zero status" ) );
    }
    free( mailStr );
    return 0;
}
