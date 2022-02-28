#ifndef IRODS_NREP_CONFIGURATION_HPP
#define IRODS_NREP_CONFIGURATION_HPP

#include "irods/private/re/rules.hpp"
#include "irods/irods_hashtable.h"
#include "irods/private/re/parser.hpp"
#include "irods/private/re/datetime.hpp"
#include "irods/checksum.h"

#define RESC_CORE_RULE_SET 0x1
#define RESC_APP_RULE_SET 0x2
#define RESC_EXT_RULE_SET 0x4
#define RESC_SYS_FUNC_DESC_INDEX 0x10
#define RESC_CORE_FUNC_DESC_INDEX 0x20
#define RESC_APP_FUNC_DESC_INDEX 0x40
#define RESC_EXT_FUNC_DESC_INDEX 0x80
#define RESC_REGION_SYS 0x100
#define RESC_REGION_CORE 0x200
#define RESC_REGION_APP 0x400
#define RESC_REGION_EXT 0x800
#define RESC_CACHE 0x1000

typedef enum ruleEngineStatus {
    UNINITIALIZED,
    INITIALIZED,
    COMPRESSED,
    /*SHARED,
    LOCAL,
    DISABLED*/
} RuleEngineStatus;

struct Cache {
    Cache(); 
    unsigned char *address;
    unsigned char *pointers;
    size_t dataSize;
    size_t cacheSize;
    RuleEngineStatus coreRuleSetStatus;
    RuleEngineStatus appRuleSetStatus;
    RuleEngineStatus extRuleSetStatus;
    RuleEngineStatus sysFuncDescIndexStatus;
    RuleEngineStatus coreFuncDescIndexStatus;
    RuleEngineStatus appFuncDescIndexStatus;
    RuleEngineStatus extFuncDescIndexStatus;
    RuleEngineStatus ruleEngineStatus;
    RuleEngineStatus cacheStatus;
    RuleEngineStatus sysRegionStatus;
    RuleEngineStatus coreRegionStatus;
    RuleEngineStatus appRegionStatus;
    RuleEngineStatus extRegionStatus;
    RuleSet *coreRuleSet;
    RuleSet *appRuleSet;
    RuleSet *extRuleSet;
    Env *sysFuncDescIndex;
    Env *coreFuncDescIndex;
    Env *appFuncDescIndex;
    Env *extFuncDescIndex;
    Region *sysRegion;
    Region *coreRegion;
    Region *appRegion;
    Region *extRegion;
    int tvarNumber; /* counter for tvar generator */
    int clearDelayed;
    time_type timestamp;
    int logging;
    char ruleBase[RULE_SET_DEF_LENGTH];
    char hash[CHKSUM_LEN];
};

#define isComponentInitialized(x) ((x)==INITIALIZED || (x)==COMPRESSED)
#define isComponentAllocated(x) ((x)==INITIALIZED)
#define clearRegion(u, l) \
		if((resources & RESC_REGION_##u) && isComponentAllocated(ruleEngineConfig.l##Region##Status)) { \
			region_free(ruleEngineConfig.l##Region); \
			ruleEngineConfig.l##Region = NULL; \
			ruleEngineConfig.l##Region##Status = UNINITIALIZED; \
		} \

#define delayClearRegion(u, l) \
		if((resources & RESC_REGION_##u) && isComponentAllocated(ruleEngineConfig.l##Region##Status)) { \
			listAppendNoRegion(&regionsToClear, ruleEngineConfig.l##Region); \
			ruleEngineConfig.l##Region = NULL; \
			ruleEngineConfig.l##Region##Status = UNINITIALIZED; \
		} \

#define createRegion(u, l) \
		if(ruleEngineConfig.l##Region##Status != INITIALIZED) { \
			ruleEngineConfig.l##Region = make_region(0, NULL); \
			ruleEngineConfig.l##Region##Status = INITIALIZED; \
		} \

#define clearRuleSet(u, l) \
		if((resources & RESC_##u##_RULE_SET) && isComponentInitialized(ruleEngineConfig.l##RuleSetStatus)) { \
			ruleEngineConfig.l##RuleSet = NULL; \
			ruleEngineConfig.l##RuleSetStatus = UNINITIALIZED; \
		} \

#define delayClearRuleSet(u, l) \
		if((resources & RESC_##u##_RULE_SET) && isComponentInitialized(ruleEngineConfig.l##RuleSetStatus)) { \
			ruleEngineConfig.l##RuleSet = NULL; \
			ruleEngineConfig.l##RuleSetStatus = UNINITIALIZED; \
		} \

#define delayClearFuncDescIndex(u, l) \
		if((resources & RESC_##u##_FUNC_DESC_INDEX) && isComponentInitialized(ruleEngineConfig.l##FuncDescIndexStatus)) { \
			listAppendNoRegion(&envToClear, ruleEngineConfig.l##FuncDescIndex); \
			ruleEngineConfig.l##FuncDescIndex = NULL; \
			ruleEngineConfig.l##FuncDescIndexStatus = UNINITIALIZED; \
		} else if((resources & RESC_##u##_FUNC_DESC_INDEX) && ruleEngineConfig.l##FuncDescIndexStatus == COMPRESSED) { \
			ruleEngineConfig.l##FuncDescIndexStatus = UNINITIALIZED; \
		} \

#define createRuleSet(u, l) \
		if(!isComponentInitialized(ruleEngineConfig.l##RuleSetStatus)) { \
			ruleEngineConfig.l##RuleSet = (RuleSet *)region_alloc(ruleEngineConfig.l##Region, sizeof(RuleSet)); \
			ruleEngineConfig.l##RuleSet->len = 0; \
			ruleEngineConfig.l##RuleSetStatus = INITIALIZED; \
		} \

#define clearFuncDescIndex(u, l) \
	if((resources & RESC_##u##_FUNC_DESC_INDEX) && isComponentInitialized(ruleEngineConfig.l##FuncDescIndexStatus)) { \
		/* deleteEnv(ruleEngineConfig.l##FuncDescIndex, 1); */\
		ruleEngineConfig.l##FuncDescIndex = NULL; \
		ruleEngineConfig.l##FuncDescIndexStatus = UNINITIALIZED; \
	} \

#define createFuncDescIndex(u, l) \
	if(!isComponentInitialized(ruleEngineConfig.l##FuncDescIndexStatus)) { \
		ruleEngineConfig.l##FuncDescIndex = newEnv(NULL, NULL, NULL, ruleEngineConfig.l##Region); \
		ruleEngineConfig.l##FuncDescIndex->current = newHashTable2(1000, ruleEngineConfig.l##Region); \
		ruleEngineConfig.l##FuncDescIndexStatus = INITIALIZED; \
	} \

extern RuleEngineStatus _ruleEngineStatus;
extern int isServer;
extern Cache ruleEngineConfig;

RuleEngineStatus getRuleEngineStatus();
int unlinkFuncDescIndex();
int clearResources( int resources );
int clearCoreRuleIndex( );
int clearAppRuleIndex( );
int readRuleStructAndRuleSetFromFile( const char *ruleBaseName, const char *rulesFileName );
int loadRuleFromCacheOrFile( const char*, const char *irbSet );
int createCoreRuleIndex( );
int createAppRuleIndex( );
int availableRules();
void removeRuleFromExtIndex( char *ruleName, int i );
void appendRuleIntoExtIndex( RuleDesc *rule, int i, Region *r );
void prependRuleIntoAppIndex( RuleDesc *rule, int i, Region *r );
int checkPointExtRuleSet( Region *r );
void appendAppRule( RuleDesc *rd, Region *r );
void prependAppRule( RuleDesc *rd, Region *r );
void popExtRuleSet( int checkPoint );
void clearDelayed();
int generateFunctionDescriptionTables();
int readICatUserInfo( char *userName, char *attr, char userInfo[MAX_NAME_LEN], rsComm_t *rsComm );
int writeICatUserInfo( char *userName, char *attr, char *userInfo, rsComm_t *rsComm );
int readICatUserLogging( char *userName, int *logging, rsComm_t *rsComm );
int writeICatUserLogging( char *userName, int logging, rsComm_t *rsComm );

#define RE_LOGGING_ATTR "rulelogging"

#endif // IRODS_NREP_CONFIGURATION_HPP
