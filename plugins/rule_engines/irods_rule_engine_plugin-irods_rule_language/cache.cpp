/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include <limits.h>
#include "cache.hpp"
#include "rules.hpp"
#include "functions.hpp"
#include "locks.hpp"
#include "sharedmemory.hpp"
#include "datetime.hpp"


#include "cache.instance.hpp"
#include "traversal.instance.hpp"
#include "restruct.templates.hpp"

/* Copy the data stored in a Cache struct into a continuously allocated memory block in *p.
 * All sub structure status are either uninitialized or compressed, which meaning that they are not allocated separately and therefore should not be deallocated.
 * Only coreRuleSet and coreFuncDescIndex are copied.
 * Starting from *p+size backwards, a list of pointers to pointers in the copied data structures are stored.
 * There pointers are used to quickly access all pointers in the stored data structures so that they can be offset when they are copied to a new memory address.
 * This function returns NULL if it runs out of memory allocated between in *p and *p+size.
 * The rule engine status is also set to uninitialized. */
Cache *copyCache( unsigned char **p, size_t size, Cache *ptr ) {
    if ( size % REGION_ALIGNMENT != 0 ) { /* size should be divisible by ALIGNMENT */
        return nullptr;
    }

    unsigned char *buf = *p;
    unsigned char *pointers0 = buf + size;

    /* shared objects */
    unsigned char **pointers = &pointers0;
    int generatePtrDesc = 1;

    allocateInBuffer( Cache, ecopy, ptr );

    MK_POINTER( &( ecopy->address ) );
    MK_POINTER( &( ecopy->pointers ) );
    Hashtable *objectMap = newHashTable( 100 );

    MK_PTR( RuleSet, coreRuleSet );
    ecopy->coreRuleSetStatus = COMPRESSED;
    ecopy->appRuleSet = nullptr;
    ecopy->appRuleSetStatus = UNINITIALIZED;
    ecopy->extRuleSet = nullptr;
    ecopy->extRuleSetStatus = UNINITIALIZED;
    MK_PTR_TAPP( Env, coreFuncDescIndex, PARAM( Node ) );
    ecopy->coreFuncDescIndexStatus = COMPRESSED; /* The coreFuncDescIndex is stored in a continuously allocated memory block in *buf */
    ecopy->appFuncDescIndex = nullptr;
    ecopy->appFuncDescIndexStatus = UNINITIALIZED;
    ecopy->extFuncDescIndex = nullptr;
    ecopy->extFuncDescIndexStatus = UNINITIALIZED;
    ecopy->dataSize = ( *p - buf );
    ecopy->address = buf;
    ecopy->pointers = pointers0;
    ecopy->cacheSize = size;
    ecopy->cacheStatus = INITIALIZED;
    ecopy->appRegion = nullptr;
    ecopy->appRegionStatus = UNINITIALIZED;
    ecopy->coreRegion = nullptr;
    ecopy->coreRegionStatus = UNINITIALIZED;
    ecopy->extRegion = nullptr;
    ecopy->extRegionStatus = UNINITIALIZED;
    ecopy->sysRegion = nullptr;
    ecopy->sysRegionStatus = UNINITIALIZED;
    ecopy->sysFuncDescIndex = nullptr;
    ecopy->sysFuncDescIndexStatus = UNINITIALIZED;
    ecopy->ruleEngineStatus = UNINITIALIZED;
    MK_VAR_ARRAY_IN_STRUCT( char, ruleBase );

    deleteHashTable( objectMap, nop );

    return ecopy;
}

/*
 * Restore a Cache struct from buf into a malloc'd buffer.
 * This function returns NULL if failed to acquire or release the mutex.
 * It first create a local copy of buf's data section and pointer pointers section. This part needs synchronization.
 * Then it works on its local copy, which does not need synchronization.
 */
Cache *restoreCache( const char* _inst_name ) {
    mutex_type *mutex;
    lockReadMutex(_inst_name, &mutex);
    unsigned char *buf = prepareNonServerSharedMemory( _inst_name );
    if (buf == nullptr) {
        return nullptr;
    }
    Cache *cache = ( Cache * ) buf;
    unsigned char *bufCopy;
    unsigned char *pointers;
    size_t pointersSize;
    unsigned char *bufMapped;

    unsigned char *pointersCopy;
    unsigned char *pointersMapped;
    size_t dataSize;
    dataSize = cache->dataSize;
    bufCopy = ( unsigned char * )malloc( dataSize );
    if ( bufCopy == nullptr ) {
        rodsLog( LOG_ERROR, "Cannot allocate object buffer of size %lld" , dataSize);
        unlockReadMutex(_inst_name, &mutex);
        return nullptr;
    }
    memcpy( bufCopy, buf, cache->dataSize );
    Cache *cacheCopy = ( Cache * ) bufCopy;
    pointersMapped = cacheCopy->pointers;
    bufMapped = cacheCopy->address;
    pointersSize = bufMapped + SHMMAX - pointersMapped;
    pointersCopy = ( unsigned char * )malloc( pointersSize );
    if ( pointersCopy == nullptr ) {
        free( bufCopy );
        rodsLog( LOG_ERROR, "Cannot allocate pointer pointer buffer of size %lld", pointersSize);
        detachSharedMemory( _inst_name );
        unlockReadMutex(_inst_name, &mutex);
        return nullptr;
    }
    memcpy( pointersCopy, pointersMapped + ( buf - bufMapped ), pointersSize );
    detachSharedMemory( _inst_name );
    unlockReadMutex(_inst_name, &mutex);
    pointers = pointersCopy;
    /*    bufCopy = (unsigned char *)malloc(cache->dataSize);

        if(bufCopy == NULL) {
            return NULL;
        }
        memcpy(bufCopy, buf, cache->dataSize);

        bufMapped = cache->address;

        long diffBuf = buf - bufMapped;
        pointers = cache->pointers + diffBuf;
        pointersSize = pointers - buf; */
    long diff = bufCopy - bufMapped;
    long pointerDiff = diff;
    applyDiff( pointers, pointersSize, diff, pointerDiff );
    free( pointersCopy );

#ifdef RE_CACHE_CHECK
    Hashtable *objectMap = newHashTable( 100 );
    cacheChkEnv( cacheCopy->coreFuncDescIndex, cacheCopy, ( CacheChkFuncType * ) cacheChkNode, objectMap );
    cacheChkRuleSet( cacheCopy->coreRuleSet, cacheCopy, objectMap );
#endif
    return cacheCopy;
}
void applyDiff( unsigned char *pointers, long pointersSize, long diff, long pointerDiff ) {
    unsigned char *p;
#ifdef DEBUG_VERBOSE
    printf( "storing cache from buf = %p, pointers = %p\n", buf, pointers );
#endif
    for ( p = pointers; p - pointers < pointersSize; p += CACHE_SIZE( unsigned char *, 1 ) ) {
#ifdef DEBUG_VERBOSE
        printf( "p = %p\n", p );
#endif
        unsigned char *pointer = *( ( unsigned char ** )p ) + pointerDiff;
#ifdef DEBUG_VERBOSE
        printf( "applying diff to pointer at %p\n", pointer );
#endif
        *( ( unsigned char ** )pointer ) += diff;
    }
}

void applyDiffToPointers( unsigned char *pointers, long pointersSize, long pointerDiff ) {
    unsigned char *p;
#ifdef DEBUG_VERBOSE
    printf( "storing cache from buf = %p, pointers = %p\n", buf, pointers );
#endif
    for ( p = pointers; p - pointers < pointersSize; p += CACHE_SIZE( unsigned char *, 1 ) ) {
#ifdef DEBUG_VERBOSE
        printf( "p = %p\n", p );
#endif
        *( ( unsigned char ** )p ) += pointerDiff;
#ifdef DEBUG_VERBOSE
        printf( "applying diff to pointer at %p\n", pointer );
#endif
    }

}

/*
 * Update shared with a new cache.
 * This function checks
 *     if new cache has a newer timestamp than the shared cache and updates,
 * 		   then the updateTS of the shared cache before it starts to update the cached data.
 * 		   else return.
 * 	   except when the processType is RULE_ENGINE_INIT_CACHE, which means that the share caches has not been initialized.
 * 		   or when the processType is RULE_ENGINE_REFRESH_CACHE, which means that we want to refresh the cache with the new cache.
 * It prepares the data in the new cache for copying into the shared cache.
 * It checks the timestamp again and does the actually copying.
 */
int updateCache( const char* _inst_name, size_t size, Cache *cache ) {
        unsigned char *buf = ( unsigned char * ) malloc( size );
        if ( buf != nullptr ) {
            int ret;
            unsigned char *cacheBuf = buf;
            Cache *cacheCopy = copyCache( &cacheBuf, size, cache );
            if ( cacheCopy != nullptr ) {
#ifdef DEBUG
                printf( "Buffer usage: %fM\n", ( ( double )( cacheCopy->dataSize ) ) / ( 1024 * 1024 ) );
#endif
                size_t pointersSize = ( cacheCopy->address + cacheCopy->cacheSize ) - cacheCopy->pointers;
                mutex_type *mutex;
                lockWriteMutex(_inst_name, &mutex);
                unsigned char *shared = prepareServerSharedMemory( _inst_name );
                if (shared == nullptr) {
                    ret = -1;
                } else {
                    long diff = shared - cacheCopy->address;
                    unsigned char *pointers = cacheCopy->pointers;

                    applyDiff( pointers, pointersSize, diff, 0 );
                    applyDiffToPointers( pointers, pointersSize, diff );

                    memset( shared, 0, SHMMAX );
                    /* copy data */
                    memcpy( shared, buf, cacheCopy->dataSize );
                    /* copy pointers */
                    memcpy( cacheCopy->pointers, pointers, pointersSize );
                    ret = 0;
                    detachSharedMemory( _inst_name );
                }
                unlockWriteMutex(_inst_name, &mutex);
            }
            else {
                rodsLog( LOG_ERROR, "Error updating cache." );
                ret = -1;
            }
            free( buf );
            return ret;
        }
        else {
            rodsLog( LOG_ERROR, "Cannot update cache because of out of memory error, let some other process update it later when memory is available." );
            return -1;
        }

}



