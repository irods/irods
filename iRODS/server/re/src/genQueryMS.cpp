/**
 * @file	genQueryMS.c
 *
 */



/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "reGlobalsExtern.hpp"
#include "reFuncDefs.hpp"
#include "genQuery.hpp"
#include "reHelpers1.hpp"
#include "rcMisc.hpp"

int _makeQuery( char *sel, char *cond, char **sql );

/**
 * \fn msiExecStrCondQueryWithOptionsNew(msParam_t* queryParam,
 *        msParam_t* zeroResultsIsOK,
 *        msParam_t* maxReturnedRowsParam,
 *        msParam_t* genQueryOutParam,
 *        ruleExecInfo_t *rei)
 *
 * \brief   This function takes a given condition string and options, creates an iCAT query, executes it and returns the values
 * 			This function returns an empty result set if "zeroOK" instead of a string "emptySet".
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Romain Guinot
 * \date    2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] queryParam - a msParam of type GenQueryInp_MS_T
 * \param[in] zeroResultsIsOK - Optional - a msParam of type STR_MS_T - must equal "zeroOK"
 * \param[in] maxReturnedRowsParam - Optional - a msParam of type STR_MS_T - as integer
 * \param[out] genQueryOutParam - a msParam of type GenQueryOut_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified  none
 * \iCatAttrDependence  none
 * \iCatAttrModified  none
 * \sideeffect  none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  msiExecStrCondQuery
**/
int msiExecStrCondQueryWithOptionsNew( msParam_t* queryParam,
                                       msParam_t* zeroResultsIsOK,
                                       msParam_t* maxReturnedRowsParam,
                                       msParam_t* genQueryOutParam,
                                       ruleExecInfo_t *rei ) {
    genQueryInp_t genQueryInp;
    int i;
    genQueryOut_t *genQueryOut = NULL;
    char *query;
    char *maxReturnedRowsStr;
    int maxReturnedRows;

    query = ( char * ) malloc( strlen( ( const char* )queryParam->inOutStruct ) + 10 + MAX_COND_LEN * 8 );
    strcpy( query, ( const char* ) queryParam->inOutStruct );

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    i = fillGenQueryInpFromStrCond( query, &genQueryInp );
    free( query );
    if ( i < 0 ) {
        return i;
    }

    if ( maxReturnedRowsParam != NULL ) {
        maxReturnedRowsStr = ( char * ) maxReturnedRowsParam->inOutStruct;
        if ( strcmp( maxReturnedRowsStr, "NULL" ) != 0 ) {
            maxReturnedRows = atoi( maxReturnedRowsStr );
            genQueryInp.maxRows = maxReturnedRows;
        }
        else {
            genQueryInp.maxRows = MAX_SQL_ROWS;
        }
    }
    else {
        genQueryInp.maxRows = MAX_SQL_ROWS;
    }

    genQueryInp.continueInx = 0;

    i = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    if ( i == CAT_NO_ROWS_FOUND && zeroResultsIsOK != NULL &&
            strcmp( ( const char* )zeroResultsIsOK->inOutStruct, "zeroOK" ) == 0 ) {
        genQueryOutParam->type = strdup( GenQueryOut_MS_T );
        genQueryOut = ( genQueryOut_t * ) malloc( sizeof( genQueryOut_t ) );
        memset( genQueryOut, 0, sizeof( genQueryOut_t ) );
        genQueryOutParam->inOutStruct = genQueryOut;

        return 0;
    }
    else if ( i < 0 ) {
        return i;
    }
    genQueryOutParam->type = strdup( GenQueryOut_MS_T );
    genQueryOutParam->inOutStruct = genQueryOut;
    return 0;
}

/**
 * \fn msiExecStrCondQueryWithOptions(msParam_t* queryParam,
 *        msParam_t* zeroResultsIsOK,
 *        msParam_t* maxReturnedRowsParam,
 *        msParam_t* genQueryOutParam,
 *        ruleExecInfo_t *rei)
 *
 * \brief   This function takes a given condition string and options, creates an iCAT query, executes it and returns the values
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Romain Guinot
 * \date    2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] queryParam - a msParam of type GenQueryInp_MS_T
 * \param[in] zeroResultsIsOK - Optional - a msParam of type STR_MS_T - must equal "zeroOK"
 * \param[in] maxReturnedRowsParam - Optional - a msParam of type STR_MS_T - as integer
 * \param[out] genQueryOutParam - a msParam of type GenQueryOut_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified  none
 * \iCatAttrDependence  none
 * \iCatAttrModified  none
 * \sideeffect  none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa  msiExecStrCondQuery
**/
int msiExecStrCondQueryWithOptions( msParam_t* queryParam,
                                    msParam_t* zeroResultsIsOK,
                                    msParam_t* maxReturnedRowsParam,
                                    msParam_t* genQueryOutParam,
                                    ruleExecInfo_t *rei ) {
    genQueryInp_t genQueryInp;
    int status;
    genQueryOut_t *genQueryOut = NULL;
    char *query;
    char *maxReturnedRowsStr;
    int maxReturnedRows;

    query = ( char * ) malloc( strlen( ( const char* )queryParam->inOutStruct ) + 10 + MAX_COND_LEN * 8 );
    strcpy( query, ( const char* ) queryParam->inOutStruct );

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    status = fillGenQueryInpFromStrCond( query, &genQueryInp );
    free( query );
    if ( status < 0 ) {
        return status;
    }

    if ( maxReturnedRowsParam != NULL ) {
        maxReturnedRowsStr = ( char * ) maxReturnedRowsParam->inOutStruct;
        if ( strcmp( maxReturnedRowsStr, "NULL" ) != 0 ) {
            maxReturnedRows = atoi( maxReturnedRowsStr );
            genQueryInp.maxRows = maxReturnedRows;
        }
        else {
            genQueryInp.maxRows = MAX_SQL_ROWS;
        }
    }
    else {
        genQueryInp.maxRows = MAX_SQL_ROWS;
    }

    genQueryInp.continueInx = 0;

    status = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND &&
            zeroResultsIsOK != NULL &&
            strcmp( ( const char* )zeroResultsIsOK->inOutStruct, "zeroOK" ) == 0 ) {
        genQueryOutParam->type = strdup( STR_MS_T );
        fillStrInMsParam( genQueryOutParam, "emptySet" );
        return 0;
    }
    else if ( status < 0 ) {
        return status;
    }

    genQueryOutParam->type = strdup( GenQueryOut_MS_T );
    genQueryOutParam->inOutStruct = genQueryOut;
    return 0;
}

/**
 * \fn msiExecStrCondQuery(msParam_t* queryParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei)
 *
 * \brief   This function takes a given condition string, creates an iCAT query, executes it and returns the values
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] queryParam - a msParam of type GenQueryInp_MS_T
 * \param[out] genQueryOutParam - a msParam of type GenQueryOut_MS_T
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
int msiExecStrCondQuery( msParam_t* queryParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei ) {
    genQueryInp_t genQueryInp;
    int i;
    genQueryOut_t *genQueryOut = NULL;
    char *query;

    query = ( char * ) malloc( strlen( ( const char* )queryParam->inOutStruct ) + 10 + MAX_COND_LEN * 8 );

    strcpy( query, ( const char* )queryParam->inOutStruct );

    /**** Jun 27, 2007
    if (queryParam->inOutStruct == NULL) {
      query = (char *) malloc(strlen(queryParam->label) + MAX_COND_LEN);
      strcpy(query , queryParam->label);
    }
    else {
      query = (char *) malloc(strlen(queryParam->inOutStruct) + MAX_COND_LEN);
      strcpy(query , queryParam->inOutStruct);
    }
    i  = replaceVariablesAndMsParams("",query, rei->msParamArray, rei);
    if (i < 0)
      return i;
    ***/
    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );
    i = fillGenQueryInpFromStrCond( query, &genQueryInp );
    free( query );
    if ( i < 0 ) {
        return i;
    }
    genQueryInp.maxRows = MAX_SQL_ROWS;
    genQueryInp.continueInx = 0;

    i = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    if ( i < 0 ) {
        if ( i == CAT_NO_ROWS_FOUND ) {
            genQueryOutParam->type = strdup( GenQueryOut_MS_T );
            genQueryOut = ( genQueryOut_t * ) malloc( sizeof( genQueryOut_t ) );
            memset( genQueryOut, 0, sizeof( genQueryOut_t ) );
            genQueryOutParam->inOutStruct = genQueryOut;
            return 0;
        }
        else {
            return i;
        }
    }
    genQueryOutParam->type = strdup( GenQueryOut_MS_T );
    genQueryOutParam->inOutStruct = genQueryOut;
    return 0;
}

/**
 * \fn msiExecGenQuery(msParam_t* genQueryInParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei)
 *
 * \brief   This function executes a given general query structure and returns results
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008
 *
 * \note Takes a SQL-like iRODS query (no FROM clause) and returns a table structure. Use #msiGetMoreRows to get all rows.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] genQueryInParam - a msParam of type GenQueryInp_MS_T
 * \param[out] genQueryOutParam - a msParam of type GenQueryOut_MS_T
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
 * \sa msiGetMoreRows and msiExecStrCondQuery
**/
int msiExecGenQuery( msParam_t* genQueryInParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei ) {
    genQueryInp_t *genQueryInp;
    int i;
    genQueryOut_t *genQueryOut = NULL;


    genQueryInp = ( genQueryInp_t* )genQueryInParam->inOutStruct;

    i = rsGenQuery( rei->rsComm, genQueryInp, &genQueryOut );
    if ( i < 0 ) {
        if ( i == CAT_NO_ROWS_FOUND ) {
            genQueryOutParam->type = strdup( GenQueryOut_MS_T );
            genQueryOut = ( genQueryOut_t * ) malloc( sizeof( genQueryOut_t ) );
            memset( genQueryOut, 0, sizeof( genQueryOut_t ) );
            genQueryOutParam->inOutStruct = genQueryOut;
            return 0;
        }
        else {
            return i;
        }
    }
    genQueryOutParam->type = strdup( GenQueryOut_MS_T );
    genQueryOutParam->inOutStruct = genQueryOut;
    return 0;
}



/**
 * \fn msiGetContInxFromGenQueryOut(msParam_t *genQueryOutParam, msParam_t *continueInx, ruleExecInfo_t *rei)
 *
 * \brief This microservice gets the continue index value from genQueryOut generated by msiExecGenQuery
 *
 * \module core
 *
 * \since 2.2
 *
 * \author  Arcot Rajasekar
 * \date    2009-10
 *
 * \note The resulting continueInx can be used to determine whether there are remaining rows to retrieve from the generated query.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] genQueryOutParam - Required - of type GenQueryOut_MS_T.
 * \param[out] continueInx - a INT_MS_T containing the new continuation index (after the query).
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
 * \sa msiExecGenQuery, msiGetMoreRows
**/
int
msiGetContInxFromGenQueryOut( msParam_t* genQueryOutParam, msParam_t* continueInx, ruleExecInfo_t *rei ) {
    genQueryOut_t *genQueryOut;

    RE_TEST_MACRO( "    Calling msiGetContInxFromGenQueryOut" )
    /* check for non null parameters */
    if ( !genQueryOutParam ) {
        rodsLog( LOG_ERROR, "msiGetContInxFromGenQueryOut: Missing parameter(s)" );
        return USER__NULL_INPUT_ERR;
    }
    /* check for proper input types */
    if ( strcmp( genQueryOutParam->type, GenQueryOut_MS_T ) ) {
        rodsLog( LOG_ERROR,
                 "msiGetContInxFromGenQueryOut: genQueryOutParam type is %s, should be GenQueryOut_MS_T",
                 genQueryOutParam->type );
        return USER_PARAM_TYPE_ERR;
    }

    genQueryOut = ( genQueryOut_t* )genQueryOutParam->inOutStruct;
    fillIntInMsParam( continueInx, genQueryOut->continueInx );
    return 0;
}


int
_makeQuery( char *sel, char *cond, char **sql ) {
    *sql = ( char * ) malloc( strlen( sel ) + strlen( cond ) + 20 );
    if ( strlen( cond ) >  0 ) {
        sprintf( *sql, "SELECT %s WHERE %s", sel, cond );
    }
    else {
        sprintf( *sql, "SELECT %s ", sel );
    }
    return 0;
}

/**
 * \fn msiMakeQuery(msParam_t* selectListParam, msParam_t* conditionsParam, msParam_t* queryOutParam, ruleExecInfo_t *rei)
 *
 * \brief Creates sql query from parameter list and conditions.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] selectListParam - a STR_MS_T containing the parameters.
 * \param[in] conditionsParam - a STR_MS_T containing the conditions.
 * \param[out] queryOutParam - a STR_MS_T containing the parameters and conditions as sql.
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
 * \sa msiGetMoreRows and msiExecStrCondQuery
**/
int
msiMakeQuery( msParam_t* selectListParam, msParam_t* conditionsParam,
              msParam_t* queryOutParam, ruleExecInfo_t* ) {
    char *sql, *sel, *cond;
    int i;
    sel = ( char * ) selectListParam->inOutStruct;
    cond = ( char * ) conditionsParam->inOutStruct;
    i = _makeQuery( sel, cond, &sql );
    queryOutParam->type = strdup( STR_MS_T );
    queryOutParam->inOutStruct = sql;
    return i;
}



/**
 * \fn msiGetMoreRows(msParam_t *genQueryInp_msp, msParam_t *genQueryOut_msp, msParam_t *continueInx, ruleExecInfo_t *rei)
 *
 * \brief This microservice continues an unfinished query.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-09-18
 *
 * \note This microservice gets the next batch of rows for an open iCAT query. Likely to follow #msiMakeGenQuery and #msiExecGenQuery.
 *
 * \usage None
 *
 * \param[in] genQueryInp_msp - Required - a GenQueryInp_MS_T containing the query parameters and conditions.
 * \param[in] genQueryOut_msp - Required - a GenQueryOut_MS_T to write results to. If its continuation index is 0 the query will be closed.
 * \param[out] continueInx - a INT_MS_T containing the new continuation index (after the query).
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
msiGetMoreRows( msParam_t *genQueryInp_msp, msParam_t *genQueryOut_msp, msParam_t *continueInx, ruleExecInfo_t *rei ) {
    genQueryInp_t *genQueryInp;
    genQueryOut_t *genQueryOut;


    RE_TEST_MACRO( "    Calling msiGetMoreRows" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiGetMoreRows: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /* check for non null parameters */
    if ( !genQueryInp_msp || !genQueryOut_msp ) {
        rodsLog( LOG_ERROR, "msiGetMoreRows: Missing parameter(s)" );
        return USER__NULL_INPUT_ERR;
    }


    /* check for proper input types */
    if ( strcmp( genQueryOut_msp->type, GenQueryOut_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiGetMoreRows: genQueryOut_msp type is %s, should be GenQueryOut_MS_T", genQueryOut_msp->type );
        return USER_PARAM_TYPE_ERR;
    }

    if ( strcmp( genQueryInp_msp->type, GenQueryInp_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiGetMoreRows: query_msp type is %s, should be GenQueryInp_MS_T", genQueryInp_msp->type );
        return USER_PARAM_TYPE_ERR;
    }


    /* retrieve genQueryXXX data structures */
    genQueryOut = ( genQueryOut_t* )genQueryOut_msp->inOutStruct;
    genQueryInp = ( genQueryInp_t* )genQueryInp_msp->inOutStruct;


    /* match continuation indexes */
    genQueryInp->continueInx = genQueryOut->continueInx;

    if ( genQueryInp->continueInx > 0 ) {
        /* get the next batch */
        genQueryInp->maxRows = MAX_SQL_ROWS;
    }
    else {
        /* close query */
        genQueryInp->maxRows = -1;
    }


    /* free memory allocated for previous results */
    freeGenQueryOut( &genQueryOut );


    /* query */
    rei->status = rsGenQuery( rei->rsComm, genQueryInp, &genQueryOut );


    if ( rei->status == 0 ) {
        /* return query results */
        genQueryOut_msp->inOutStruct = genQueryOut;

        /* return continuation index separately in case it is needed in conditional expressions */
        resetMsParam( continueInx );
        fillIntInMsParam( continueInx, genQueryOut->continueInx );
    }

    return rei->status;
}



/**
 * \fn msiCloseGenQuery(msParam_t *genQueryInp_msp, msParam_t *genQueryOut_msp, ruleExecInfo_t *rei)
 *
 * \brief This microservice closes an unfinished query. This is based on the code from #msiGetMoreRows.
 *
 * \module core
 *
 * \since 3.1
 *
 * \author  Hao Xu, Antoine de Torcy (msiGetMoreRows)
 * \date    2012-01-12
 *
 * \usage None
 *
 * \param[in] genQueryInp_msp - Required - a GenQueryInp_MS_T containing the query parameters and conditions.
 * \param[in] genQueryOut_msp - Required - a GenQueryOut_MS_T to write results to. If its continuation index is 0 the query will be closed.
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
msiCloseGenQuery( msParam_t *genQueryInp_msp, msParam_t *genQueryOut_msp, ruleExecInfo_t *rei ) {

    genQueryInp_t *genQueryInp;
    genQueryOut_t *genQueryOut;

    RE_TEST_MACRO( "    Calling msiCloseGenQuery" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiCloseGenQuery: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* check for non null parameters */
    if ( genQueryInp_msp == NULL || genQueryOut_msp == NULL ) {
        rodsLog( LOG_ERROR, "msiCloseGenQuery: Missing parameter(s)" );
        return USER__NULL_INPUT_ERR;
    }

    /* no double close */
    if ( genQueryOut_msp->type == NULL ) {
        return 0;
    }

    /* check for proper input types */
    if ( strcmp( genQueryOut_msp->type, GenQueryOut_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiCloseGenQuery: genQueryOut_msp type is %s, should be GenQueryOut_MS_T", genQueryOut_msp->type );
        return USER_PARAM_TYPE_ERR;
    }

    if ( strcmp( genQueryInp_msp->type, GenQueryInp_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiCloseGenQuery: query_msp type is %s, should be GenQueryInp_MS_T", genQueryInp_msp->type );
        return USER_PARAM_TYPE_ERR;
    }

    /* retrieve genQueryXXX data structures */
    genQueryOut = ( genQueryOut_t* )genQueryOut_msp->inOutStruct;
    genQueryInp = ( genQueryInp_t* )genQueryInp_msp->inOutStruct;

    /* set contiuation index so icat know which statement to free */
    genQueryInp->continueInx = genQueryOut->continueInx;
    genQueryInp->maxRows = -1;

    /* free memory allocated for previous results */
    freeGenQueryOut( &genQueryOut );

    if ( genQueryInp->continueInx == 0 ) {
        /* query already closed */
        rei->status = 0;
        return rei->status;
    }

    /* close query */
    rei->status = rsGenQuery( rei->rsComm, genQueryInp, &genQueryOut );

    /* free memory allocated */
    freeGenQueryOut( &genQueryOut );

    if ( rei->status == 0 ) {
        /* clear output parameter */
        genQueryOut_msp->type = NULL;
        genQueryOut_msp->inOutStruct = NULL;
    }

    return rei->status;
}



/**
 * \fn msiMakeGenQuery(msParam_t* selectListStr, msParam_t* condStr, msParam_t* genQueryInpParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice sets up a GenQueryInp_MS_T from a list of parameters and conditions
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-09-19
 *
 * \note This microservice sets up a genQueryInp_t data structure needed by calls to rsGenQuery().
 *    To be used before #msiExecGenQuery and #msiGetMoreRows.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] selectListStr - Required - a STR_MS_T containing the parameters.
 * \param[in] condStr - Required - a STR_MS_T containing the conditions
 * \param[out] genQueryInpParam - a GenQueryInp_MS_T containing the parameters and conditions.
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
msiMakeGenQuery( msParam_t* selectListStr, msParam_t* condStr, msParam_t* genQueryInpParam, ruleExecInfo_t *rei ) {
    char *sel, *cond, *rawQuery, *query;


    RE_TEST_MACRO( "    Calling msiMakeGenQuery" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiMakeGenQuery: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /* parse selectListStr */
    if ( ( sel = parseMspForStr( selectListStr ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiMakeGenQuery: input selectListStr is NULL." );
        return USER__NULL_INPUT_ERR;
    }


    /* parse condStr */
    if ( ( cond = parseMspForStr( condStr ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiMakeGenQuery: input condStr is NULL." );
        return USER__NULL_INPUT_ERR;
    }


    /* The code below is partly taken from msiMakeQuery and msiExecStrCondQuery. There may be a better way to do this. */

    /* Generate raw SQL query string */
    rei->status = _makeQuery( sel, cond, &rawQuery );

    /* allocate more memory for query string with expanded variable names */
    query = ( char * )malloc( strlen( rawQuery ) + 10 + MAX_COND_LEN * 8 );
    strcpy( query, rawQuery );

    /* allocate memory for genQueryInp */
    genQueryInp_t * genQueryInp = ( genQueryInp_t* )malloc( sizeof( genQueryInp_t ) );
    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    /* set up GenQueryInp */
    genQueryInp->maxRows = MAX_SQL_ROWS;
    genQueryInp->continueInx = 0;
    rei->status = fillGenQueryInpFromStrCond( query, genQueryInp );
    if ( rei->status < 0 ) {
        rodsLog( LOG_ERROR, "msiMakeGenQuery: fillGenQueryInpFromStrCond failed." );
        freeGenQueryInp( &genQueryInp );
        free( rawQuery ); // cppcheck - Memory leak: rawQuery
        free( query );
        return rei->status;
    }


    /* return genQueryInp through GenQueryInpParam */
    genQueryInpParam->type = strdup( GenQueryInp_MS_T );
    genQueryInpParam->inOutStruct = genQueryInp;


    /* cleanup */
    free( rawQuery );
    free( query );

    return rei->status;
}


/**
 * \fn msiPrintGenQueryInp( msParam_t *where, msParam_t* genQueryInpParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice prints the given GenQueryInp_MS_T to the given target buffer
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] where - Required - a STR_MS_T containing the parameters.
 * \param[in] genQueryInpParam - Required - a GenQueryInp_MS_T containing the parameters and conditions.
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
 * \sa writeString
**/
int
msiPrintGenQueryInp( msParam_t *where, msParam_t* genQueryInpParam, ruleExecInfo_t *rei ) {
    genQueryInp_t *genQueryInp;
    int i, j;
    char *writeId;
    char writeStr[MAX_NAME_LEN * 2];
    int len;
    int *ip1, *ip2;
    char *cp;
    char **cpp;

    RE_TEST_MACRO( "    Calling msiPrintGenQueryInp" );

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiPrintGenQueryInp: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    if ( !where ) {
        rodsLog( LOG_ERROR, "msiPrintGenQueryInp: No destination provided for writing." );
        return USER__NULL_INPUT_ERR;
    }
    /* where are we writing to? */
    if ( where->inOutStruct != NULL ) {
        writeId = ( char* )where->inOutStruct;
    }
    else {
        writeId = where->label;
    }

    /*  genQueryInp = (genQueryInp_t *)  strtol((char *)genQueryInpParam->inOutStruct,
        (char **) NULL,0); */

    genQueryInp = ( genQueryInp_t * )  genQueryInpParam->inOutStruct;


    /* print each selection pair to writeStr */
    len = genQueryInp->selectInp.len;
    ip1 = genQueryInp->selectInp.inx;
    ip2 = genQueryInp->selectInp.value;
    for ( i = 0; i < len; i++ ) {
        sprintf( writeStr, "Selected Column %d With Option %d\n", *ip1, *ip2 );
        j = _writeString( writeId, writeStr, rei );
        if ( j < 0 ) {
            return j;
        }
        ip1++;
        ip2++;
    }

    len = genQueryInp->sqlCondInp.len;
    ip1 = genQueryInp->sqlCondInp.inx;
    cpp = genQueryInp->sqlCondInp.value;
    cp = *cpp;
    for ( i = 0; i < len; i++ ) {
        sprintf( writeStr, "Condition Column %d %s\n", *ip1, cp );
        j = _writeString( writeId, writeStr, rei );
        if ( j < 0 ) {
            return j;
        }
        ip1++;
        cpp++;
        cp = *cpp;
    }
    return 0;
}



/**
 * \fn msiAddSelectFieldToGenQuery(msParam_t *select, msParam_t *function, msParam_t *queryInput, ruleExecInfo_t *rei)
 *
 * \brief  Sets a select field in a genQueryInp_t
 *
 * \module core
 *
 *
 * \author  Antoine de Torcy
 * \date    2009-11-28
 *
 * \note This microservice sets a select field in a genQueryInp_t, from two parameters.
 *       One is an iCAT attribute index given without its 'COL_' prefix.
 *       The second one is the optional SQL operator.
 *       A new genQueryInp_t is created if queryInput is NULL.
 *       Followed with #msiExecGenQuery, #msiAddSelectFieldToGenQuery allows to take the
 *       results of other microservices to build and execute queries within a rule.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] select - Required - A STR_MS_T with the select field.
 * \param[in] function - Optional - A STR_MS_T with the function. Valid values are [MIN|MAX|SUM|AVG|COUNT]
 * \param[in,out] queryInput - Optional - A GenQueryInp_MS_T.
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
msiAddSelectFieldToGenQuery( msParam_t *select, msParam_t *function, msParam_t *queryInput, ruleExecInfo_t *rei ) {
    char *column_str;
    int column_inx, function_inx;
    genQueryInp_t *genQueryInp;

    /*************************************  INIT **********************************/

    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiAddSelectFieldToGenQuery" )

    /* Sanity checks */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiAddSelectFieldToGenQuery: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /********************************** PARAM PARSING  *********************************/

    /* Parse select */
    if ( ( column_str = parseMspForStr( select ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiAddSelectFieldToGenQuery: input select is NULL." );
        return USER__NULL_INPUT_ERR;
    }

    /* Parse function and convert to index directly, getSelVal() returns 1 if string is NULL or empty. */
    function_inx = getSelVal( parseMspForStr( function ) );

    /* Check for proper parameter type for queryInput */
    if ( queryInput->type && strcmp( queryInput->type, GenQueryInp_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiAddSelectfieldToGenQuery: queryInput is not of type GenQueryInp_MS_T." );
        return USER_PARAM_TYPE_ERR;
    }

    /* Parse queryInput. Create new structure if empty. */
    if ( !queryInput->inOutStruct ) {
        /* Set content */
        genQueryInp = ( genQueryInp_t* )malloc( sizeof( genQueryInp_t ) );
        memset( genQueryInp, 0, sizeof( genQueryInp_t ) );
        genQueryInp->maxRows = MAX_SQL_ROWS;
        queryInput->inOutStruct = ( void* )genQueryInp;

        /* Set type */
        if ( !queryInput->type ) {
            queryInput->type = strdup( GenQueryInp_MS_T );
        }
    }
    else {
        genQueryInp = ( genQueryInp_t* )queryInput->inOutStruct;
    }


    /***************************** ADD INDEXES TO QUERY INPUT  *****************************/

    /* Get column index */
    column_inx = getAttrIdFromAttrName( column_str );

    /* Error? */
    if ( column_inx < 0 ) {
        rodsLog( LOG_ERROR, "msiAddSelectfieldToGenQuery: Unable to get valid ICAT column index." );
        return column_inx;
    }

    /* Add column and function to genQueryInput */
    addInxIval( &genQueryInp->selectInp, column_inx, function_inx );


    /*********************************** DONE *********************************************/
    return 0;
}


/**
 * \fn msiAddConditionToGenQuery(msParam_t *attribute, msParam_t *opr, msParam_t *value, msParam_t *queryInput, ruleExecInfo_t *rei)
 *
 * \brief Adds a condition to a genQueryInp_t
 *
 * \module core
 *
 * \author  Antoine de Torcy
 * \date    2009-12-07
 *
 * \note This microservice adds a condition to an existing genQueryInp_t, from three parameters.
 *       One is an iCAT attribute index given without its 'COL_' prefix.
 *       The second one is the SQL operator. The third one is the value and may contain wildcards.
 *       To be used with #msiAddSelectFieldToGenQuery and #msiExecGenQuery to build queries from the
 *       results of other microservices or actions within an iRODS rule.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] attribute - Required - A STR_MS_T with the iCAT attribute name (see wiki.irods.org/index.php/icatAttributes).
 * \param[in] opr - Required - A STR_MS_T with the operator.
 * \param[in] value - Required - A STR_MS_T with the value.
 * \param[in,out] queryInput - Required - A GenQueryInp_MS_T.
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
msiAddConditionToGenQuery( msParam_t *attribute, msParam_t *opr, msParam_t *value, msParam_t *queryInput, ruleExecInfo_t *rei ) {
    genQueryInp_t *genQueryInp;
    char condStr[MAX_NAME_LEN];
    char *att_str, *op_str, *val_str;
    int att_inx;

    /*************************************  INIT **********************************/

    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiAddConditionToGenQuery" )

    /* Sanity checks */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiAddConditionToGenQuery: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /********************************** PARAM PARSING  *********************************/

    /* Parse attribute */
    if ( ( att_str = parseMspForStr( attribute ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiAddConditionToGenQuery: input attribute is NULL." );
        return USER__NULL_INPUT_ERR;
    }

    /* Parse operator */
    if ( ( op_str = parseMspForStr( opr ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiAddConditionToGenQuery: input opr is NULL." );
        return USER__NULL_INPUT_ERR;
    }

    /* Parse value */
    if ( ( val_str = parseMspForStr( value ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiAddConditionToGenQuery: input value is NULL." );
        return USER__NULL_INPUT_ERR;
    }

    /* Check for proper parameter type for queryInput */
    if ( queryInput->type && strcmp( queryInput->type, GenQueryInp_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiAddConditionToGenQuery: queryInput is not of type GenQueryInp_MS_T." );
        return USER_PARAM_TYPE_ERR;
    }

    /* Parse queryInput. Must not be empty. */
    if ( !queryInput->inOutStruct ) {
        rodsLog( LOG_ERROR, "msiAddConditionToGenQuery: input queryInput is NULL." );
        return USER__NULL_INPUT_ERR;
    }
    else {
        genQueryInp = ( genQueryInp_t* )queryInput->inOutStruct;
    }


    /***************************** ADD CONDITION TO QUERY INPUT  *****************************/

    /* Get attribute index */
    att_inx = getAttrIdFromAttrName( att_str );

    /* Error? */
    if ( att_inx < 0 ) {
        rodsLog( LOG_ERROR, "msiAddConditionToGenQuery: Unable to get valid ICAT column index." );
        return att_inx;
    }

    /* Make the condition */
    snprintf( condStr, MAX_NAME_LEN, " %s '%s'", op_str, val_str );

    /* Add condition to genQueryInput */
    addInxVal( &genQueryInp->sqlCondInp, att_inx, condStr );  /* condStr gets strdup'ed */


    /*********************************** DONE *********************************************/
    return 0;
}



/**
 * \fn msiPrintGenQueryOutToBuffer(msParam_t *queryOut, msParam_t *format, msParam_t *buffer, ruleExecInfo_t *rei)
 *
 * \brief  Writes the contents of a GenQueryOut_MS_T into a BUF_LEN_MS_T.
 *
 * \module core
 *
 * \author  Antoine de Torcy
 * \date    2009-12-16
 *
 * \note This microservice writes the contents of a GenQueryOut_MS_T into a BUF_LEN_MS_T.
 *       The results can be formatted with an optional C-style format string the same way it is done in iquest.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] queryOut - Required - A GenQueryOut_MS_T.
 * \param[in] format - Optional - A STR_MS_T with a C-style format string, like in iquest.
 * \param[out] buffer - A BUF_LEN_MS_T
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
msiPrintGenQueryOutToBuffer( msParam_t *queryOut, msParam_t *format, msParam_t *buffer, ruleExecInfo_t *rei ) {
    genQueryOut_t *genQueryOut;
    char *format_str;
    bytesBuf_t *bytesBuf;
    FILE *stream;
    char readbuffer[MAX_NAME_LEN];

    /*************************************  INIT **********************************/

    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiPrintGenQueryOutToBuffer" )

    /* Sanity checks */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiPrintGenQueryOutToBuffer: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /********************************** PARAM PARSING  *********************************/

    /* Check for proper param type */
    if ( !queryOut || !queryOut->inOutStruct || !queryOut->type || strcmp( queryOut->type, GenQueryOut_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiPrintGenQueryOutToBuffer: Invalid input for queryOut." );
        return USER_PARAM_TYPE_ERR;
    }
    genQueryOut = ( genQueryOut_t * )queryOut->inOutStruct;


    /* Parse format */
    format_str = parseMspForStr( format );


    /********************************** EXTRACT SQL RESULTS  *********************************/

    /* Let's use printGenQueryOut() here for the sake of consistency over efficiency (somewhat). It needs a stream. */
    char filename[7];
    memset( filename, 'X', sizeof( filename ) );
    filename[sizeof( filename ) - 1] = '\0';
    umask( S_IRUSR | S_IWUSR );
    int fd = mkstemp( filename );
    if ( fd < 0 ) { /* Since it won't be caught by printGenQueryOut */
        rodsLog( LOG_ERROR, "msiPrintGenQueryOutToBuffer: mkstemp() failed." );
        return ( FILE_OPEN_ERR ); /* accurate enough */
    }
    stream = fdopen( fd, "w" );
    if ( !stream ) { /* Since it won't be caught by printGenQueryOut */
        rodsLog( LOG_ERROR, "msiPrintGenQueryOutToBuffer: fdopen() failed." );
        return ( FILE_OPEN_ERR ); /* accurate enough */
    }

    /* Write results to temp file */
    rei->status = printGenQueryOut( stream, format_str, NULL, genQueryOut );
    if ( rei->status < 0 ) {
        rodsLog( LOG_ERROR, "msiPrintGenQueryOutToBuffer: printGenQueryOut() failed, status = %d", rei->status );
        fclose( stream );
        return rei->status;
    }

    /* bytesBuf init */
    bytesBuf = ( bytesBuf_t * )malloc( sizeof( bytesBuf_t ) );
    memset( bytesBuf, 0, sizeof( bytesBuf_t ) );

    /* Read from temp file and write to bytesBuf */
    rewind( stream );
    while ( fgets( readbuffer, MAX_NAME_LEN, stream ) != NULL ) {
        appendToByteBuf( bytesBuf, readbuffer );
    }


    /********************************* RETURN AND DONE **********************************/

    /* Free memory previously allocated for previous result batches (when used in loop). */
    resetMsParam( buffer );

    /* Fill bytesBuf in our buffer output */
    fillBufLenInMsParam( buffer, bytesBuf->len, bytesBuf );
    fclose( stream );

    return 0;

}



