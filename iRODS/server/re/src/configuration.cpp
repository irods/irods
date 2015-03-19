/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include <sys/stat.h>
#include <time.h>
#include "configuration.hpp"
#include "utils.hpp"
#include "rules.hpp"
#include "index.hpp"
#include "cache.hpp"
#include "region.hpp"
#include "functions.hpp"
#include "filesystem.hpp"
#include "sharedmemory.hpp"
#include "icatHighLevelRoutines.hpp"
#include "modAVUMetadata.hpp"

#include "irods_get_full_path_for_config_file.hpp"
#include "irods_log.hpp"

#if defined( osx_platform )
#undef CACHE_ENABLE
#endif


#ifdef DEBUG
#include "re.hpp"
#endif
Cache ruleEngineConfig = {
    NULL, /* unsigned char *address */
    NULL, /* unsigned char *pointers */
    0, /* size_t dataSize */
    0, /* size_t cacheSize */
    UNINITIALIZED, /* RuleEngineStatus coreRuleSetStatus */
    UNINITIALIZED, /* RuleEngineStatus appRuleSetStatus */
    UNINITIALIZED, /* RuleEngineStatus extRuleSetStatus */
    UNINITIALIZED, /* RuleEngineStatus sysFuncDescIndexStatus */
    UNINITIALIZED, /* RuleEngineStatus coreFuncDescIndexStatus */
    UNINITIALIZED, /* RuleEngineStatus appFuncDescIndexStatus */
    UNINITIALIZED, /* RuleEngineStatus extFuncDescIndexStatus */
    UNINITIALIZED, /* RuleEngineStatus ruleEngineStatus */
    UNINITIALIZED, /* RuleEngineStatus cacheStatus */
    UNINITIALIZED, /* RuleEngineStatus sysRegionStatus */
    UNINITIALIZED, /* RuleEngineStatus regionCoreStatus */
    UNINITIALIZED, /* RuleEngineStatus regionAppStatus */
    UNINITIALIZED, /* RuleEngineStatus regionExtStatus */
    NULL, /* RuleSet *coreRuleSet */
    NULL, /* RuleSet *appRuleSet */
    NULL, /* RuleSet *extRuleSet */
    NULL, /* Env *sysFuncDescIndex */
    NULL, /* Env *coreFuncDescIndex */
    NULL, /* Env *appFuncDescIndex */
    NULL, /* Env *extFuncDescIndex */
    NULL, /* Region *sysRegion */
    NULL, /* Region *coreRegion */
    NULL, /* Region *appRegion */
    NULL, /* Region *extRegion */
    0, /* int tvarNumber */
    0, /* int clearDelayed */
    time_type_initializer, /* time_type timestamp */
    time_type_initializer, /* time_type updateTS */
    0, /* int version */
    0, /* int logging */
    "", /* char ruleBase[RULE_SET_DEF_LENGTH] */
};

void removeRuleFromExtIndex( char *ruleName, int i ) {
    if ( isComponentInitialized( ruleEngineConfig.extFuncDescIndexStatus ) ) {
        FunctionDesc *fd = ( FunctionDesc * )lookupFromHashTable( ruleEngineConfig.extFuncDescIndex->current, ruleName );
        if ( fd != NULL ) {
            RuleIndexList *rd = FD_RULE_INDEX_LIST( fd );
            removeNodeFromRuleIndexList2( rd, i );
            if ( rd->head == NULL ) {
                deleteFromHashTable( ruleEngineConfig.extFuncDescIndex->current, ruleName );
            }
        }
    }

}
void appendRuleIntoExtIndex( RuleDesc *rule, int i, Region *r ) {
    FunctionDesc *fd = ( FunctionDesc * )lookupFromHashTable( ruleEngineConfig.extFuncDescIndex->current, RULE_NAME( rule->node ) );
    RuleIndexList *rd;
    if ( fd == NULL ) {
        rd = newRuleIndexList( RULE_NAME( rule->node ), i, r );
        fd = newRuleIndexListFD( rd, NULL, r );

        insertIntoHashTable( ruleEngineConfig.extFuncDescIndex->current, RULE_NAME( rule->node ), fd );
    }
    else {
        if ( getNodeType( fd ) == N_FD_RULE_INDEX_LIST ) {
            rd = FD_RULE_INDEX_LIST( fd );
            appendRuleNodeToRuleIndexList( rd, i , r );
        }
        else if ( getNodeType( fd ) == N_FD_EXTERNAL ) {
            /* combine N_FD_EXTERNAL with N_FD_RULE_LIST */
            updateInHashTable( ruleEngineConfig.extFuncDescIndex->current, RULE_NAME( rule->node ), newRuleIndexListFD( newRuleIndexList( RULE_NAME( rule->node ), i, r ), fd->exprType, r ) );
        }
        else {
            /* todo error handling */
        }
    }
}

void prependRuleIntoAppIndex( RuleDesc *rule, int i, Region *r ) {
    RuleIndexList *rd;
    FunctionDesc *fd = ( FunctionDesc * )lookupFromHashTable( ruleEngineConfig.appFuncDescIndex->current, RULE_NAME( rule->node ) );
    if ( fd == NULL ) {
        rd = newRuleIndexList( RULE_NAME( rule->node ), i, r );
        fd = newRuleIndexListFD( rd, NULL, r );
        insertIntoHashTable( ruleEngineConfig.appFuncDescIndex->current, RULE_NAME( rule->node ), fd );
    }
    else {
        rd = FD_RULE_INDEX_LIST( fd );
        prependRuleNodeToRuleIndexList( rd, i , r );
    }
}
int checkPointExtRuleSet( Region *r ) {
    ruleEngineConfig.extFuncDescIndex = newEnv( newHashTable2( 100, r ), ruleEngineConfig.extFuncDescIndex, NULL, r );
    return ruleEngineConfig.extRuleSet->len;
}
/*void appendAppRule(RuleDesc *rd, Region *r) {
	int i = ruleEngineConfig.appRuleSet->len++;
	ruleEngineConfig.appRuleSet->rules[i] = rd;
	appendRuleIntoIndex(rd, i, r);
}*/
void prependAppRule( RuleDesc *rd, Region *r ) {
    int i = ruleEngineConfig.appRuleSet->len++;
    ruleEngineConfig.appRuleSet->rules[i] = rd;
    prependRuleIntoAppIndex( rd, i, r );
}
void popExtRuleSet( int checkPoint ) {
    /*int i;
    for(i = checkPoint; i < ruleEngineConfig.extRuleSet->len; i++) {
    	removeRuleFromExtIndex(RULE_NAME(ruleEngineConfig.extRuleSet->rules[i]->node), i);
    }*/
    Env *temp = ruleEngineConfig.extFuncDescIndex;
    ruleEngineConfig.extFuncDescIndex = temp->previous;
    /* deleteEnv(temp, 1); */
    ruleEngineConfig.extRuleSet->len = checkPoint;
}
RuleEngineStatus getRuleEngineStatus() {
    return ruleEngineConfig.ruleEngineStatus;
}
/* RuleEngineStatus getRuleEngineMemStatus() {
    return _ruleEngineMemStatus;
}
void setAppRuleSetStatus(RuleEngineStatus s) {
    _ruleEngineStatus = s;
}
RuleEngineStatus getAppRuleSetStatus() {
    return _ruleEngineStatus;
}
void setRuleEngineMemStatus(RuleEngineStatus s) {
	_ruleEngineMemStatus = s;
} */
int clearResources( int resources ) {
    clearFuncDescIndex( APP, app );
    clearFuncDescIndex( SYS, sys );
    clearFuncDescIndex( CORE, core );
    clearFuncDescIndex( EXT, ext );
    clearRegion( APP, app );
    clearRegion( SYS, sys );
    clearRegion( CORE, core );
    clearRegion( EXT, ext );
    clearRuleSet( APP, app );
    clearRuleSet( CORE, core );
    clearRuleSet( EXT, ext );

    if ( ( resources & RESC_CACHE ) && isComponentAllocated( ruleEngineConfig.cacheStatus ) ) {
        free( ruleEngineConfig.address );
        ruleEngineConfig.address = NULL;
        ruleEngineConfig.cacheStatus = UNINITIALIZED;
    }
    return 0;
}
List hashtablesToClear = {0, NULL, NULL};
List envToClear = {0, NULL, NULL};
List regionsToClear = {0, NULL, NULL};
List memoryToFree = {0, NULL, NULL};

void delayClearResources( int resources ) {
    /*if((resources & RESC_RULE_INDEX) && ruleEngineConfig.ruleIndexStatus == INITIALIZED) {
    	listAppendNoRegion(hashtablesToClear, ruleEngineConfig.ruleIndex);
    	ruleEngineConfig.ruleIndexStatus = UNINITIALIZED;
    }
    if((resources & RESC_COND_INDEX) && ruleEngineConfig.condIndexStatus == INITIALIZED) {
    	listAppendNoRegion(hashtableToClear, ruleEngineConfig.condIndex);
    	ruleEngineConfig.condIndexStatus = UNINITIALIZED;
    }*/
    delayClearRegion( APP, app );
    delayClearRegion( SYS, sys );
    delayClearRegion( CORE, core );
    delayClearRegion( EXT, ext );
    delayClearRuleSet( APP, app );
    delayClearRuleSet( CORE, core );
    delayClearRuleSet( EXT, ext );
    delayClearFuncDescIndex( APP, app );
    delayClearFuncDescIndex( SYS, sys );
    delayClearFuncDescIndex( CORE, core );
    delayClearFuncDescIndex( EXT, ext );

    if ( ( resources & RESC_CACHE ) && ruleEngineConfig.cacheStatus == INITIALIZED ) {
        listAppendNoRegion( &memoryToFree, ruleEngineConfig.address );
        ruleEngineConfig.cacheStatus = UNINITIALIZED;
    }
}

void clearDelayed() {
    ListNode *n = envToClear.head;
    while ( n != NULL ) {
        /* deleteEnv((Env *) n->value, 1); */
        listRemoveNoRegion( &envToClear, n );
        n = envToClear.head;
    }
    n = hashtablesToClear.head;
    while ( n != NULL ) {
        deleteHashTable( ( Hashtable * ) n->value, nop );
        listRemoveNoRegion( &hashtablesToClear, n );
        n = hashtablesToClear.head;
    }
    n = regionsToClear.head;
    while ( n != NULL ) {
        region_free( ( Region * ) n->value );
        listRemoveNoRegion( &regionsToClear, n );
        n = regionsToClear.head;
    }
    n = memoryToFree.head;
    while ( n != NULL ) {
        free( n->value );
        listRemoveNoRegion( &memoryToFree, n );
        n = memoryToFree.head;
    }
}

void setCacheAddress( unsigned char *addr, RuleEngineStatus status, long size ) {
    ruleEngineConfig.address = addr;
    ruleEngineConfig.cacheStatus = status;
    ruleEngineConfig.cacheSize = size;

}

int generateLocalCache() {
    unsigned char *buf = NULL;
    if ( ruleEngineConfig.cacheStatus == INITIALIZED ) {
        free( ruleEngineConfig.address );
    }
    buf = ( unsigned char * )malloc( SHMMAX );
    if ( buf == NULL ) {
        return RE_OUT_OF_MEMORY;
    }
    setCacheAddress( buf, INITIALIZED, SHMMAX );
    return 0;
}
int generateFunctionDescriptionTables() {
    createFuncDescIndex( SYS, sys );
    createFuncDescIndex( CORE, core );
    createFuncDescIndex( APP, app );
    if ( !isComponentInitialized( ruleEngineConfig.extFuncDescIndexStatus ) ) {
        createFuncDescIndex( EXT, ext );
        ruleEngineConfig.extFuncDescIndex->previous = ruleEngineConfig.appFuncDescIndex;
    }
    else {
        Env *extEnv = ruleEngineConfig.extFuncDescIndex;
        while ( extEnv->previous != NULL ) {
            extEnv = extEnv->previous;
        }
        extEnv->previous = ruleEngineConfig.appFuncDescIndex;
    }
    ruleEngineConfig.appFuncDescIndex->previous = ruleEngineConfig.coreFuncDescIndex;
    ruleEngineConfig.coreFuncDescIndex->previous = ruleEngineConfig.sysFuncDescIndex;

    return 0;
}
void generateRuleSets() {
    createRuleSet( APP, app );
    createRuleSet( CORE, core );
    createRuleSet( EXT, ext );
}
void generateRegions() {
    createRegion( APP, app );
    createRegion( CORE, core );
    createRegion( SYS, sys );
    createRegion( EXT, ext );
}
int unlinkFuncDescIndex() {
    Env *extEnv = ruleEngineConfig.extFuncDescIndex;
    while ( extEnv->previous != ruleEngineConfig.appFuncDescIndex ) {
        extEnv = extEnv->previous;
    }
    extEnv->previous = NULL;
    ruleEngineConfig.appFuncDescIndex->previous = NULL;
    ruleEngineConfig.coreFuncDescIndex->previous = NULL;
    return 0;
}
int clearRuleIndex( ruleStruct_t *inRuleStruct ) {
    if ( inRuleStruct == &coreRuleStrct ) {
        clearResources( RESC_CORE_FUNC_DESC_INDEX );
    }
    else if ( inRuleStruct == &appRuleStrct ) {
        clearResources( RESC_APP_FUNC_DESC_INDEX );
    }
    return 0;
}


int createRuleIndex( ruleStruct_t *inRuleStruct ) {
    if ( inRuleStruct == &coreRuleStrct ) {
        createRuleNodeIndex( ruleEngineConfig.coreRuleSet, ruleEngineConfig.coreFuncDescIndex->current, CORE_RULE_INDEX_OFF, ruleEngineConfig.coreRegion );
        createCondIndex( ruleEngineConfig.coreRegion );
    }
    else if ( inRuleStruct == &appRuleStrct ) {
        createRuleNodeIndex( ruleEngineConfig.appRuleSet, ruleEngineConfig.appFuncDescIndex->current, APP_RULE_INDEX_OFF, ruleEngineConfig.appRegion );
    }
    return 0;

}

int loadRuleFromCacheOrFile( int processType, char *irbSet, ruleStruct_t *inRuleStruct ) {
    char r1[NAME_LEN], r2[RULE_SET_DEF_LENGTH], r3[RULE_SET_DEF_LENGTH];
    snprintf( r2, sizeof( r2 ), "%s", irbSet );
    int res = 0;

#ifdef DEBUG
    /*Cache *cache;*/
#endif

    /* get max timestamp */
    char fn[MAX_NAME_LEN];
    time_type timestamp = time_type_initializer, mtim;
    while ( strlen( r2 ) > 0 ) {
        rSplitStr( r2, r1, NAME_LEN, r3, RULE_SET_DEF_LENGTH, ',' );
        getRuleBasePath( r1, fn );
        if ( ( res = getModifiedTime( fn, &mtim ) ) != 0 ) {
            return res;
        }
        if ( time_type_gt( mtim, timestamp ) ) {
            time_type_set( timestamp, mtim );
        }
        snprintf( r2, sizeof( r2 ), "%s", r3 );
    }
    snprintf( r2, sizeof( r2 ), "%s", irbSet );

#ifdef CACHE_ENABLE

    int update = 0;
    unsigned char *buf = NULL;
    /* try to find shared memory cache */
    if ( processType == RULE_ENGINE_TRY_CACHE && inRuleStruct == &coreRuleStrct ) {
        buf = prepareNonServerSharedMemory();
        if ( buf != NULL ) {
            Cache * cache = restoreCache( buf );
            detachSharedMemory();

            if ( cache == NULL ) {
                rodsLog( LOG_ERROR, "Failed to restore cache." );
            }
            else {
                int diffIrbSet = strcmp( cache->ruleBase, irbSet ) != 0;
                if ( diffIrbSet ) {
                    rodsLog( LOG_DEBUG, "Rule base set changed, old value is %s", cache->ruleBase );
                }

                if ( diffIrbSet || time_type_gt( timestamp, cache->timestamp ) ) {
                    update = 1;
                    free( cache->address );
                    rodsLog( LOG_DEBUG, "Rule base set or rule files modified, force refresh." );
                }
                else {

                    cache->cacheStatus = INITIALIZED;
                    ruleEngineConfig = *cache;
                    /* generate extRuleSet */
                    generateRegions();
                    generateRuleSets();
                    generateFunctionDescriptionTables();
                    if ( inRuleStruct == &coreRuleStrct && ruleEngineConfig.ruleEngineStatus == UNINITIALIZED ) {
                        getSystemFunctions( ruleEngineConfig.sysFuncDescIndex->current, ruleEngineConfig.sysRegion );
                    }
                    /* ruleEngineConfig.extRuleSetStatus = LOCAL;
                    ruleEngineConfig.extFuncDescIndexStatus = LOCAL; */
                    /* createRuleIndex(inRuleStruct); */
                    ruleEngineConfig.ruleEngineStatus = INITIALIZED;
                    //free( cache );
                    return res;
                }
            }
            //free( cache );
        }
        else {
            rodsLog( LOG_DEBUG, "Cannot open shared memory." );
        }
    }
#endif

    if ( ruleEngineConfig.ruleEngineStatus == INITIALIZED ) {
        /* Reloading rule set, clear previously generated rule set */
        unlinkFuncDescIndex();
        clearRuleIndex( inRuleStruct );
    }

    generateRegions();
    generateRuleSets();
    generateFunctionDescriptionTables();
    if ( inRuleStruct == &coreRuleStrct && ruleEngineConfig.ruleEngineStatus == UNINITIALIZED ) {
        getSystemFunctions( ruleEngineConfig.sysFuncDescIndex->current, ruleEngineConfig.sysRegion );
    }
    /*ruleEngineConfig.extRuleSetStatus = LOCAL;
    ruleEngineConfig.extFuncDescIndexStatus = LOCAL;*/
    while ( strlen( r2 ) > 0 ) {
        int i = rSplitStr( r2, r1, NAME_LEN, r3, RULE_SET_DEF_LENGTH, ',' );
        if ( i == 0 ) {
            i = readRuleStructAndRuleSetFromFile( r1, inRuleStruct );
        }
#ifdef DEBUG
        printf( "%d rules in core rule set\n", ruleEngineConfig.coreRuleSet->len );
#endif
        if ( i != 0 ) {
            res = i;
            ruleEngineConfig.ruleEngineStatus = INITIALIZED;
            return res;
        }
        snprintf( r2, sizeof( r2 ), "%s", r3 );
    }

    createRuleIndex( inRuleStruct );
    /* set max timestamp */
    time_type_set( ruleEngineConfig.timestamp, timestamp );
    snprintf( ruleEngineConfig.ruleBase, sizeof( ruleEngineConfig.ruleBase ), "%s", irbSet );

#ifdef CACHE_ENABLE
    if ( ( processType == RULE_ENGINE_INIT_CACHE || update ) && inRuleStruct == &coreRuleStrct ) {
        unsigned char *shared = prepareServerSharedMemory();

        if ( shared != NULL ) {
            int ret = updateCache( shared, SHMMAX, &ruleEngineConfig, processType );
            detachSharedMemory();
            if ( ret != 0 ) {
                removeSharedMemory();
            }
        }
        else {
            rodsLog( LOG_ERROR, "Cannot open shared memory." );
        }
    }
#endif

    ruleEngineConfig.ruleEngineStatus = INITIALIZED;

    return res;
}
int readRuleStructAndRuleSetFromFile( char *ruleBaseName, ruleStruct_t *inRuleStrct ) {
    char rulesFileName[MAX_NAME_LEN];

    if ( ruleBaseName[0] == '/' || ruleBaseName[0] == '\\' ||
            ruleBaseName[1] == ':' ) {
        snprintf( rulesFileName, MAX_NAME_LEN, "%s", ruleBaseName );
    } else {
        std::string cfg_file, fn( ruleBaseName );
        fn += ".re";
        irods::error ret = irods::get_full_path_for_config_file( fn, cfg_file );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        snprintf( rulesFileName, sizeof( rulesFileName ), "%s", cfg_file.c_str() );
    }

    int errloc;
    rError_t errmsgBuf;
    errmsgBuf.errMsg = NULL;
    errmsgBuf.len = 0;

    char *buf = ( char * ) malloc( ERR_MSG_LEN * 1024 * sizeof( char ) );
    int res = 0;
    if ( inRuleStrct == &coreRuleStrct ) {
        if ( ( res = readRuleSetFromFile( ruleBaseName, ruleEngineConfig.coreRuleSet, ruleEngineConfig.coreFuncDescIndex, &errloc, &errmsgBuf, ruleEngineConfig.coreRegion ) ) == 0 ) {
        }
        else {
            errMsgToString( &errmsgBuf, buf, ERR_MSG_LEN * 1024 );
            rodsLog( LOG_ERROR, "%s", buf );
        }
    }
    else if ( inRuleStrct == &appRuleStrct ) {
        if ( ( res = readRuleSetFromFile( ruleBaseName, ruleEngineConfig.appRuleSet, ruleEngineConfig.appFuncDescIndex, &errloc, &errmsgBuf, ruleEngineConfig.appRegion ) ) == 0 ) {
        }
        else {
            errMsgToString( &errmsgBuf, buf, ERR_MSG_LEN * 1024 );
            rodsLog( LOG_ERROR, "%s", buf );
        }
    }
    free( buf );
    freeRErrorContent( &errmsgBuf );
    return res;
}

int availableRules() {
    return ( isComponentInitialized( ruleEngineConfig.appRuleSetStatus ) ? ruleEngineConfig.coreRuleSet->len : 0 ) + ( isComponentInitialized( ruleEngineConfig.appRuleSetStatus ) == INITIALIZED ? ruleEngineConfig.appRuleSet->len : 0 );
}


int readICatUserInfo( char *userName, char *attr, char userInfo[MAX_NAME_LEN], rsComm_t *rsComm ) {
    int status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condstr[MAX_NAME_LEN];
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condstr, MAX_NAME_LEN, "= '%s'", userName );
    addInxVal( &genQueryInp.sqlCondInp, COL_USER_NAME, condstr );
    snprintf( condstr, MAX_NAME_LEN, "= '%s'", attr );
    addInxVal( &genQueryInp.sqlCondInp, COL_META_USER_ATTR_NAME, condstr );

    addInxIval( &genQueryInp.selectInp, COL_META_USER_ATTR_VALUE, 1 );

    status = rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
    userInfo[0] = '\0';
    if ( status >= 0 && genQueryOut->rowCnt > 0 ) {
        if ( sqlResult_t *r = getSqlResultByInx( genQueryOut, COL_META_USER_ATTR_VALUE ) ) {
            rstrcpy( userInfo, &r->value[0], MAX_NAME_LEN );
        }
        genQueryInp.continueInx =  genQueryOut->continueInx;
    }
    clearGenQueryInp( &genQueryInp );
    freeGenQueryOut( &genQueryOut );
    return status;

}

int writeICatUserInfo( char *userName, char *attr, char *value, rsComm_t *rsComm ) {
    modAVUMetadataInp_t modAVUMetadataInp;

    modAVUMetadataInp.arg0 = "set";
    modAVUMetadataInp.arg1 = "-u";
    modAVUMetadataInp.arg2 = userName;
    modAVUMetadataInp.arg3 = attr;
    modAVUMetadataInp.arg4 = value;
    modAVUMetadataInp.arg5 = "";

    return rsModAVUMetadata( rsComm, &modAVUMetadataInp );
}

int readICatUserLogging( char *userName, int *logging, rsComm_t *rsComm ) {
    char userInfo[MAX_NAME_LEN];
    int i = readICatUserInfo( userName, RE_LOGGING_ATTR, userInfo, rsComm );
    if ( i < 0 ) {
        return i;
    }
    if ( strcmp( userInfo, "true" ) == 0 ) {
        *logging = 1;
    }
    else if ( strcmp( userInfo, "false" ) == 0 ) {
        *logging = 0;
    }
    else {
        return RE_RUNTIME_ERROR; /* todo change this to a more specific error code */
    }
    return 0;
}
int writeICatUserLogging( char *userName, int logging, rsComm_t *rsComm ) {
    char value[MAX_NAME_LEN];
    rstrcpy( value, ( char * )( logging ? "true" : "false" ), MAX_NAME_LEN );
    return writeICatUserInfo( userName, RE_LOGGING_ATTR, value, rsComm );
}
