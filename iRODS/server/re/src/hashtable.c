/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "hashtable.h"
#include "utils.h"
/**
 * returns NULL if out of memory
 */
struct bucket *newBucket(char* key, void* value) {
	struct bucket *b = (struct bucket *)malloc(sizeof(struct bucket));
	if(b==NULL) {
		return NULL;
	}
	b->next = NULL;
	b->key = key;
	b->value = value;
	return b;
}

struct bucket *newBucket2(char* key, void* value, Region *r) {
	struct bucket *b = (struct bucket *)region_alloc(r, sizeof(struct bucket));
	if(b==NULL) {
		return NULL;
	}
	b->next = NULL;
	b->key = key;
	b->value = value;
	return b;
}

/**
 * returns NULL if out of memory
 */
Hashtable *newHashTable(int size) {
	Hashtable *h = (Hashtable *)malloc(sizeof (Hashtable));
	memset(h, 0, sizeof(Hashtable));

	if(h==NULL) {
		return NULL;
	}
	h->dynamic = 0;
	h->bucketRegion = NULL;
	h->size = size;
	h->buckets = (struct bucket **)malloc(sizeof(struct bucket *)*size);
	if(h->buckets == NULL) {
		free(h);
		return NULL;
	}
	memset(h->buckets, 0, sizeof(struct bucket *)*size);
	h->len = 0;
	return h;
}
/**
 * hashtable with dynamic expansion
 * returns NULL if out of memory
 */
Hashtable *newHashTable2(int size, Region *r) {

	Hashtable *h = (Hashtable *)region_alloc(r, sizeof (Hashtable));
	memset(h, 0, sizeof(Hashtable));

	if(h==NULL) {
		return NULL;
	}
	h->dynamic = 1;
	h->bucketRegion = r;
	h->size = size;
	h->buckets = (struct bucket **)region_alloc(h->bucketRegion, sizeof(struct bucket *)*size);
	if(h->buckets == NULL) {
		return NULL;
	}
	memset(h->buckets, 0, sizeof(struct bucket *)*size);
	h->len = 0;
	return h;
}
/**
 * the key is duplicated but the value is not duplicated.
 * MM: the key is managed by hashtable while the value is managed by the caller.
 * returns 0 if out of memory
 */
int insertIntoHashTable(Hashtable *h, char* key, void *value) {
/*
    printf("insert %s=%s\n", key, value==NULL?"null":"<value>");
*/
	if(h->dynamic) {
		if(h->len >= h->size) {
			Hashtable *h2 = newHashTable2(h->size * 2, h->bucketRegion);
			int i;
			for(i=0;i<h->size;i++) {
				if(h->buckets[i]!=NULL) {
					struct bucket *b = h->buckets[i];
					do {
						insertIntoHashTable(h2, b->key, b->value);
						b=b->next;

					} while (b!=NULL);
				}
			}
			memcpy(h, h2, sizeof(Hashtable));
		}
		struct bucket *b = newBucket2(cpStringExt(key, h->bucketRegion), value, h->bucketRegion);
		if(b==NULL) {
			return 0;
		}

		unsigned long hs = myhash(key);
		unsigned long index = hs%h->size;
		if(h->buckets[index] == NULL) {
			h->buckets[index] = b;
		} else {
			struct bucket *b0 = h->buckets[index];
			while(b0->next!=NULL)
				b0=b0->next;
			b0->next = b;
		}
		h->len ++;
		return 1;
	} else {
		struct bucket *b = newBucket(strdup(key), value);
		if(b==NULL) {
			return 0;
		}
		unsigned long hs = myhash(key);
		unsigned long index = hs%h->size;
		if(h->buckets[index] == NULL) {
			h->buckets[index] = b;
		} else {
			struct bucket *b0 = h->buckets[index];
			while(b0->next!=NULL)
				b0=b0->next;
			b0->next = b;
		}
		h->len ++;
		return 1;
	}
}
/**
 * update hash table returns the pointer to the old value
 */
void* updateInHashTable(Hashtable *h, char* key, void* value) {
	unsigned long hs = myhash(key);
	unsigned long index = hs%h->size;
	if(h->buckets[index] != NULL) {
		struct bucket *b0 = h->buckets[index];
		while(b0!=NULL) {
			if(strcmp(b0->key, key) == 0) {
				void* tmp = b0->value;
				b0->value = value;
				return tmp;
				/* free not the value */
			}
                        b0=b0->next;
		}
	}
	return NULL;
}
/**
 * delete from hash table
 */
void *deleteFromHashTable(Hashtable *h, char* key) {
	unsigned long hs = myhash(key);
	unsigned long index = hs%h->size;
	void *temp = NULL;
	if(h->buckets[index] != NULL) {
            struct bucket *b0 = h->buckets[index];
            if(strcmp(b0->key, key) == 0) {
                    h->buckets[index] = b0->next;
                    temp = b0->value;
                	if(!h->dynamic) {
                    free(b0->key);
                    free(b0);
                	}
                    h->len --;
            } else {
                while(b0->next!=NULL) {
                    if(strcmp(b0->next->key, key) == 0) {
                        struct bucket *tempBucket = b0->next;
                        temp = b0->next->value;
                        b0->next = b0->next->next;
                    	if(!h->dynamic) {
                        free(tempBucket->key);
                        free(tempBucket);
                    	}
                        h->len --;
                        break;
                    }
                }
            }
	}

	return temp;
}
/**
 * returns NULL if not found
 */
void* lookupFromHashTable(Hashtable *h, char* key) {
	unsigned long hs = myhash(key);
	unsigned long index = hs%h->size;
	struct bucket *b0 = h->buckets[index];
	while(b0!=NULL) {
		if(strcmp(b0->key,key)==0) {
			return b0->value;
		}
		b0=b0->next;
	}
	return NULL;
}
/**
 * returns NULL if not found
 */
struct bucket* lookupBucketFromHashTable(Hashtable *h, char* key) {
	unsigned long hs = myhash(key);
	unsigned long index = hs%h->size;
	struct bucket *b0 = h->buckets[index];
	while(b0!=NULL) {
		if(strcmp(b0->key,key)==0) {
			return b0;
		}
		b0=b0->next;
	}
	return NULL;
}
struct bucket* nextBucket(struct bucket *b0, char* key) {
    b0 = b0->next;
    while(b0!=NULL) {
        if(strcmp(b0->key,key)==0) {
            return b0;
        }
        b0=b0->next;
    }
    return NULL;
}

void deleteHashTable(Hashtable *h, void (*f)(void *)) {
	if(h->dynamic) {
		/*if(f != NULL) {
			int i;
			for(i =0;i<h->size;i++) {
				struct bucket *b0 = h->buckets[i];
				while(b0!=NULL) {
					f(b0->value);
					b0= b0->next;
				}
			}

		}*/
	} else {
		int i;
		for(i =0;i<h->size;i++) {
			struct bucket *b0 = h->buckets[i];
			if(b0!=NULL)
				deleteBucket(b0,f);
		}
			free(h->buckets);
			free(h);
	}
}
void deleteBucket(struct bucket *b0, void (*f)(void *)) {
		if(b0->next!=NULL) {
			deleteBucket(b0->next, f);
		}
                /* todo do not delete keys if they are allocated in regions */
                free(b0->key);
		if(f!=NULL) f(b0->value);
		free(b0);
}
void nop(void *a) {
}




unsigned long B_hash(unsigned char* string) { /* Bernstein hash */
	unsigned long hash = HASH_BASE;
	while(*string!='\0') {
	    hash = ((hash << 5) + hash) + (int)*string;
        string++;
	}
	return hash;

}
unsigned long sdbm_hash(char* str) { /* sdbm */
        unsigned long hash = 0;
        while (*str!='\0') {
            hash = ((int)*str) + (hash << 6) + (hash << 16) - hash;
			str++;
		}

        return hash;
}

#include "utils.h"
#include "index.h"

void dumpHashtableKeys(Hashtable *t) {
    int i;
    for(i = 0;i<t->size;i++) {
        struct bucket *b=t->buckets[i];
        while(b!=NULL) {
            writeToTmp("htdump", b->key);
            writeToTmp("htdump", "\n");
            b= b->next;
        }
    }
}

