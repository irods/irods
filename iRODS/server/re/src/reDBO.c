/**
 * @file reDBO.c
 *
 * /brief Microservices for Database Objects (DBOs) and DB Resources (DBRs).
 */


/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.h"

#include "databaseObjControl.h"
#include "dataObjWrite.h"

/**
 * \fn msiDboExec(msParam_t *dbrName, msParam_t *dboName, msParam_t *dborName,
 *      msParam_t *options,
 *      msParam_t *inpParam1, msParam_t *inpParam2, 
 *      msParam_t *inpParam3, msParam_t *inpParam4, 
 *      msParam_t *inpParam5, msParam_t *inpParam6, 
 *      ruleExecInfo_t *rei)
 *
 * \brief Execute a database object on a DBR
 *
 * \module core
 *
 * \since 2.5
 *
 * \author Wayne Schroeder
 * \date   2010-11-23
 *
 * \usage See clients/icommands/test/rules3.0/ and https://www.irods.org/index.php/DBR
 *
 * \param[in] dbrName - a STR_MS_T, name of the DBR being used
 * \param[in] dboName - a STR_MS_T, name of the DBO being used
 * \param[in] dborName - a STR_MS_T, name of the DBOR being used
 * \param[in] options - a STR_MS_T, currently 'force' or not
 * \param[in] inpParam1 - Optional - STR_MS_T parameters to the DBO SQL.
 * \param[in] inpParam2 - Optional - STR_MS_T parameters to the DBO SQL.
 * \param[in] inpParam3 - Optional - STR_MS_T parameters to the DBO SQL.
 * \param[in] inpParam4 - Optional - STR_MS_T parameters to the DBO SQL.
 * \param[in] inpParam5 - Optional - STR_MS_T parameters to the DBO SQL.
 * \param[in] inpParam6 - Optional - STR_MS_T parameters to the DBO SQL.
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
msiDboExec(msParam_t *dbrName, msParam_t *dboName, msParam_t *dborName,
	   msParam_t *options,
	   msParam_t *inpParam1, msParam_t *inpParam2, 
	   msParam_t *inpParam3, msParam_t *inpParam4, 
	   msParam_t *inpParam5, msParam_t *inpParam6, 
	   ruleExecInfo_t *rei) {
    rsComm_t *rsComm; 
    char *myDbrName;
    char *myDboName;
    char *myDborName;
    char *myOptions;
    char *p1;
    char *p2;
    char *p3;
    char *p4;
    char *p5;
    char *p6;
    databaseObjControlInp_t databaseObjControlInp;
    databaseObjControlOut_t *databaseObjControlOut;
    int status;

    RE_TEST_MACRO ("    Calling msiDboExec")

    if (rei == NULL || rei->rsComm == NULL) {
	rodsLog (LOG_ERROR,
	  "msiDboExec rei or rsComm is NULL");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    rsComm = rei->rsComm;

    myDbrName = parseMspForStr(dbrName);
    if (myDbrName == NULL) {
	rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiDboExec: input dbrName is NULL");
        return (USER__NULL_INPUT_ERR);
    }

    myDboName = parseMspForStr(dboName);
    if (myDboName == NULL) {
	rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiDboExec: input dboName is NULL");
        return (USER__NULL_INPUT_ERR);
    }


    myDborName = parseMspForStr(dborName);
    if (myDborName == NULL) {
	rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiDboExec: input dborName is NULL");
        return (USER__NULL_INPUT_ERR);
    }

    myOptions = parseMspForStr(options);

    p1 = parseMspForStr(inpParam1);
    p2 = parseMspForStr(inpParam2);
    p3 = parseMspForStr(inpParam3);
    p4 = parseMspForStr(inpParam4);
    p5 = parseMspForStr(inpParam5);
    p6 = parseMspForStr(inpParam6);

    if (rei->status < 0) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiDboExec: input inpParam error. status = %d", rei->status);
        return (rei->status);
    }

    memset((void *)&databaseObjControlInp, 0, sizeof(databaseObjControlInp));

    databaseObjControlInp.option = DBO_EXECUTE;

    databaseObjControlInp.dbrName = myDbrName;
    databaseObjControlInp.dboName = myDboName;
    databaseObjControlInp.dborName = myDborName;

    if (strcmp(myOptions, "force")==0) {
       databaseObjControlInp.subOption = 1;
    }

    databaseObjControlInp.args[0] = p1;
    databaseObjControlInp.args[1] = p2;
    databaseObjControlInp.args[2] = p3;
    databaseObjControlInp.args[3] = p4;
    databaseObjControlInp.args[4] = p5;
    databaseObjControlInp.args[5] = p6;

    status = rsDatabaseObjControl(rsComm, &databaseObjControlInp, 
				  &databaseObjControlOut);
    if (status) {
       return(status);
    }
    if (*databaseObjControlOut->outBuf != '\0') {
       int stat2;
       stat2 = _writeString("stdout",databaseObjControlOut->outBuf,rei);
    }

    return(status);
}

/**
 * \fn msiDbrCommit(msParam_t *dbrName, ruleExecInfo_t *rei)
 *
 * \brief This microservice does a commit on a Database Resource (DBR)
 *
 * \module core
 *
 * \since 2.5
 *
 * \author Wayne Schroeder
 * \date   2010-11-23
 *
 * \usage See clients/icommands/test/rules3.0/ and https://www.irods.org/index.php/DBR
 *
 * \param[in] dbrName - a STR_MS_T, name of the DBR being used
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
msiDbrCommit(msParam_t *dbrName, ruleExecInfo_t *rei)
{
    rsComm_t *rsComm; 
    int status;

    databaseObjControlInp_t databaseObjControlInp;
    databaseObjControlOut_t *databaseObjControlOut;

    char *myDbrName;

    RE_TEST_MACRO ("    Calling msiDboCommit")

    if (rei == NULL || rei->rsComm == NULL) {
	rodsLog (LOG_ERROR,
	  "msiDbrCommit rei or rsComm is NULL");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    rsComm = rei->rsComm;

    memset((void *)&databaseObjControlInp, 0, sizeof(databaseObjControlInp));

    myDbrName = parseMspForStr(dbrName);
    if (myDbrName == NULL) {
       rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			   "msiDbrCommit: input dbrName is NULL");
       return(USER__NULL_INPUT_ERR);
    }

   databaseObjControlInp.dbrName = myDbrName;

   databaseObjControlInp.option =  DBR_COMMIT;

   status = rsDatabaseObjControl(rsComm, &databaseObjControlInp, 
				 &databaseObjControlOut);
   return(status);
}

/**
 * \fn msiDbrRollback(msParam_t *dbrName, ruleExecInfo_t *rei)
 *
 * \brief This microservice does a rollback on a Database Resource (DBR)
 *
 * \module core
 *
 * \since 2.5
 *
 * \author Wayne Schroeder
 * \date   2010-11-23
 *
 * \usage See clients/icommands/test/rules3.0/ and https://www.irods.org/index.php/DBR
 *
 * \param[in] dbrName - a STR_MS_T, name of the DBR being used
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
msiDbrRollback(msParam_t *dbrName, ruleExecInfo_t *rei)
{
    rsComm_t *rsComm; 
    int status;

    databaseObjControlInp_t databaseObjControlInp;
    databaseObjControlOut_t *databaseObjControlOut;

    char *myDbrName;

    RE_TEST_MACRO ("    Calling msiDboRollback")

    if (rei == NULL || rei->rsComm == NULL) {
	rodsLog (LOG_ERROR,
	  "msiDbrRollback rei or rsComm is NULL");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    rsComm = rei->rsComm;

    memset((void *)&databaseObjControlInp, 0, sizeof(databaseObjControlInp));

    myDbrName = parseMspForStr(dbrName);
    if (myDbrName == NULL) {
       rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			   "msiDbrRollback: input dbrName is NULL");
       return(USER__NULL_INPUT_ERR);
    }

   databaseObjControlInp.dbrName = myDbrName;

   databaseObjControlInp.option =  DBR_ROLLBACK;

   status = rsDatabaseObjControl(rsComm, &databaseObjControlInp, 
				 &databaseObjControlOut);
   return(status);

}
