/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "irods_hashtable.h"
#include <cstdlib>
//#include "utils.hpp"
/**
 * Allocated dynamically
 * returns NULL if out of memory
 */

static char *cpStringExtForHashTable( const char *str, Region *r ) {
    char *strCp = ( char * )region_alloc( r, ( strlen( str ) + 1 ) * sizeof( char ) );
    strcpy( strCp, str );
    return strCp;
}


struct bucket *newBucket( const char* key, const void* value ) {
    struct bucket *b = ( struct bucket * )malloc( sizeof( struct bucket ) );
    if ( b == nullptr ) {
        return nullptr;
    }
    b->next = nullptr;
    b->key = strdup( key );
    b->value = value;
    return b;
}

/**
 * Allocated in regions
 */
struct bucket *newBucket2( char* key, const void* value, Region *r ) {
    struct bucket *b = ( struct bucket * )region_alloc( r, sizeof( struct bucket ) );
    if ( b == nullptr ) {
        return nullptr;
    }
    b->next = nullptr;
    b->key = key;
    b->value = value;
    return b;
}

/**
 * returns NULL if out of memory
 */
Hashtable *newHashTable( int size ) {
    Hashtable *h = ( Hashtable * )malloc( sizeof( Hashtable ) );
    if ( nullptr == h ) { // JMC cppcheck - nullptr ref
        return nullptr;
    }
    memset( h, 0, sizeof( Hashtable ) );

//	if(h==NULL) {
//		return NULL;
//	}
    h->dynamic = 0;
    h->bucketRegion = nullptr;
    h->size = size;
    h->buckets = ( struct bucket ** )malloc( sizeof( struct bucket * ) * size );
    if ( h->buckets == nullptr ) {
        free( h );
        return nullptr;
    }
    memset( h->buckets, 0, sizeof( struct bucket * )*size );
    h->len = 0;
    return h;
}
/**
 * hashtable with dynamic expansion
 * returns NULL if out of memory
 */
Hashtable *newHashTable2( int size, Region *r ) {

    Hashtable *h = ( Hashtable * )region_alloc( r, sizeof( Hashtable ) );
    if ( nullptr == h ) { // JMC cppcheck - nullptr ref
        return nullptr;
    }
    memset( h, 0, sizeof( Hashtable ) );

//	if(h==NULL) {
//		return NULL;
//	}
    h->dynamic = 1;
    h->bucketRegion = r;
    h->size = size;
    h->buckets = ( struct bucket ** )region_alloc( h->bucketRegion, sizeof( struct bucket * ) * size );
    if ( h->buckets == nullptr ) {
        return nullptr;
    }
    memset( h->buckets, 0, sizeof( struct bucket * )*size );
    h->len = 0;
    return h;
}
/**
 * the key is duplicated but the value is not duplicated.
 * MM: the key is managed by hashtable while the value is managed by the caller.
 * returns 0 if out of memory
 */
int insertIntoHashTable( Hashtable *h, const char* key, const void *value ) {
    /*
        printf("insert %s=%s\n", key, value==NULL?"null":"<value>");
    */
    if ( h->dynamic ) {
        if ( h->len >= h->size ) {
            Hashtable *h2 = newHashTable2( h->size * 2, h->bucketRegion );
            int i;
            for ( i = 0; i < h->size; i++ ) {
                if ( h->buckets[i] != nullptr ) {
                    struct bucket *b = h->buckets[i];
                    do {
                        insertIntoHashTable( h2, b->key, b->value );
                        b = b->next;

                    }
                    while ( b != nullptr );
                }
            }
            memcpy( h, h2, sizeof( Hashtable ) );
        }
        struct bucket *b = newBucket2( cpStringExtForHashTable( key, h->bucketRegion ), value, h->bucketRegion );
        if ( b == nullptr ) {
            return 0;
        }

        unsigned long hs = myhash( key );
        unsigned long index = hs % h->size;
        if ( h->buckets[index] == nullptr ) {
            h->buckets[index] = b;
        }
        else {
            struct bucket *b0 = h->buckets[index];
            while ( b0->next != nullptr ) {
                b0 = b0->next;
            }
            b0->next = b;
        }
        h->len ++;
        return 1;
    }
    else {
        struct bucket *b = newBucket( key, value );
        if ( b == nullptr ) {
            return 0;
        }
        unsigned long hs = myhash( key );
        unsigned long index = hs % h->size;
        if ( h->buckets[index] == nullptr ) {
            h->buckets[index] = b;
        }
        else {
            struct bucket *b0 = h->buckets[index];
            while ( b0->next != nullptr ) {
                b0 = b0->next;
            }
            b0->next = b;
        }
        h->len ++;
        return 1;
    }
}
/**
 * update hash table returns the pointer to the old value
 */
const void* updateInHashTable( Hashtable *h, const char* key, const void* value ) {
    unsigned long hs = myhash( key );
    unsigned long index = hs % h->size;
    if ( h->buckets[index] != nullptr ) {
        struct bucket *b0 = h->buckets[index];
        while ( b0 != nullptr ) {
            if ( strcmp( b0->key, key ) == 0 ) {
                const void* tmp = b0->value;
                b0->value = value;
                return tmp;
                /* do not free the value */
            }
            b0 = b0->next;
        }
    }
    return nullptr;
}
/**
 * delete from hash table
 */
const void *deleteFromHashTable( Hashtable *h, const char* key ) {
    unsigned long hs = myhash( key );
    unsigned long index = hs % h->size;
    const void *temp = nullptr;
    if ( h->buckets[index] != nullptr ) {
        struct bucket *b0 = h->buckets[index];
        if ( strcmp( b0->key, key ) == 0 ) {
            h->buckets[index] = b0->next;
            temp = b0->value;
            if ( !h->dynamic ) {
                free( b0->key );
                free( b0 );
            }
            h->len --;
        }
        else {
            while ( b0->next != nullptr ) {
                if ( strcmp( b0->next->key, key ) == 0 ) {
                    struct bucket *tempBucket = b0->next;
                    temp = b0->next->value;
                    b0->next = b0->next->next;
                    if ( !h->dynamic ) {
                        free( tempBucket->key );
                        free( tempBucket );
                    }
                    h->len --;
                    break;
                }
                b0 = b0->next; // JMC - backport 4799
            }
        }
    }

    return temp;
}
/**
 * returns NULL if not found
 */
const void* lookupFromHashTable( Hashtable *h, const char* key ) {
    unsigned long hs = myhash( key );
    unsigned long index = hs % h->size;
    struct bucket *b0 = h->buckets[index];
    while ( b0 != nullptr ) {
        if ( strcmp( b0->key, key ) == 0 ) {
            return b0->value;
        }
        b0 = b0->next;
    }
    return nullptr;
}
/**
 * returns NULL if not found
 */
struct bucket* lookupBucketFromHashTable( Hashtable *h, const char* key ) {
    unsigned long hs = myhash( key );
    unsigned long index = hs % h->size;
    struct bucket *b0 = h->buckets[index];
    while ( b0 != nullptr ) {
        if ( strcmp( b0->key, key ) == 0 ) {
            return b0;
        }
        b0 = b0->next;
    }
    return nullptr;
}
struct bucket* nextBucket( struct bucket *b0, const char* key ) {
    b0 = b0->next;
    while ( b0 != nullptr ) {
        if ( strcmp( b0->key, key ) == 0 ) {
            return b0;
        }
        b0 = b0->next;
    }
    return nullptr;
}

void deleteHashTable( Hashtable *h, void ( *f )( const void * ) ) {
    if ( !h->dynamic ) {
        int i;
        for ( i = 0; i < h->size; i++ ) {
            struct bucket *b0 = h->buckets[i];
            if ( b0 != nullptr ) {
                deleteBucket( b0, f );
            }
        }
        free( h->buckets );
        free( h );
    }
}
void deleteBucket( struct bucket *b0, void ( *f )( const void * ) ) {
    if ( b0->next != nullptr ) {
        deleteBucket( b0->next, f );
    }
    /* todo do not delete keys if they are allocated in regions */
    free( b0->key );
    if ( f != nullptr ) {
        f( b0->value );
    }
    free( b0 );
}
void nop( const void* ) {
}

void free_const( const void *a ) {
    free( const_cast< void * >( a ) );
}



unsigned long B_hash( unsigned char* string ) { /* Bernstein hash */
    unsigned long hash = HASH_BASE;
    while ( *string != '\0' ) {
        hash = ( ( hash << 5 ) + hash ) + ( int ) * string;
        string++;
    }
    return hash;

}
unsigned long sdbm_hash( char* str ) { /* sdbm */
    unsigned long hash = 0;
    while ( *str != '\0' ) {
        hash = ( ( int ) * str ) + ( hash << 6 ) + ( hash << 16 ) - hash;
        str++;
    }

    return hash;
}

/*
#include "utils.hpp"
#include "index.hpp"

void dumpHashtableKeys( Hashtable *t ) {
    int i;
    for ( i = 0; i < t->size; i++ ) {
        struct bucket *b = t->buckets[i];
        while ( b != NULL ) {
            writeToTmp( "htdump", b->key );
            writeToTmp( "htdump", "\n" );
            b = b->next;
        }
    }
}
*/
