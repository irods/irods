/**
 * @file ruleAdminMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "dataObjCreate.h"
#include "objMetaOpr.h"
#include "regDataObj.h"
/* #include "reAction.h" */
#include "miscServerFunct.h"




/**
 * \fn msiAdmChangeCoreIRB (msParam_t *newFileNameParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice copies the specified file in the configuration 
 * directory 'server/config/reConfigs' onto the core.irb file in the same directory.
 *
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author   Arcot Rajasekar
 * \date     2007-04
 * 
 * \note  This microservice expects the alternate file to be of the form *.irb and to be
 *   located in the configuration directory.
 *
 * \note This microservice requires iRODS administration privilege.
 *  
 * \note This microservice changes the core.irb file currently in the configuration
 * directory. It can be invoked through an irule. When the server is re-started, the 
 * new core file will be used by the rule engine.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] newFileNameParam - is a msParam of type STR_MS_T, which is a new core file name without the .irb extension.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified   none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect The core.irb file is replaced by the alternate core file.
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int msiAdmChangeCoreIRB(msParam_t *newFileNameParam, ruleExecInfo_t *rei)
{
  /*  newFileNameParam contains the file name of the new core.
      The file should be in reConfigs directory */

  char sysString[1000];
  int i;

  RE_TEST_MACRO ("Loopback on admChangeCoreIRB");

  if (strcmp (newFileNameParam->type,STR_MS_T) != 0)
    return(USER_PARAM_TYPE_ERR);

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  snprintf(sysString, 999, "cp %s/reConfigs/%s %s/reConfigs/core.irb",
	        (char *) getConfigDir (), (char *) newFileNameParam->inOutStruct, 
	   (char *) getConfigDir ());
  system(sysString);

   
  return(0);
}

/**
 * \fn msiAdmAppendToTopOfCoreIRB (msParam_t *newFileNameParam, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that changes the core.irb file currently in
 * the configuration directory 'server/config/reConfigs' by prepending the given
 * rules file to it. When the server is started next time, 
 * then the new core file will be used by the rule engine.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author   Arcot Rajasekar
 * \date     2007-04
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This microservice expects the prepended file to be of the form *.irb and to be
 * located in the configuration directory.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] newFileNameParam - is a msParam of type STR_MS_T, which is a prepended core file name without the .irb extension.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect core.irb file is prepended by the alternate new core file.
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmShowIRB, msiAdmChangeCoreIRB
**/
int msiAdmAppendToTopOfCoreIRB(msParam_t *newFileNameParam, ruleExecInfo_t *rei)
{
  /*  newFileNameParam contains the file name to be added to top of  core.irb
      The file  should be in reConfigs  directory */

  char sysString[1000];
  int i;
  char *conDir;

  RE_TEST_MACRO ("Loopback on admAppendToTopOfCoreIRB");

  if (strcmp (newFileNameParam->type,STR_MS_T) != 0)
    return(USER_PARAM_TYPE_ERR);

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);
  conDir = getConfigDir ();
  snprintf(sysString, 999, "cat %s/reConfigs/%s %s/reConfigs/core.irb > %s/reConfigs/admtmpcore.irb",
	   conDir, (char *) newFileNameParam->inOutStruct, 
	   conDir, conDir);
  system(sysString);
  snprintf(sysString, 999, "mv %s/reConfigs/admtmpcore.irb %s/reConfigs/core.irb",
	        conDir,conDir); 
  system(sysString);
  return(0);
 

}

/**
 * \fn msiAdmShowDVM (msParam_t *bufParam, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that reads the data-value-mapping data structure
 * in the Rule Engine and pretty-prints that structure to the stdout buffer.
 *
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author   Arcot Rajasekar  
 * \date     2007-08
 * 
 * \note This microservice uses a dummy parameter.
 *  
 * \note   Lists the currently loaded dollar variable mappings from the rule 
 *  engine memory. The list is written to stdout in ruleExecOut.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] bufParam - is a msParam (not used for anything, a dummy parameter)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified - rei->MsParamArray->MsParam->ruleExecOut->stdout is modified
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmShowIRB, msiAdmShowFNM
**/
int msiAdmShowDVM(msParam_t *bufParam, ruleExecInfo_t *rei)
{
  int i;

    _writeString("stdout","----------------------------- DVM -----------------------------\n",rei);

    i = _admShowDVM(bufParam, rei, &appRuleVarDef,0);
    if (i != 0)
      return(i);
    i = _admShowDVM(bufParam, rei, &coreRuleVarDef,1000);
    _writeString("stdout","----------------------------- DVM -----------------------------\n",rei);
    return(i);
}

int _admShowDVM(msParam_t *bufParam, ruleExecInfo_t *rei, rulevardef_t *inRuleVarDef, int inx)
{
  int j;
  char outStr[MAX_RULE_LENGTH];

  _writeString("stdout","---------------------------------------------------------------\n",rei);
  for( j = 0 ; j  < inRuleVarDef->MaxNumOfDVars ; j++) {
    sprintf(outStr," %-5i %-15.15s %s ===> %s\n",j+inx,inRuleVarDef->action[j], 
	    inRuleVarDef->varName[j], inRuleVarDef->var2CMap[j]);
    _writeString("stdout",outStr,rei);
  }
  _writeString("stdout","---------------------------------------------------------------\n",rei);
  return(0);
}

/**
 * \fn msiAdmShowFNM (msParam_t *bufParam, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that reads the function-name-mapping data structure
 * in the Rule Engine and pretty-prints that structure to the stdout buffer.
 *
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author   Arcot Rajasekar  
 * \date     2007-08
 * 
 * \note This microservice has a dummy parameter.
 *  
 * \note   This microservice lists the currently loaded microServices and action
 * name mappings from the rule engine memory. The list is written to stdout in ruleExecOut.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] bufParam - is a msParam (not used for anything, a dummy parameter)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified - rei->MsParamArray->MsParam->ruleExecOut->stdout is modified
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmShowIRB, msiAdmShowDVM
**/
int msiAdmShowFNM(msParam_t *bufParam, ruleExecInfo_t *rei)
{
  int i;

  _writeString("stdout","----------------------------- FNM -----------------------------\n",rei);
    i = _admShowFNM(bufParam, rei, &appRuleFuncMapDef,0);
    if (i != 0)
      return(i);
    i = _admShowFNM(bufParam, rei, &coreRuleFuncMapDef,1000);
  _writeString("stdout","----------------------------- FNM -----------------------------\n",rei);
    return(i);
}

int _admShowFNM(msParam_t *bufParam, ruleExecInfo_t *rei, rulefmapdef_t *inRuleFuncMapDef, int inx)
{

  int j;
  char outStr[MAX_RULE_LENGTH];

  _writeString("stdout","---------------------------------------------------------------\n",rei);
  for( j = 0 ; j  < inRuleFuncMapDef->MaxNumOfFMaps ; j++) {
    sprintf(outStr," %-5i %s ===> %s\n",j+inx, inRuleFuncMapDef->funcName[j], inRuleFuncMapDef->func2CMap[j]);
    _writeString("stdout",outStr,rei);
  }
  _writeString("stdout","---------------------------------------------------------------\n",rei);
  return(0);

}

/**
 * \fn msiAdmShowIRB (msParam_t *bufParam, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that reads the data structure in the Rule Engine, which holds the 
 * current set of Rules, and pretty-prints that structure to the stdout buffer.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author   Arcot Rajasekar
 * \date     2007-06
 * 
 * \note This microservice has a dummy parameter.
 *  
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] bufParam - is a msParam (not used for anything, a dummy parameter)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified - rei->MsParamArray->MsParam->ruleExecOut->stdout is modified
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmShowDVM, msiAdmShowFNM
**/

/**
 * \fn msiAdmClearAppRuleStruct (ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that clears the application level IRB Rules and DVM 
 * and FNM mappings that were loaded into the Rule engine's working memory.
 *
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author   Arcot Rajasekar
 * \date     2007-09
 * 
 * \note This microservice needs iRODS administration privileges to perform 
 * this function.
 *  
 * \note   Clears the application structures in the working memory of the rule engine
 * holding the rules, $-variable mappings and microService name mappings.
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
 * \sideeffect The rule engine's application-level ruleset and mappings get cleared.
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmAddAppRuleStruct, msiAdmShowIRB, msiAdmShowDVM, msiAdmShowFNM
**/

/**
 * \fn msiAdmAddAppRuleStruct(msParam_t *irbFilesParam, msParam_t *dvmFilesParam, 
 *  msParam_t *fnmFilesParam, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that reads the given file in the configuration directory
 * 'server/config/reConfigs' and adds them to the Rule list being used by the Rule 
 * Engine. These Rules are loaded at the beginning of the core.irb file, and hence can
 * be used to override the core Rules from the core.irb file (i.e., it adds application level 
 * IRB Rules and DVM and FNM mappings to the Rule engine).
 *
 * \module core 
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2007-09
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Adds the given rules (irb) file and $-variable mapping (dvm) and microService
 * logical microService logical name mapping (fnm) files to the working memory
 * of the rule engine. Any subsequent rule or microServices will also use the newly 
 * prepended rules and mappings
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] irbFilesParam - a msParam of type STR_MS_T, which is an application Rules file name without the .irb extension.
 * \param[in] dvmFilesParam - a msParam of type STR_MS_T, which is a variable file name mapping without the .dvm extension.
 * \param[in] fnmFilesParam - a msParam of type STR_MS_T, which is an application microService mapping file name without the .fnm extension.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect The rule engine's application ruleset and mappings get modified.
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmClearAppRuleStruct, msiAdmShowIRB, msiAdmShowDVM, msiAdmShowFNM
**/

/**
 * \fn msiAdmReadRulesFromFileIntoStruct(msParam_t *inIrbFileNameParam, msParam_t *outCoreRuleStruct, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that reads the given file in the configuration directory
 * 'server/config/reConfigs' or any file in the server local file system and 
 * reads them into a rule structure. 
 *
 * \module core 
 *
 * \since 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2010
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Adds the given rules from an irb-file to a given rule structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inIrbFileNameParam - a msParam of type STR_MS_T, a Rules file in irb format,
 *      either in 'server/config/reConfigs/' and without the .irb extension, 
 *      or a full file path in another directory on the server.
 * \param[out] outCoreRuleStruct - a msParam of type RuleStruct_MS_T (can be NULL in which case it is allocated)
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
 * \sa msiAdmInsertRulesFromStructIntoDB, msiGetRulesFromDBIntoStruct, msiAdmWriteRulesFromStructIntoFile
**/

/**
 * \fn msiAdmInsertRulesFromStructIntoDB(msParam_t *inIrbBaseNameParam, msParam_t *inCoreRuleStruct, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that reads the contents of a rule structure and writes them as
 * a new rule base set by populating the core rule tables of the iCAT.
 * It also maintains versioning of the rule base in the iCAT by giving an older version number to the existing base set of rules. 
 * 
 * \module core 
 *
 * \since 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2010
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Adds rules to the iCAT rule base.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inIrbBaseNameParam - a msParam of type STR_MS_T, which is name of the base that is being added.
 * \param[in] inCoreRuleStruct - a msParam of type RuleStruct_MS_T containing the rules.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified icat rule-tables get modified
 * \sideeffect none 
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadRulesFromFileIntoStruct, msiGetRulesFromDBIntoStruct, msiAdmWriteRulesFromStructIntoFile
**/



/**
 * \fn msiGetRulesFromDBIntoStruct(msParam_t *inIrbBaseNameParam, msParam_t *inVersionParam,
 *                  msParam_t *outCoreRuleStruct, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that queries the iCAT for rules with a given base name and 
 *     version number and populates a rule structure with those rules.
 *
 * \module core 
 *
 * \since 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2010
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Queries rules from the iCAT rule base.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inIrbBaseNameParam - a msParam of type STR_MS_T, which is the name of the base that is being queried.
 * \param[in] inVersionParam - a msParam of type STR_MS_T, which is the version string of the base being queried (use 0 for current version)
 * \param[out] outCoreRuleStruct - a msParam of type RuleStruct_MS_T (can be NULL in which case it is allocated)
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
 * \sa msiAdmReadRulesFromFileIntoStruct, msiAdmInsertRulesFromStructIntoDB, msiAdmWriteRulesFromStructIntoFile
**/


/**
 * \fn msiAdmWriteRulesFromStructIntoFile(msParam_t *inIrbFileNameParam, msParam_t *inCoreRuleStruct, ruleExecInfo_t *rei)
 *
 * \brief  This is a microservice that writes into a given file the contents of a given rule structure.
 * The file can be in 'server/config/reConfigs/' or any path on the server local file system.
 *
 * \module core 
 *
 * \since 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2010
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Writes a file (in irb format) with rules from a rule structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inIrbFileNameParam - a msParam of type STR_MS_T, which is either a base-name in which case the file will 
 *      be written into the 'server/config/reConfigs/' directory with an .irb extension, 
 *      or a full file path in another directory on the server.
 * \param[in] inCoreRuleStruct - a msParam of type RuleStruct_MS_T 
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect  A new rule file is created
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadRulesFromFileIntoStruct, msiAdmInsertRulesFromStructIntoDB, msiGetRulesFromDBIntoStruct
**/


/**
 * \fn msiAdmReadDVMapsFromFileIntoStruct(msParam_t *inDvmFileNameParam, msParam_t *outCoreDVMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Reads a DVM configuration file into a DVM structure
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Reads the given file in the configuration directory
 * 'server/config/reConfigs' or any file in the server local file system and 
 * puts them into a DVM structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inDvmFileNameParam - a msParam of type STR_MS_T
 * \param[in] outCoreDVMapStruct - a msParam of type RuleStruct_MS_T 
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
 * \sa msiAdmInsertDVMapsFromStructIntoDB, msiGetDVMapsFromDBIntoStruct, msiAdmWriteDVMapsFromStructIntoFile
**/
int msiAdmReadDVMapsFromFileIntoStruct(msParam_t *inDvmFileNameParam, msParam_t *outCoreDVMapStruct, ruleExecInfo_t *rei)
{

  int i;
  dvmStruct_t *coreDVMapStrct;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmReadDVMapsFromFileIntoStruct");

  
  if (inDvmFileNameParam == NULL ||
      strcmp (inDvmFileNameParam->type,STR_MS_T) != 0 ||
      inDvmFileNameParam->inOutStruct == NULL ||
      strlen((char *) inDvmFileNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  if (outCoreDVMapStruct->type != NULL &&
      strcmp (outCoreDVMapStruct->type,DVMapStruct_MS_T) == 0 &&
      outCoreDVMapStruct->inOutStruct != NULL) {
    coreDVMapStrct = (dvmStruct_t *) outCoreDVMapStruct->inOutStruct;
  }
  else {
    coreDVMapStrct = (dvmStruct_t *) malloc (sizeof(dvmStruct_t));
    coreDVMapStrct->MaxNumOfDVars = 0;    
  }
  i = readDVarStructFromFile((char*) inDvmFileNameParam->inOutStruct, coreDVMapStrct);
  if (i != 0) {
    if (strcmp (outCoreDVMapStruct->type,DVMapStruct_MS_T) != 0 )    
      free(coreDVMapStrct);
    return(i);
  }

  outCoreDVMapStruct->inOutStruct = (void *) coreDVMapStrct;
  if (outCoreDVMapStruct->type == NULL || 
      strcmp (outCoreDVMapStruct->type,DVMapStruct_MS_T) != 0)
    outCoreDVMapStruct->type = (char *) strdup(DVMapStruct_MS_T);
  return(0);
}

/**
 * \fn msiAdmInsertDVMapsFromStructIntoDB(msParam_t *inDvmBaseNameParam, msParam_t *inCoreDVMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Writes a DVM structure into the current DVM base
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that reads the contents of a DVM structure and writes them as
 * a new DVM base set in the iCAT.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inDvmBaseNameParam - a msParam of type STR_MS_T, the name of the target database
 * \param[in] inCoreDVMapStruct - a msParam of type RuleStruct_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified iCAT tables are modified
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadDVMapsFromFileIntoStruct, msiGetDVMapsFromDBIntoStruct, msiAdmWriteDVMapsFromStructIntoFile
**/
int msiAdmInsertDVMapsFromStructIntoDB(msParam_t *inDvmBaseNameParam, msParam_t *inCoreDVMapStruct, ruleExecInfo_t *rei)
{

  dvmStruct_t *coreDVMapStruct;
  int i;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmInsertDVMapsFromStructIntoDB");

  if (inDvmBaseNameParam == NULL || inCoreDVMapStruct == NULL ||
      strcmp (inDvmBaseNameParam->type,STR_MS_T) != 0 ||
      strcmp (inCoreDVMapStruct->type,DVMapStruct_MS_T) != 0 ||
      inDvmBaseNameParam->inOutStruct == NULL ||
      inCoreDVMapStruct->inOutStruct == NULL ||
      strlen((char *) inDvmBaseNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);

  coreDVMapStruct = (dvmStruct_t *) inCoreDVMapStruct->inOutStruct;
  i = insertDVMapsIntoDB((char *) inDvmBaseNameParam->inOutStruct, coreDVMapStruct, rei);
  return(i);
    
}


/**
 * \fn msiGetDVMapsFromDBIntoStruct(msParam_t *inDvmBaseNameParam, msParam_t *inVersionParam, msParam_t *outCoreDVMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Populates a DVM structure with DVMs from the given base name
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that queries the iCAT for DVM with a given base name and version number and populates a DVM rule structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inDvmBaseNameParam - a msParam of type STR_MS_T, the name of the base being queried
 * \param[in] inVersionParam - a msParam of type STR_MS_T, which is the version string of the base being queried (use 0 for current version)
 * \param[in] outCoreDVMapStruct - a msParam of type RuleStruct_MS_T
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
 * \sa msiAdmReadDVMapsFromFileIntoStruct, msiAdmInsertDVMapsFromStructIntoDB, msiAdmWriteDVMapsFromStructIntoFile
**/
int
msiGetDVMapsFromDBIntoStruct(msParam_t *inDvmBaseNameParam, msParam_t *inVersionParam, msParam_t *outCoreDVMapStruct, ruleExecInfo_t *rei)
{
    
  int i;
  dvmStruct_t *coreDVMapStrct;

  RE_TEST_MACRO ("Loopback on msiGetDVMapsFromDBIntoStruct");

  if (inDvmBaseNameParam == NULL ||
      strcmp (inDvmBaseNameParam->type,STR_MS_T) != 0 ||
      inDvmBaseNameParam->inOutStruct == NULL ||
      strlen((char *) inDvmBaseNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  if (inVersionParam == NULL ||
      strcmp (inVersionParam->type,STR_MS_T) != 0 ||
      inVersionParam->inOutStruct == NULL ||
      strlen((char *) inVersionParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  if (outCoreDVMapStruct->type != NULL &&
      strcmp (outCoreDVMapStruct->type,DVMapStruct_MS_T) == 0 &&
      outCoreDVMapStruct->inOutStruct != NULL) {
    coreDVMapStrct = (dvmStruct_t *) outCoreDVMapStruct->inOutStruct;
  }
  else {
    coreDVMapStrct = (dvmStruct_t *) malloc (sizeof(dvmStruct_t));
    coreDVMapStrct->MaxNumOfDVars = 0;
  }
  i = readDVMapStructFromDB((char*) inDvmBaseNameParam->inOutStruct, (char*) inVersionParam->inOutStruct,  coreDVMapStrct, rei);
  if (i != 0) {
    if (strcmp (outCoreDVMapStruct->type,DVMapStruct_MS_T) != 0 )
      free(coreDVMapStrct);
    return(i);
  }

  outCoreDVMapStruct->inOutStruct = (void *) coreDVMapStrct;
  if (outCoreDVMapStruct->type == NULL ||
      strcmp (outCoreDVMapStruct->type,DVMapStruct_MS_T) != 0)
    outCoreDVMapStruct->type = (char *) strdup(DVMapStruct_MS_T);
  return(0);
}

/**
 * \fn msiAdmWriteDVMapsFromStructIntoFile(msParam_t *inDvmFileNameParam, msParam_t *inCoreDVMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Writes to file the DVMs within a given DVM structure
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that writes into a given file the contents of a given DVM structure.
 * The file can be in 'server/config/reConfigs/' or any path on the server local file system.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inDvmFileNameParam - a msParam of type STR_MS_T, the name of the file to be written
 * \param[in] inCoreDVMapStruct - a msParam of type RuleStruct_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect A new DVM file is created
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadDVMapsFromFileIntoStruct, msiAdmInsertDVMapsFromStructIntoDB, msiGetDVMapsFromDBIntoStruct
**/
int
msiAdmWriteDVMapsFromStructIntoFile(msParam_t *inDvmFileNameParam, msParam_t *inCoreDVMapStruct, ruleExecInfo_t *rei)
{
  int i;
  dvmStruct_t *myDVMapStruct;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmWriteDVMapsFromStructIntoFile");

  if (inDvmFileNameParam == NULL || inCoreDVMapStruct == NULL ||
      strcmp (inDvmFileNameParam->type,STR_MS_T) != 0 ||
      strcmp (inCoreDVMapStruct->type,DVMapStruct_MS_T) != 0 ||
      inDvmFileNameParam->inOutStruct == NULL ||
      strlen((char *) inDvmFileNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);

  myDVMapStruct = (dvmStruct_t *) inCoreDVMapStruct->inOutStruct;
  i = writeDVMapsIntoFile((char *) inDvmFileNameParam->inOutStruct, myDVMapStruct, rei);
  return(i);

}


/**
 * \fn msiAdmReadFNMapsFromFileIntoStruct(msParam_t *inFnmFileNameParam, msParam_t *outCoreFNMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Reads a FNM configuration file into a FNM structure
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Reads the given file in the configuration directory
 * 'server/config/reConfigs' or any file in the server local file system and 
 * puts them into a FNM structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inFnmFileNameParam - a msParam of type STR_MS_T
 * \param[in] outCoreFNMapStruct - a msParam of type RuleStruct_MS_T 
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
 * \sa msiAdmInsertFNMapsFromStructIntoDB, msiGetFNMapsFromDBIntoStruct, msiAdmWriteFNMapsFromStructIntoFile
**/
int msiAdmReadFNMapsFromFileIntoStruct(msParam_t *inFnmFileNameParam, msParam_t *outCoreFNMapStruct, ruleExecInfo_t *rei)
{

  int i;
  fnmapStruct_t *coreFNMapStrct;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmReadFNMapsFromFileIntoStruct");

  
  if (inFnmFileNameParam == NULL ||
      strcmp (inFnmFileNameParam->type,STR_MS_T) != 0 ||
      inFnmFileNameParam->inOutStruct == NULL ||
      strlen((char *) inFnmFileNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  if (outCoreFNMapStruct->type != NULL &&
      strcmp (outCoreFNMapStruct->type,FNMapStruct_MS_T) == 0 &&
      outCoreFNMapStruct->inOutStruct != NULL) {
    coreFNMapStrct = (fnmapStruct_t *) outCoreFNMapStruct->inOutStruct;
  }
  else {
    coreFNMapStrct = (fnmapStruct_t *) malloc (sizeof(fnmapStruct_t));
    coreFNMapStrct->MaxNumOfFMaps = 0;    
  }
  i = readFuncMapStructFromFile((char*) inFnmFileNameParam->inOutStruct, coreFNMapStrct);
  if (i != 0) {
    if (strcmp (outCoreFNMapStruct->type,FNMapStruct_MS_T) != 0 )    
      free(coreFNMapStrct);
    return(i);
  }

  outCoreFNMapStruct->inOutStruct = (void *) coreFNMapStrct;
  if (outCoreFNMapStruct->type == NULL || 
      strcmp (outCoreFNMapStruct->type,FNMapStruct_MS_T) != 0)
    outCoreFNMapStruct->type = (char *) strdup(FNMapStruct_MS_T);
  return(0);
}


/**
 * \fn msiAdmInsertFNMapsFromStructIntoDB(msParam_t *inFnmBaseNameParam, msParam_t *inCoreFNMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Writes a FNM structure into the current FNM base
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that reads the contents of a FNM structure and writes them as
 * a new FNM base set in the iCAT.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inFnmBaseNameParam - a msParam of type STR_MS_T, the name of the target database
 * \param[in] inCoreFNMapStruct - a msParam of type RuleStruct_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified iCAT tables are modified
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadFNMapsFromFileIntoStruct, msiGetFNMapsFromDBIntoStruct, msiAdmWriteFNMapsFromStructIntoFile
**/
int msiAdmInsertFNMapsFromStructIntoDB(msParam_t *inFnmBaseNameParam, msParam_t *inCoreFNMapStruct, ruleExecInfo_t *rei)
{

  fnmapStruct_t *coreFNMapStruct;
  int i;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmInsertFNMapsFromStructIntoDB");

  if (inFnmBaseNameParam == NULL || inCoreFNMapStruct == NULL ||
      strcmp (inFnmBaseNameParam->type,STR_MS_T) != 0 ||
      strcmp (inCoreFNMapStruct->type,FNMapStruct_MS_T) != 0 ||
      inFnmBaseNameParam->inOutStruct == NULL ||
      inCoreFNMapStruct->inOutStruct == NULL ||
      strlen((char *) inFnmBaseNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);

  coreFNMapStruct = (fnmapStruct_t *) inCoreFNMapStruct->inOutStruct;
  i = insertFNMapsIntoDB((char *) inFnmBaseNameParam->inOutStruct, coreFNMapStruct, rei);
  return(i);
    
}


/**
 * \fn msiGetFNMapsFromDBIntoStruct(msParam_t *inFnmBaseNameParam, msParam_t *inVersionParam, msParam_t *outCoreFNMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Populates a FNM structure with FNMs from the given base name
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that queries the iCAT for FNM with a given base name and version number and populates a FNM rule structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inFnmBaseNameParam - a msParam of type STR_MS_T, the name of the base being queried
 * \param[in] inVersionParam - a msParam of type STR_MS_T, which is the version string of the base being queried (use 0 for current version)
 * \param[in] outCoreFNMapStruct - a msParam of type RuleStruct_MS_T
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
 * \sa msiAdmReadFNMapsFromFileIntoStruct, msiAdmInsertFNMapsFromStructIntoDB, msiAdmWriteFNMapsFromStructIntoFile
**/
int
msiGetFNMapsFromDBIntoStruct(msParam_t *inFnmBaseNameParam, msParam_t *inVersionParam, msParam_t *outCoreFNMapStruct, ruleExecInfo_t *rei)
{
    
  int i;
  fnmapStruct_t *coreFNMapStrct;

  RE_TEST_MACRO ("Loopback on msiGetFNMapsFromDBIntoStruct");

  if (inFnmBaseNameParam == NULL ||
      strcmp (inFnmBaseNameParam->type,STR_MS_T) != 0 ||
      inFnmBaseNameParam->inOutStruct == NULL ||
      strlen((char *) inFnmBaseNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  if (inVersionParam == NULL ||
      strcmp (inVersionParam->type,STR_MS_T) != 0 ||
      inVersionParam->inOutStruct == NULL ||
      strlen((char *) inVersionParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  if (outCoreFNMapStruct->type != NULL &&
      strcmp (outCoreFNMapStruct->type,FNMapStruct_MS_T) == 0 &&
      outCoreFNMapStruct->inOutStruct != NULL) {
    coreFNMapStrct = (fnmapStruct_t *) outCoreFNMapStruct->inOutStruct;
  }
  else {
    coreFNMapStrct = (fnmapStruct_t *) malloc (sizeof(fnmapStruct_t));
    coreFNMapStrct->MaxNumOfFMaps = 0;
  }
  i = readFNMapStructFromDB((char*) inFnmBaseNameParam->inOutStruct, (char*) inVersionParam->inOutStruct,  coreFNMapStrct, rei);
  if (i != 0) {
    if (strcmp (outCoreFNMapStruct->type,FNMapStruct_MS_T) != 0 )
      free(coreFNMapStrct);
    return(i);
  }

  outCoreFNMapStruct->inOutStruct = (void *) coreFNMapStrct;
  if (outCoreFNMapStruct->type == NULL ||
      strcmp (outCoreFNMapStruct->type,FNMapStruct_MS_T) != 0)
    outCoreFNMapStruct->type = (char *) strdup(FNMapStruct_MS_T);
  return(0);
}


/**
 * \fn msiAdmWriteFNMapsFromStructIntoFile(msParam_t *inFnmFileNameParam, msParam_t *inCoreFNMapStruct, ruleExecInfo_t *rei)
 *
 * \brief  Writes to file the FNMs within a given FNM structure
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that writes into a given file the contents of a given FNM structure.
 * The file can be in 'server/config/reConfigs/' or any path on the server local file system.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inFnmFileNameParam - a msParam of type STR_MS_T, the name of the file to be written
 * \param[in] inCoreFNMapStruct - a msParam of type RuleStruct_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect A new FNM file is created
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadFNMapsFromFileIntoStruct, msiAdmInsertFNMapsFromStructIntoDB, msiGetFNMapsFromDBIntoStruct
**/
int
msiAdmWriteFNMapsFromStructIntoFile(msParam_t *inFnmFileNameParam, msParam_t *inCoreFNMapStruct, ruleExecInfo_t *rei)
{
  int i;
  fnmapStruct_t *myFNMapStruct;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmWriteFNMapsFromStructIntoFile");

  if (inFnmFileNameParam == NULL || inCoreFNMapStruct == NULL ||
      strcmp (inFnmFileNameParam->type,STR_MS_T) != 0 ||
      strcmp (inCoreFNMapStruct->type,FNMapStruct_MS_T) != 0 ||
      inFnmFileNameParam->inOutStruct == NULL ||
      strlen((char *) inFnmFileNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);

  myFNMapStruct = (fnmapStruct_t *) inCoreFNMapStruct->inOutStruct;
  i = writeFNMapsIntoFile((char *) inFnmFileNameParam->inOutStruct, myFNMapStruct, rei);
  return(i);

}


/**
 * \fn msiAdmReadMSrvcsFromFileIntoStruct(msParam_t *inMsrvcFileNameParam, msParam_t *outCoreMsrvcStruct, ruleExecInfo_t *rei)
 *
 * \brief  Reads a microservice configuration file into a microservice structure
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note Reads the given file in the configuration directory
 * 'server/config/reConfigs' or any file in the server local file system and 
 * puts them into a microservice structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inMsrvcFileNameParam - a msParam of type STR_MS_T
 * \param[in] outCoreMsrvcStruct - a msParam of type RuleStruct_MS_T 
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
 * \sa msiAdmInsertMSrvcsFromStructIntoDB, msiGetMSrvcsFromDBIntoStruct, msiAdmWriteMSrvcsFromStructIntoFile
**/
int msiAdmReadMSrvcsFromFileIntoStruct(msParam_t *inMsrvcFileNameParam, msParam_t *outCoreMsrvcStruct, ruleExecInfo_t *rei)
{

  int i;
  msrvcStruct_t *coreMsrvcStrct;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmReadMSrvcsFromFileIntoStruct");

  
  if (inMsrvcFileNameParam == NULL ||
      strcmp (inMsrvcFileNameParam->type,STR_MS_T) != 0 ||
      inMsrvcFileNameParam->inOutStruct == NULL ||
      strlen((char *) inMsrvcFileNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  if (outCoreMsrvcStruct->type != NULL &&
      strcmp (outCoreMsrvcStruct->type,MsrvcStruct_MS_T) == 0 &&
      outCoreMsrvcStruct->inOutStruct != NULL) {
    coreMsrvcStrct = (msrvcStruct_t *) outCoreMsrvcStruct->inOutStruct;
  }
  else {
    coreMsrvcStrct = (msrvcStruct_t *) malloc (sizeof(msrvcStruct_t));
    coreMsrvcStrct->MaxNumOfMsrvcs = 0;    
  }
  i = readMsrvcStructFromFile((char*) inMsrvcFileNameParam->inOutStruct, coreMsrvcStrct);
  if (i != 0) {
    if (strcmp (outCoreMsrvcStruct->type,MsrvcStruct_MS_T) != 0 )    
      free(coreMsrvcStrct);
    return(i);
  }

  outCoreMsrvcStruct->inOutStruct = (void *) coreMsrvcStrct;
  if (outCoreMsrvcStruct->type == NULL || 
      strcmp (outCoreMsrvcStruct->type,MsrvcStruct_MS_T) != 0)
    outCoreMsrvcStruct->type = (char *) strdup(MsrvcStruct_MS_T);
  return(0);
}


/**
 * \fn msiAdmInsertMSrvcsFromStructIntoDB(msParam_t *inMsrvcBaseNameParam, msParam_t *inCoreMsrvcStruct, ruleExecInfo_t *rei)
 *
 * \brief  Writes a microservice structure into the current microservice base
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that reads the contents of a microservice structure and writes them as
 * a new microservice base set in the iCAT.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inMsrvcBaseNameParam - a msParam of type STR_MS_T, the name of the target database
 * \param[in] inCoreMsrvcStruct - a msParam of type RuleStruct_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified iCAT tables are modified
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadMSrvcsFromFileIntoStruct, msiGetMSrvcsFromDBIntoStruct, msiAdmWriteMSrvcsFromStructIntoFile
**/
int msiAdmInsertMSrvcsFromStructIntoDB(msParam_t *inMsrvcBaseNameParam, msParam_t *inCoreMsrvcStruct, ruleExecInfo_t *rei)

{

  msrvcStruct_t *coreMsrvcStruct;
  int i;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmInsertMSrvcsFromStructIntoDB");

  if ( inCoreMsrvcStruct == NULL ||
       strcmp (inCoreMsrvcStruct->type,MsrvcStruct_MS_T) != 0 ||
       inCoreMsrvcStruct->inOutStruct == NULL)
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);

  coreMsrvcStruct = (msrvcStruct_t *) inCoreMsrvcStruct->inOutStruct;
  i = insertMSrvcsIntoDB( coreMsrvcStruct, rei);
  return(i);
    
}


/**
 * \fn msiGetMSrvcsFromDBIntoStruct(msParam_t *inStatus, msParam_t *outCoreMsrvcStruct, ruleExecInfo_t *rei)
 *
 * \brief  Populates a microservice structure with microservices from the given base name
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that queries the iCAT for microservices with a given base name and version number and populates a microservice rule structure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inStatus - a msParam of type INT_MS_T or an integer stored in STR_MS_T, normally 1 if microservice is available, 0 otherwise.
 * \param[out] outCoreMsrvcStruct - a msParam of type MsrvcStruct_MS_T
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
 * \sa msiAdmReadMSrvcsFromFileIntoStruct, msiAdmInsertMSrvcsFromStructIntoDB, msiAdmWriteMSrvcsFromStructIntoFile
**/
int
msiGetMSrvcsFromDBIntoStruct(msParam_t *inStatus, msParam_t *outCoreMsrvcStruct, ruleExecInfo_t *rei)
{
    
  int i, stat;
  msrvcStruct_t *coreMsrvcStrct;

  RE_TEST_MACRO ("Loopback on msiGetMSrvcsFromDBIntoStruct");

  if (outCoreMsrvcStruct->type != NULL &&
      strcmp (outCoreMsrvcStruct->type,MsrvcStruct_MS_T) == 0 &&
      outCoreMsrvcStruct->inOutStruct != NULL) {
    coreMsrvcStrct = (msrvcStruct_t *) outCoreMsrvcStruct->inOutStruct;
  }
  else {
    coreMsrvcStrct = (msrvcStruct_t *) malloc (sizeof(msrvcStruct_t));
    coreMsrvcStrct->MaxNumOfMsrvcs = 0;
  }
  if (inStatus != NULL && inStatus->type != NULL && inStatus->inOutStruct != NULL) {
    if (strcmp (inStatus->type, INT_MS_T) == 0) {
      stat = *(int *)inStatus->inOutStruct;
    }
    else if (strcmp (inStatus->type, STR_MS_T) == 0) {
      stat = atoi((char *) inStatus->inOutStruct);
    }
    else {
    	free(coreMsrvcStrct); // cppcheck - Memory leak: coreMsrvcStrct
    	return(USER_PARAM_TYPE_ERR);
    }
  }
  else {
	free(coreMsrvcStrct); // cppcheck - Memory leak: coreMsrvcStrct
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);
  }
  i = readMsrvcStructFromDB( stat, coreMsrvcStrct, rei);
  if (i != 0) {
    if (strcmp (outCoreMsrvcStruct->type,MsrvcStruct_MS_T) != 0 )
      free(coreMsrvcStrct);
    return(i);
  }

  outCoreMsrvcStruct->inOutStruct = (void *) coreMsrvcStrct;
  if (outCoreMsrvcStruct->type == NULL ||
      strcmp (outCoreMsrvcStruct->type,MsrvcStruct_MS_T) != 0)
    outCoreMsrvcStruct->type = (char *) strdup(MsrvcStruct_MS_T);
  return(0);
}

/**
 * \fn msiAdmWriteMSrvcsFromStructIntoFile(msParam_t *inMsrvcFileNameParam, msParam_t *inCoreMsrvcStruct, ruleExecInfo_t *rei)
 *
 * \brief  Writes to file the microservices within a given microservice structure
 *
 * \module core 
 *
 * \since after 2.5
 *
 * \author  Arcot Rajasekar
 * \date    2011
 * 
 * \note This microservice requires iRODS administration privileges.
 *  
 * \note  This is a microservice that writes into a given file the contents of a given microservice structure.
 * The file can be in 'server/config/reConfigs/' or any path on the server local file system.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inMsrvcFileNameParam - a msParam of type STR_MS_T, the name of the file to be written
 * \param[in] inCoreMsrvcStruct - a msParam of type MsrvcStruct_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect A new microservice file is created
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiAdmReadMSrvcsFromFileIntoStruct, msiAdmInsertMSrvcsFromStructIntoDB, msiGetMSrvcsFromDBIntoStruct
**/
int
msiAdmWriteMSrvcsFromStructIntoFile(msParam_t *inMsrvcFileNameParam, msParam_t *inCoreMsrvcStruct, ruleExecInfo_t *rei)
{
  int i;
  msrvcStruct_t *myMsrvcStruct;

  if ((i = isUserPrivileged(rei->rsComm)) != 0)
    return (i);

  RE_TEST_MACRO ("Loopback on msiAdmWriteMSrvcsFromStructIntoFile");

  if (inMsrvcFileNameParam == NULL || inCoreMsrvcStruct == NULL ||
      strcmp (inMsrvcFileNameParam->type,STR_MS_T) != 0 ||
      strcmp (inCoreMsrvcStruct->type,MsrvcStruct_MS_T) != 0 ||
      inMsrvcFileNameParam->inOutStruct == NULL ||
      strlen((char *) inMsrvcFileNameParam->inOutStruct) == 0 )
    return(PARAOPR_EMPTY_IN_STRUCT_ERR);

  myMsrvcStruct = (msrvcStruct_t *) inCoreMsrvcStruct->inOutStruct;
  i = writeMSrvcsIntoFile((char *) inMsrvcFileNameParam->inOutStruct, myMsrvcStruct, rei);
  return(i);

}

