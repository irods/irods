/**
 * @file reRDA.c
 *
 * /brief Microservices for the Rule-based Database Access (RDA).
 */


/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.hpp"

#include "rdaHighLevelRoutines.hpp"
#include "dataObjWrite.hpp"

/* For now, uncomment this line to build RDA #define BUILD_RDA 1  */
/* Do same in rdaHighLevelRoutines.c */

/**
 * \fn msiRdaToStdout (msParam_t *inpRdaName, msParam_t *inpSQL,
 *      msParam_t *inpParam1, msParam_t *inpParam2,
 *      msParam_t *inpParam3, msParam_t *inpParam4,
 *      ruleExecInfo_t *rei)
 *
 * \brief This microservice calls new RDA functions to interface
 *    to an arbitrary database (under iRODS access control), getting results
 *    (i.e. from a query) and returning them in stdout.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Wayne Schroeder
 * \date   2008-05-12
 *
 * \note This is being replaced with the new 'Database Resources'
 * feature, see https://www.irods.org/index.php/RDA
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpRdaName - a STR_MS_T, name of the RDA being used
 * \param[in] inpSQL -  a STR_MS_T which is the SQL
 * \param[in] inpParam1 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam2 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam3 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam4 - Optional - STR_MS_T parameters (bind variables) to the SQL.
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
msiRdaToStdout( msParam_t *inpRdaName, msParam_t *inpSQL,
                msParam_t *inpParam1, msParam_t *inpParam2,
                msParam_t *inpParam3, msParam_t *inpParam4,
                ruleExecInfo_t *rei ) {
#if defined(BUILD_RDA)
    rsComm_t *rsComm;
    char *rdaName;
    char *sql;
    char *p1;
    char *p2;
    char *p3;
    char *p4;
    int status;
    char *parms[20];
    int nParm = 0;
    char *outBuf = 0;

    RE_TEST_MACRO( "    Calling msiRdaToStdout" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiRdaToStdout rei or rsComm is NULL" );
        return ( SYS_INTERNAL_NULL_INPUT_ERR );
    }
    rsComm = rei->rsComm;


    rdaName = parseMspForStr( inpRdaName );
    if ( rdaName == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaToStdout: input inpRdaName is NULL" );
        return( USER__NULL_INPUT_ERR );
    }

    sql = parseMspForStr( inpSQL );
    if ( sql == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaToStdout: input inpSQL is NULL" );
        return( USER__NULL_INPUT_ERR );
    }

    p1 = parseMspForStr( inpParam1 );
    p2 = parseMspForStr( inpParam2 );
    p3 = parseMspForStr( inpParam3 );
    p4 = parseMspForStr( inpParam4 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaToStdout: input inpParam1 error. status = %d", rei->status );
        return ( rei->status );
    }

    status = rdaCheckAccess( rdaName, rsComm );
    if ( status ) {
        return( status );
    }

    status = rdaOpen( rdaName );
    if ( status ) {
        return( status );
    }

    if ( p1 != NULL ) {
        parms[nParm++] = p1;
    }
    if ( p2 != NULL ) {
        parms[nParm++] = p2;
    }
    if ( p3 != NULL ) {
        parms[nParm++] = p3;
    }
    if ( p4 != NULL ) {
        parms[nParm++] = p4;
    }


    status = rdaSqlWithResults( sql, parms, nParm, &outBuf );
    if ( status ) {
        return( status );
    }

    if ( outBuf != NULL ) {
        _writeString( "stdout", outBuf, rei );
        free( outBuf );
    }

    return ( 0 );
#else
    return( RDA_NOT_COMPILED_IN );
#endif

}

/**
 * \fn msiRdaToDataObj (msParam_t *inpRdaName, msParam_t *inpSQL,
 *      msParam_t *inpParam1, msParam_t *inpParam2,
 *      msParam_t *inpParam3, msParam_t *inpParam4,
 *      msParam_t *inpOutObj, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls new RDA functions to interface
 *    to an arbitrary database (under iRODS access control), getting results
 *    (i.e. from a query) and writing them to an open DataObject.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Wayne Schroeder
 * \date   2008-05-15
 *
 * \note This is being replaced with the new 'Database Resources'
 * feature, see https://www.irods.org/index.php/RDA
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpRdaName - a STR_MS_T, name of the RDA being used
 * \param[in] inpSQL - a STR_MS_T which is the SQL
 * \param[in] inpParam1 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam2 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam3 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam4 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpOutObj - a INT_MS_T, open descriptor to write results to.
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
msiRdaToDataObj( msParam_t *inpRdaName, msParam_t *inpSQL,
                 msParam_t *inpParam1, msParam_t *inpParam2,
                 msParam_t *inpParam3, msParam_t *inpParam4,
                 msParam_t *inpOutObj, ruleExecInfo_t *rei ) {
#if defined(BUILD_RDA)
    rsComm_t *rsComm = 0; // JMC cppcheck - uninit var
    char *rdaName;
    char *sql;
    char *p1;
    char *p2;
    char *p3;
    char *p4;
    int status;
    char *parms[20];
    int nParm = 0;
    char *outBuf = 0;
    int myInt;
    dataObjWriteInp_t myDataObjWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;

    RE_TEST_MACRO( "    Calling msiRdaToDataObj" )

    if ( inpOutObj == NULL ) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaToDataObje: input inpOutBuf is NULL" );
        return ( rei->status );
    }
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiRdaToDataObj rei or rsComm is NULL" );
        return ( SYS_INTERNAL_NULL_INPUT_ERR );
    }
    rsComm = rei->rsComm;

    rdaName = parseMspForStr( inpRdaName );
    if ( rdaName == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaToStdout: input inpRdaName is NULL" );
        return( USER__NULL_INPUT_ERR );
    }

    sql = parseMspForStr( inpSQL );
    if ( sql == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRDAToDataObj: input inpParam1 is NULL" );
        return( USER__NULL_INPUT_ERR );
    }

    p1 = parseMspForStr( inpParam1 );
    p2 = parseMspForStr( inpParam2 );
    p3 = parseMspForStr( inpParam3 );
    p4 = parseMspForStr( inpParam4 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaToDataObj: input inpParam1 error. status = %d", rei->status );
        return ( rei->status );
    }

    status = rdaCheckAccess( rdaName, rsComm );
    if ( status ) {
        return( status );
    }

    status = rdaOpen( rdaName );
    if ( status ) {
        return( status );
    }

    if ( p1 != NULL ) {
        parms[nParm++] = p1;
    }
    if ( p2 != NULL ) {
        parms[nParm++] = p2;
    }
    if ( p3 != NULL ) {
        parms[nParm++] = p3;
    }
    if ( p4 != NULL ) {
        parms[nParm++] = p4;
    }

    myInt = parseMspForPosInt( inpOutObj );
    if ( myInt >= 0 ) {
        myDataObjWriteInp.l1descInx = myInt;
    }
    else {
        rei->status = myInt;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaToDataObj: parseMspForPosInt error for inpOutObj" );
        return ( rei->status );
    }

    status = rdaSqlWithResults( sql, parms, nParm, &outBuf );
    if ( status ) {
        return( status );
    }

    if ( outBuf != NULL ) {
        int len;
        len = strlen( outBuf );
        dataObjWriteInpBBuf.buf = outBuf;
        myDataObjWriteInp.len = len;
        status = rsDataObjWrite( rsComm, &myDataObjWriteInp,
                                 &dataObjWriteInpBBuf );
        free( outBuf );
    }
    return ( status );
#else
    return( RDA_NOT_COMPILED_IN );
#endif

}

/**
 * \fn msiRdaNoResults(msParam_t *inpRdaName, msParam_t *inpSQL,
 *      msParam_t *inpParam1, msParam_t *inpParam2,
 *      msParam_t *inpParam3, msParam_t *inpParam4,
 *      ruleExecInfo_t *rei)
 *
 * \brief This microservice calls new RDA functions to interface
 *    to an arbitrary database (under iRODS access control), performing
 *    a SQL that does not return results.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Wayne Schroeder
 * \date   2008-06-03
 *
 * \note This is being replaced with the new 'Database Resources'
 * feature, see https://www.irods.org/index.php/RDA
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpRdaName - a STR_MS_T, name of the RDA being used
 * \param[in] inpSQL - a STR_MS_T which is the SQL
 * \param[in] inpParam1 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam2 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam3 - Optional - STR_MS_T parameters (bind variables) to the SQL.
 * \param[in] inpParam4 - Optional - STR_MS_T parameters (bind variables) to the SQL.
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
msiRdaNoResults( msParam_t *inpRdaName, msParam_t *inpSQL,
                 msParam_t *inpParam1, msParam_t *inpParam2,
                 msParam_t *inpParam3, msParam_t *inpParam4,
                 ruleExecInfo_t *rei ) {
#if defined(BUILD_RDA)
    rsComm_t *rsComm;
    char *rdaName;
    char *sql;
    char *p1;
    char *p2;
    char *p3;
    char *p4;
    int status;
    char *parms[20];
    int nParm = 0;

    RE_TEST_MACRO( "    Calling msiRdaNoResults" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiRdaNoResults rei or rsComm is NULL" );
        return ( SYS_INTERNAL_NULL_INPUT_ERR );
    }
    rsComm = rei->rsComm;


    rdaName = parseMspForStr( inpRdaName );
    if ( rdaName == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaNoResults: input inpRdaName is NULL" );
        return( USER__NULL_INPUT_ERR );
    }

    sql = parseMspForStr( inpSQL );
    if ( sql == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaNoResults: input inpSQL is NULL" );
        return( USER__NULL_INPUT_ERR );
    }

    p1 = parseMspForStr( inpParam1 );
    p2 = parseMspForStr( inpParam2 );
    p3 = parseMspForStr( inpParam3 );
    p4 = parseMspForStr( inpParam4 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRdaNoResults: input inpParam1 error. status = %d", rei->status );
        return ( rei->status );
    }

    status = rdaCheckAccess( rdaName, rsComm );
    if ( status ) {
        return( status );
    }

    status = rdaOpen( rdaName );
    if ( status ) {
        return( status );
    }

    if ( p1 != NULL ) {
        parms[nParm++] = p1;
    }
    if ( p2 != NULL ) {
        parms[nParm++] = p2;
    }
    if ( p3 != NULL ) {
        parms[nParm++] = p3;
    }
    if ( p4 != NULL ) {
        parms[nParm++] = p4;
    }

    status = rdaSqlNoResults( sql, parms, nParm );
    if ( status ) {
        return( status );
    }

    return ( 0 );
#else
    return( RDA_NOT_COMPILED_IN );
#endif

}

/**
 * \fn msiRdaCommit(ruleExecInfo_t *rei)
 *
 * \brief This microservice calls new RDA functions to interface
 *    to an arbitrary database (under iRODS access control), performing
 *    a commit operation.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Wayne Schroeder
 * \date   2008-06-03
 *
 * \note This is being replaced with the new 'Database Resources'
 * feature, see https://www.irods.org/index.php/RDA
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
msiRdaCommit( ruleExecInfo_t *rei ) {
#if defined(BUILD_RDA)
    rsComm_t *rsComm;
    int status;

    RE_TEST_MACRO( "    Calling msiRdaCommit" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiRdaCommit rei or rsComm is NULL" );
        return ( SYS_INTERNAL_NULL_INPUT_ERR );
    }
    rsComm = rei->rsComm;

    status = rdaCommit( rsComm );
    return( status );
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}

/**
 * \fn msiRdaRollback(ruleExecInfo_t *rei)
 *
 * \brief This microservice calls new RDA functions to interface
 *    to an arbitrary database (under iRODS access control), performing
 *    a rollback operation.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Wayne Schroeder
 * \date   2008-08-05
 *
 * \note This is being replaced with the new 'Database Resources'
 * feature, see https://www.irods.org/index.php/RDA
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
msiRdaRollback( ruleExecInfo_t *rei ) {
#if defined(BUILD_RDA)
    rsComm_t *rsComm;
    int status;

    RE_TEST_MACRO( "    Calling msiRdaRollback" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiRdaRollback rei or rsComm is NULL" );
        return ( SYS_INTERNAL_NULL_INPUT_ERR );
    }
    rsComm = rei->rsComm;

    status = rdaRollback( rsComm );
    return( status );
#else
    return( RDA_NOT_COMPILED_IN );
#endif
}
