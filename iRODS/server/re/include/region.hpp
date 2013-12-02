/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef _REGION_H
#define _REGION_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#define DEFAULT_BLOCK_SIZE 1024
/* the alignment in the region in bytes */
#define REGION_ALIGNMENT 8
#define roundToAlignment(x) ((x)%REGION_ALIGNMENT == 0?(x):(((x)/REGION_ALIGNMENT)+1)*REGION_ALIGNMENT)
/* the point the elem i of type t starting from p in the cache, aligned */
#define CACHE_ELEM(t, p, i) (((t)p)+i)
/* the size of an array of type t and length i in the cache, aligned */
#define CACHE_SIZE(t, l) roundToAlignment(sizeof(t)*l)

struct region_error {
    int code;
    char msg[1024];
    void *obj; /* a generic point to an object which is the source of the error */
};

/* #define REGION_MALLOC */
#ifdef REGION_MALLOC
struct region_node {
	size_t size;
	void *ptr;
	struct region_node *next;
};

typedef struct region {
	struct region_node *head, *tail;
} Region;
#else
struct region_node {
	unsigned char *block; /* pointer to memory block */
	size_t size; /* size of the memory block in bytes */
	size_t used; /* used bytes of the memory block */
	struct region_node *next; /* pointer to the next region */

};


typedef struct region {
	struct region_node *head, *active;
        jmp_buf *label;
        struct region_error error;
} Region;
#endif
typedef struct region_desc {
    Region *region;
    size_t size;
    int del;
} RegionDesc;

#define IN_REGION(x,r) (((RegionDesc *)(((unsigned char*)(x))-CACHE_SIZE(RegionDesc, 1)))->region == (r))
#define SET_DELETE(x) (((RegionDesc *)(((unsigned char*)(x))-CACHE_SIZE(RegionDesc, 1)))->del=1)
#define DELETED(x) (((RegionDesc *)(((unsigned char*)(x))-CACHE_SIZE(RegionDesc, 1)))->del)
#define SIZE(x) (((RegionDesc *)(((unsigned char*)(x))-CACHE_SIZE(RegionDesc, 1)))->size)

/* create a region with initial size is */
/* if s == 0 then the initial size is DEFAULT_BLOCK_SIZE */
/* returns NULL if it runs out of memory */
Region *make_region(size_t is, jmp_buf *label);
/* allocation s bytes in region r */
/* return NULL if it runs out of memory */
void *region_alloc(Region *r, size_t s);
/* free region r */
void region_free(Region *r);
size_t region_size(Region *r);
#endif
