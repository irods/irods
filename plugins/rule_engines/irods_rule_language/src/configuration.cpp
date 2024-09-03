/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include <sys/stat.h>
#include <time.h>
#include "irods/private/re/configuration.hpp"
#include "irods/private/re/utils.hpp"
#include "irods/private/re/rules.hpp"
#include "irods/private/re/index.hpp"
#include "irods/private/re/cache.hpp"
#include "irods/locks.hpp"
#include "irods/region.h"
#include "irods/private/re/functions.hpp"
#include "irods/private/re/filesystem.hpp"
#include "irods/sharedmemory.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/rcMisc.h"
#include "irods/modAVUMetadata.h"
#include "irods/genQuery.h"
#include "irods/rsGenQuery.hpp"
#include "irods/rsModAVUMetadata.hpp"
#include "irods/private/re/reFuncDefs.hpp"
#include "irods/Hasher.hpp"
#include "irods/irods_hasher_factory.hpp"
#include "irods/irods_exception.hpp"

#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_log.hpp"
#include <boost/filesystem.hpp>
#include <memory>
#include <sstream>
#include <unordered_map>

using log_re = irods::experimental::log::rule_engine;
constexpr auto err_buf_len = ERR_MSG_LEN * 1024;

Cache::Cache() : address(NULL), /* unsigned char *address */
    pointers(NULL), /* unsigned char *pointers */
    dataSize(0), /* size_t dataSize */
    cacheSize(0), /* size_t cacheSize */
    coreRuleSetStatus(UNINITIALIZED), /* RuleEngineStatus coreRuleSetStatus */
    appRuleSetStatus(UNINITIALIZED), /* RuleEngineStatus appRuleSetStatus */
    extRuleSetStatus(UNINITIALIZED), /* RuleEngineStatus extRuleSetStatus */
    sysFuncDescIndexStatus(UNINITIALIZED), /* RuleEngineStatus sysFuncDescIndexStatus */
    coreFuncDescIndexStatus(UNINITIALIZED), /* RuleEngineStatus coreFuncDescIndexStatus */
    appFuncDescIndexStatus(UNINITIALIZED), /* RuleEngineStatus appFuncDescIndexStatus */
    extFuncDescIndexStatus(UNINITIALIZED), /* RuleEngineStatus extFuncDescIndexStatus */
    ruleEngineStatus(UNINITIALIZED), /* RuleEngineStatus ruleEngineStatus */
    cacheStatus(UNINITIALIZED), /* RuleEngineStatus cacheStatus */
    sysRegionStatus(UNINITIALIZED), /* RuleEngineStatus sysRegionStatus */
    coreRegionStatus(UNINITIALIZED), /* RuleEngineStatus regionCoreStatus */
    appRegionStatus(UNINITIALIZED), /* RuleEngineStatus regionAppStatus */
    extRegionStatus(UNINITIALIZED), /* RuleEngineStatus regionExtStatus */
    coreRuleSet(NULL), /* RuleSet *coreRuleSet */
    appRuleSet(NULL), /* RuleSet *appRuleSet */
    extRuleSet(NULL), /* RuleSet *extRuleSet */
    sysFuncDescIndex(NULL), /* Env *sysFuncDescIndex */
    coreFuncDescIndex(NULL), /* Env *coreFuncDescIndex */
    appFuncDescIndex(NULL), /* Env *appFuncDescIndex */
    extFuncDescIndex(NULL), /* Env *extFuncDescIndex */
    sysRegion(NULL), /* Region *sysRegion */
    coreRegion(NULL), /* Region *coreRegion */
    appRegion(NULL), /* Region *appRegion */
    extRegion(NULL), /* Region *extRegion */
    tvarNumber(0), /* int tvarNumber */
    clearDelayed(0), /* int clearDelayed */
    timestamp(time_type_initializer), /* time_type timestamp */
    logging(0), /* int logging */
    ruleBase(""), /* char ruleBase[RULE_SET_DEF_LENGTH] */
    hash("") /* char *hash */{}

Cache ruleEngineConfig;

void clearRuleEngineConfig() {
  int resources = 0xf00;
  clearRegion (APP, app);
  clearRegion (CORE, core);
  clearRegion (SYS, sys);
  clearRegion (EXT, ext);
  free(ruleEngineConfig.address);
  memset (&ruleEngineConfig, 0, sizeof(Cache));
}

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
    if( !extEnv ) {
        return 0;
    }

    while ( extEnv->previous != ruleEngineConfig.appFuncDescIndex ) {
        extEnv = extEnv->previous;
    }
    extEnv->previous = NULL;
    ruleEngineConfig.appFuncDescIndex->previous = NULL;
    ruleEngineConfig.coreFuncDescIndex->previous = NULL;
    return 0;
}
int clearCoreRuleIndex( ) {
        clearResources( RESC_CORE_FUNC_DESC_INDEX );
    return 0;
}
int clearAppRuleIndex( ) {
        clearResources( RESC_APP_FUNC_DESC_INDEX );
    return 0;
}


int createCoreRuleIndex( ) {
        createRuleNodeIndex( ruleEngineConfig.coreRuleSet, ruleEngineConfig.coreFuncDescIndex->current, CORE_RULE_INDEX_OFF, ruleEngineConfig.coreRegion );
        createCondIndex( ruleEngineConfig.coreRegion );
    return 0;
    }
int createAppRuleIndex( ) {
        createRuleNodeIndex( ruleEngineConfig.appRuleSet, ruleEngineConfig.appFuncDescIndex->current, APP_RULE_INDEX_OFF, ruleEngineConfig.appRegion );
    return 0;

}

int clearCoreRule();


std::string get_rule_base_path( const std::string &irb) {
    char fn[NAME_LEN];
    if (getRuleBasePath( irb.c_str(), fn )==nullptr) {
        return "rule base name is too long";
    }
    return std::string(fn);
}

std::string get_rule_base_path_copy( const std::string &irb, const int pid) {
    return get_rule_base_path(irb) + "." + std::to_string(pid);
}

std::vector<std::string> parse_irbSet(const std::string &irbSet) {
    std::vector<std::string> irbs;
    char r1[NAME_LEN], r2[RULE_SET_DEF_LENGTH], r3[RULE_SET_DEF_LENGTH];
    snprintf( r2, sizeof( r2 ), "%s", irbSet.c_str() );

    while ( strlen( r2 ) > 0 ) {
        rSplitStr( r2, r1, NAME_LEN, r3, RULE_SET_DEF_LENGTH, ',' );
        irbs.push_back(r1);
        snprintf( r2, sizeof( r2 ), "%s", r3 );
    }
    return irbs;
}

int hash_rules(const std::vector<std::string> &irbs, const int pid, std::string &digest) {
    int status;
    irods::Hasher hasher;
    irods::error ret = irods::getHasher(
                           "md5",
                           hasher );
    if ( !ret.ok() ) {
        status = ret.code();
        rodsLogError(
                LOG_ERROR,
                status,
                "hash_rules: cannot get hasher, status = %d",
                status );
        return status;

    }

    for (auto const &irb : irbs) {
        auto fn = get_rule_base_path_copy( irb, pid );
        std::ifstream in_file(
            fn,
            std::ios::in | std::ios::binary );
        if ( !in_file.is_open() ) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLogError(
                LOG_ERROR,
                status,
                "hash_rules: fopen failed for %s, status = %d",
                fn.c_str(),
                status );
            return status;
        }

        std::string buffer_read;
        constexpr auto HASH_BUF_SZ = 1024 * 1024;
        buffer_read.resize( HASH_BUF_SZ );

        while ( in_file.read( &buffer_read[0], HASH_BUF_SZ ) ) {
            hasher.update( buffer_read );
        }

        if ( in_file.eof() ) {
            if ( in_file.gcount() > 0 ) {
                buffer_read.resize( in_file.gcount() );
                hasher.update( buffer_read );
            }
        } else {
            status = UNIX_FILE_READ_ERR - errno;
            rodsLogError(
                     LOG_ERROR,
                     status,
                     "hash_rules: read failed for %s, status = %d",
                     fn.c_str(),
                     status );
            return status;
        }
    }

    hasher.digest( digest );

    return 0;
}

class in_memory_rulebases
{
  public:
    explicit in_memory_rulebases(const std::vector<std::string>& _irods_rule_bases)
    {
        for (auto const& irb : _irods_rule_bases) {
            std::ifstream ifs(get_rule_base_path(irb));
            if (!ifs) {
                log_re::error("in_memory_rulebases: input file stream [{}] failed", get_rule_base_path(irb));
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(RULES_FILE_READ_ERROR, fmt::format("Failed to load rulebase [{}] into memory", irb));
            }
            rule_bases_.insert(
                {irb, std::string({std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()})});
        }
    }

    const std::string& get_rulebase(const std::string& irb)
    {
        return rule_bases_.at(irb);
    }

  private:
    std::unordered_map<std::string, std::string> rule_bases_;
};

int hash_rules_with_copy(const std::vector<std::string>& irods_rule_bases,
                         std::string& digest,
                         in_memory_rulebases& copy)
{
    irods::Hasher hasher;
    irods::error ret = irods::getHasher("md5", hasher);
    if (!ret.ok()) {
        int status = static_cast<int>(ret.code());
        log_re::error("hash_rules_with_copy: cannot get hasher, status = {}", status);
        return status;
    }

    for (auto const& irb : irods_rule_bases) {
        try {
            hasher.update(copy.get_rulebase(irb));
        }
        catch (std::out_of_range& ex) {
            log_re::warn("hash_rules_with_copy: attempted to access nonexistent rulebase [{}]", irb);
        }
    }

    hasher.digest(digest);

    log_re::debug("hash_rules_with_copy: rulebases = [{}]; digest = [{}]", fmt::join(irods_rule_bases, ", "), digest);

    return 0;
}

int load_rules(const char* irbSet, const std::vector<std::string> &irbs, const int pid, const time_type timestamp) {
                generateRegions();
                generateRuleSets();
                generateFunctionDescriptionTables();
                if ( ruleEngineConfig.ruleEngineStatus == UNINITIALIZED ) {
                    getSystemFunctions( ruleEngineConfig.sysFuncDescIndex->current, ruleEngineConfig.sysRegion );
                }

                try {
                    in_memory_rulebases stored_rulebases(irbs);
                    for (auto const& irb : irbs) {
                        try {
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                            auto* rule_ptr = const_cast<char*>(&stored_rulebases.get_rulebase(irb)[0]);
                            int i = readRuleStructAndRuleSetFromBuffer(irb.c_str(), rule_ptr);

                            if (i != 0) {
                                ruleEngineConfig.ruleEngineStatus = INITIALIZED;
                                return i;
                            }
                        }
                        catch (std::out_of_range& ex) {
                            log_re::warn("{}: attempted to access nonexistent rulebase [{}]", __func__, irb);
                        }
                    }

                    createCoreRuleIndex();

                    /* set max timestamp */
                    time_type_set(ruleEngineConfig.timestamp, timestamp);
                    std::string hash;
                    int ret = hash_rules_with_copy(irbs, hash, stored_rulebases);
                    if (ret >= 0) {
                        rstrcpy(static_cast<char*>(ruleEngineConfig.hash), hash.c_str(), CHKSUM_LEN);
                    }
                    else {
                        ruleEngineConfig.ruleEngineStatus = INITIALIZED;
                        return ret;
                    }
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                    snprintf(
                        static_cast<char*>(ruleEngineConfig.ruleBase), sizeof(ruleEngineConfig.ruleBase), "%s", irbSet);
                    ruleEngineConfig.ruleEngineStatus = INITIALIZED;
                }
                catch (const irods::exception& e) {
                    log_re::error(
                        "{} encountered an exception in source file {}:{}", __PRETTY_FUNCTION__, __FILE__, e.what());
                    return static_cast<int>(e.code());
                }
                return 0;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int loadRuleFromCacheOrFile( const char* inst_name, const char *irbSet ) {
    int res = 0;
    auto irbs = parse_irbSet(irbSet);

    clearRuleEngineConfig();

    /* get max timestamp */
    time_type timestamp = time_type_initializer;
    for (auto const& irb : irbs) {
        time_type mtim;
        auto fn = get_rule_base_path( irb );
        if ( ( res = getModifiedTime( fn.c_str(), &mtim ) ) != 0 ) {
            return res;
        }

        if ( time_type_gt(mtim, timestamp) ) {
            timestamp = mtim;
        }
    }

    auto pid = getpid();

    int update = 0;
    unsigned char *buf = NULL;
    /* try to find shared memory cache */
        mutex_type *mutex;
        lockReadMutex(inst_name, &mutex);
        int cmp = 0;
        buf = prepareNonServerSharedMemory( inst_name );
        if ( buf != NULL ) {
            cmp = strcmp((char*)buf, "UNINITIALIZED");
        } else {
            rodsLog( LOG_DEBUG, "Cannot open shared memory." );
        }
        unlockReadMutex(inst_name, &mutex);
        if ( cmp == 0 ) {

                int ret = load_rules(irbSet, irbs, pid, timestamp);
                if ( ret != 0 ) {
                    return ret;
                }

                ret = updateCache( inst_name, SHMMAX, &ruleEngineConfig);
                return ret;

        } else {
                Cache * cache = restoreCache( inst_name );

                if ( cache == NULL ) {
                    rodsLog( LOG_ERROR, "Failed to restore cache." );
                } else {
                    int diffIrbSet = strcmp( cache->ruleBase, irbSet ) != 0;
                    try {
                        in_memory_rulebases stored_rulebases(irbs);
                        std::string hash;
                        int ret = hash_rules_with_copy(irbs, hash, stored_rulebases);

                        int diffHash = static_cast<int>(ret < 0 || hash != static_cast<char*>(cache->hash));
                        if (static_cast<bool>(diffIrbSet)) {
                            log_re::debug("Rule base set changed, old value is {}", cache->ruleBase);
                        }

                        if (static_cast<bool>(diffIrbSet) || time_type_gt(timestamp, cache->timestamp) ||
                            static_cast<bool>(diffHash)) {
                            update = 1;
                            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
                            free(cache->address);
                            log_re::debug("Rule base set or rule files modified, force refresh.");
                        }
                        else {
                            cache->cacheStatus = INITIALIZED;
                            ruleEngineConfig = *cache;

                            /* generate extRuleSet */
                            generateRegions();
                            generateRuleSets();
                            generateFunctionDescriptionTables();
                            if (ruleEngineConfig.ruleEngineStatus == UNINITIALIZED) {
                                getSystemFunctions(
                                    ruleEngineConfig.sysFuncDescIndex->current, ruleEngineConfig.sysRegion);
                            }

                            ruleEngineConfig.ruleEngineStatus = INITIALIZED;

                            return res;
                        }
                    }
                    catch (const irods::exception& e) {
                        log_re::error("{} encountered an exception in file {}:{}",
                                      __PRETTY_FUNCTION__,
                                      __FILE__,
                                      e.client_display_what());
                        return static_cast<int>(e.code());
                    }
                }
        }

    if ( ruleEngineConfig.ruleEngineStatus == INITIALIZED ) {
        /* Reloading rule set, clear previously generated rule set */
        unlinkFuncDescIndex();
        clearCoreRuleIndex();
    }

    int ret = load_rules(irbSet, irbs, pid, timestamp);
    if ( ret != 0 ) {
        return ret;
    }

    if ( update ) {
        ret = updateCache( inst_name, SHMMAX, &ruleEngineConfig );
        if ( ret != 0 ) {
            res = ret;
        }
    }

    return res;
}
int readRuleStructAndRuleSetFromFile( const char *ruleBaseName, const char *rulesBaseFile ) {
    int errloc = 0;
    rError_t errmsgBuf;
    errmsgBuf.errMsg = NULL;
    errmsgBuf.len = 0;

    char *buf = ( char * ) malloc( ERR_MSG_LEN * 1024 * sizeof( char ) );
    int res = 0;
        if ( ( res = readRuleSetFromLocalFile( ruleBaseName, rulesBaseFile, ruleEngineConfig.coreRuleSet, ruleEngineConfig.coreFuncDescIndex, &errloc, &errmsgBuf, ruleEngineConfig.coreRegion ) ) == 0 ) {
        }
        else {
            errMsgToString( &errmsgBuf, buf, ERR_MSG_LEN * 1024 );
            rodsLog( LOG_ERROR, "%s", buf );
        }
    free( buf );
    freeRErrorContent( &errmsgBuf );
    return res;
}

int readRuleStructAndRuleSetFromBuffer(const char* ruleBaseName, char* ruleBase)
{
    int errloc = 0;
    rError_t errmsgBuf;
    errmsgBuf.errMsg = nullptr;
    errmsgBuf.len = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    auto buf = std::make_unique<char[]>(err_buf_len * sizeof(char));
    int res = readRuleSetFromBuffer(ruleBaseName,
                                    ruleBase,
                                    ruleEngineConfig.coreRuleSet,
                                    ruleEngineConfig.coreFuncDescIndex,
                                    &errloc,
                                    &errmsgBuf,
                                    ruleEngineConfig.coreRegion);
    if (res != 0) {
        errMsgToString(&errmsgBuf, buf.get(), err_buf_len);
        log_re::error(buf.get());
    }
    freeRErrorContent(&errmsgBuf);
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
