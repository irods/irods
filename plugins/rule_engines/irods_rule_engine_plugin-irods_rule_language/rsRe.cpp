/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsRe.c - Routines for Server interfacing to the Rule Engine
 */

#include "rsGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "reconstants.hpp"
#include "configuration.hpp"
#include "reFuncDefs.hpp"
#include "irods_re_plugin.hpp"
#include "irods_re_structs.hpp"

/* initialize the Rule Engine if it hasn't been done yet */
int
initRuleEngine( const char* inst_name, int processType, rsComm_t *svrComm, const char *ruleSet, const char *dvmSet, const char* fnmSet ) {
    int status;
    status = initRuleStruct( inst_name, processType, svrComm, ruleSet, dvmSet, fnmSet );

    return status;
}

/* clearCoreRule - clear the core rules. Code copied from msiAdmAddAppRuleStruct
 *
 */
int
clearCoreRule() {
    int i;

    i = unlinkFuncDescIndex();
    if ( i < 0 ) {
        return i;
    }
    i = clearResources( RESC_CORE_RULE_SET | RESC_CORE_FUNC_DESC_INDEX );
    if ( i < 0 ) {
        return i;
    }
    i = generateFunctionDescriptionTables();
    if ( i < 0 ) {
        return i;
    }
    i = clearDVarStruct( &coreRuleVarDef );
    if ( i < 0 ) {
        return i;
    }
    i = clearFuncMapStruct( &coreRuleFuncMapDef );

    if ( i < 0 ) {
        return i;
    }

    return i;

}
