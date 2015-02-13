/**
 * @file	mailMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "reGlobalsExtern.hpp"
#include "reFuncDefs.hpp"
#include "icatHighLevelRoutines.hpp"

#include "irods_log.hpp"

/**
 * \fn msiSendMail(msParam_t* xtoAddr, msParam_t* xsubjectLine, msParam_t* xbody, ruleExecInfo_t *rei)
 *
 * \brief Sends email
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \note   This microservice sends e-mail using the mail command in the unix system. No attachments are supported. The sender of the e-mail is the unix user-id running the irodsServer.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xtoAddr - a msParam of type STR_MS_T which is an address of the receiver.
 * \param[in] xsubjectLine - a msParam of type STR_MS_T which is a subject of the message.
 * \param[in] xbody - a msParam of type STR_MS_T which is a body of the message.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
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

    if ( reTestFlag > 0 ) {
        if ( reTestFlag == COMMAND_TEST_1 ) {
            fprintf( stdout, "  Sending Email\n     To:%s\n     Subject:%s\n     Body:%s\n",
                     toAddr, subjectLine, body );
        }
        else if ( reTestFlag == HTML_TEST_1 ) {
            fprintf( stdout, "Sending Email\n<UL>\n" );
            fprintf( stdout, "<LI>To: %s\n", toAddr );
            fprintf( stdout, "<LI>subjectLine: %s\n", subjectLine );
            fprintf( stdout, "<LI>Body: %s\n", body );
            fprintf( stdout, "</UL>\n" );
        }
        else if ( reTestFlag == LOG_TEST_1 )
            rodsLog( LOG_NOTICE, "   Calling msiSendMail To:%s Subject %s\n",
                     toAddr, subjectLine );
        if ( reLoopBackFlag > 0 ) {
            return 0;
        }
    }
    char fName[100];
    sprintf( fName, "mailFile%d.ml", getpid() );
    FILE* fd = fopen( fName, "w" );
    if ( fd == NULL ) {
        return FILE_CREATE_ERROR;
    }
#ifdef solaris_platform
    if ( strlen( subjectLine ) > 0 ) {
        fprintf( fd, "Subject:%s\n\n", subjectLine );
    }
#endif
    const char * t1 = body;
    while ( const char * t2 = strstr( t1, "\\n" ) ) {
        fwrite( t1, sizeof( *t1 ), t2 - t1, fd );
        fprintf( fd, "\n" );
        t1 = t2 + 2;
    }
    fprintf( fd, "%s\n", t1 );
    fclose( fd );
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


/**
 * \fn msiSendStdoutAsEmail(msParam_t* xtoAddr, msParam_t* xsubjectLine, ruleExecInfo_t *rei)
 *
 * \brief Sends the current buffer content in rei->ruleExecOut->stdoutBuf.buf as email.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \note   This microservice sends contents of the buffer rei->ruleExecOut->stdoutBuf.buf as email.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xtoAddr - a msParam of type STR_MS_T which is the address of the receiver.
 * \param[in] xsubjectLine - a msParam of type STR_MS_T which is the subject of the message.
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
 * \sa writeLine, writeString
**/
int msiSendStdoutAsEmail( msParam_t* xtoAddr, msParam_t* xsubjectLine, ruleExecInfo_t *rei ) {
    msParam_t *mP;
    if ( ( mP = getMsParamByLabel( rei->msParamArray, "ruleExecOut" ) ) == NULL ) {
        return NO_VALUES_FOUND;
    }
    execCmdOut_t *myExecCmdOut = ( execCmdOut_t* )mP->inOutStruct;
    char tmpVarName[MAX_ACTION_SIZE];
    getNewVarName( tmpVarName, rei->msParamArray );
    addMsParam( rei->msParamArray, tmpVarName,  STR_MS_T, myExecCmdOut->stdoutBuf.buf , NULL );
    mP = getMsParamByLabel( rei->msParamArray, tmpVarName );
    int status;
    if ( NULL != mP ) {
        status = msiSendMail( xtoAddr, xsubjectLine, mP, rei );
    }
    else {
        status = SYS_INTERNAL_NULL_INPUT_ERR;
    }
    rmMsParamByLabel( rei->msParamArray, tmpVarName, 1 );
    return status;

}
