/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "region.hpp"
#define HASH_BASE 5381
#define myhash(x) B_hash((unsigned char*)(x))

struct bucket {
    char* key;
    const void* value;
    struct bucket *next;
};
typedef struct hashtable {
    struct bucket **buckets;
    int size; /* capacity */
    int len;
    int dynamic;
    Region *bucketRegion;
} Hashtable;

struct bucket *newBucket( const char* key, const void* value );
unsigned long B_hash( unsigned char* string );
Hashtable *newHashTable( int size );
Hashtable *newHashTable2( int size, Region *r );
int insertIntoHashTable( Hashtable *h, const char* key, const void *value );
const void* updateInHashTable( Hashtable *h, const char* key, const void *value );
const void* deleteFromHashTable( Hashtable *h, const char* key );
const void* lookupFromHashTable( Hashtable *h, const char* key );
void deleteHashTable( Hashtable *h, void ( *f )( const void * ) );
void deleteBucket( struct bucket *h, void ( *f )( const void * ) );
struct bucket* lookupBucketFromHashTable( Hashtable *h, const char* key );
struct bucket* nextBucket( struct bucket *h, const char* key );
void nop( const void *a );
void free_const( const void *a );
#endif
