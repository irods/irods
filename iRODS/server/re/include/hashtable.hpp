/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "region.h"
#define HASH_BASE 5381
#define myhash(x) B_hash((unsigned char*)(x))

struct bucket {
	char* key;
	void* value;
	struct bucket *next;
};
typedef struct hashtable {
	struct bucket **buckets;
	int size; /* capacity */
	int len;
	int dynamic;
	Region *bucketRegion;
} Hashtable;

struct bucket *newBucket(char* key, void* value);
unsigned long B_hash(unsigned char* string);
Hashtable *newHashTable(int size);
Hashtable *newHashTable2(int size, Region *r);
int insertIntoHashTable(Hashtable *h, char* key, void *value);
void* updateInHashTable(Hashtable *h, char* key, void *value);
void* deleteFromHashTable(Hashtable *h, char* key);
void* lookupFromHashTable(Hashtable *h, char* key);
void deleteHashTable(Hashtable *h, void (*f)(void *));
void deleteBucket(struct bucket *h, void (*f)(void *));
struct bucket* lookupBucketFromHashTable(Hashtable *h, char* key);
struct bucket* nextBucket(struct bucket *h, char* key);
void nop(void *a);
#endif
