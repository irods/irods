/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsRe.c - Routines for Server interfacing to the Rule Engine
 */

#include "rsGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "reconstants.h"
#include "configuration.h"

static char ruleSetInitialized[NAME_LEN]="";

/* initialize the Rule Engine if it hasn't been done yet */
#ifdef RULE_ENGINE_N
int 
initRuleEngine(int processType, rsComm_t *svrComm, char *ruleSet, char *dvmSet, char* fnmSet) {
   int status;
   if (strcmp(ruleSet, ruleSetInitialized)==0) {
      return(0); /* already done */
   }
   status = initRuleStruct(processType, svrComm, ruleSet, dvmSet, fnmSet);
   if (status == 0) {
      rstrcpy(ruleSetInitialized, ruleSet, NAME_LEN);
   }
   return(status);
}

/* clearCoreRule - clear the core rules. Code copied from msiAdmAddAppRuleStruct
 *
 */
int
clearCoreRule ()
{
  int i;

  i = unlinkFuncDescIndex();
  if (i < 0)
	  return i;
  i = clearResources(RESC_CORE_RULE_SET | RESC_CORE_FUNC_DESC_INDEX);
  if (i < 0)
	  return i;
  i = generateFunctionDescriptionTables();
  if (i < 0)
    return(i);
  i = clearDVarStruct(&coreRuleVarDef);
  if (i < 0)
    return(i);
  i = clearFuncMapStruct(&coreRuleFuncMapDef);
  bzero (ruleSetInitialized, sizeof (ruleSetInitialized));
  return(i);

}
#else
/* initialize the Rule Engine if it hasn't been done yet */
int 
initRuleEngine(rsComm_t *svrComm, char *ruleSet, char *dvmSet, char* fnmSet) {
   int status;
   if (strcmp(ruleSet, ruleSetInitialized)==0) {
      return(0); /* already done */
   }
   status = initRuleStruct(svrComm, ruleSet, dvmSet, fnmSet);
   if (status == 0) {
      rstrcpy(ruleSetInitialized, ruleSet, NAME_LEN);
   }
   return(status);
}

/* clearCoreRule - clear the core rules. Code copied from msiAdmAddAppRuleStruct
 *
 */
int
clearCoreRule ()
{
  int i;

  i = clearRuleStruct(&coreRuleStrct);
  if (i < 0)
    return(i);
  i = clearDVarStruct(&coreRuleVarDef);
  if (i < 0)
    return(i);
  i = clearFuncMapStruct(&coreRuleFuncMapDef);
  bzero (ruleSetInitialized, sizeof (ruleSetInitialized));
  return(i);

}

#endif
