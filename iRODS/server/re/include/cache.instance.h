/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef CACHE_INSTANCE_H
#define CACHE_INSTANCE_H

#include "cache.proto.h"


#define allocateInBuffer(ty, vn, val) \
    ty *vn = ((ty *)(*p)); \
    memcpy(vn, val, sizeof(ty)); \
    (*p)+=CACHE_SIZE(ty, 1); \
    if(*p > *pointers) return NULL;

#define allocateArrayInBuffer(elemTy, n, lval, val) \
    lval = ((elemTy *)(*p)); \
    memcpy(lval, val, CACHE_SIZE(elemTy, n)); \
    (*p)+=CACHE_SIZE(elemTy, n); \
    if(*p > *pointers) return NULL;

#define MK_POINTER(ptr) \
	if(generatePtrDesc) { \
		(*pointers)-=CACHE_SIZE(unsigned char *, 1); \
		*(unsigned char **)(*pointers) = (unsigned char *)(ptr); \
	} \
	if(*p > *pointers) return NULL;

#define MK_TRANSIENT_PTR(type, f) \
		ecopy->f = NULL;

#define MK_TRANSIENT_VAL(type, f, val) \
		ecopy->f = val;

#define TRAVERSE_BEGIN(T) \
	TRAVERSE_CYCLIC(T, key, objectMap); \
    allocateInBuffer(T, ecopy, ptr); \
	insertIntoHashTable(objectMap, key, ecopy);

#define TRAVERSE_END(T) \
	return ecopy;

#define TRAVERSE_PTR(fn, f) \
	CASCADE_NULL(ecopy->f=fn(buf, p, pointers, ptr->f, objectMap, generatePtrDesc)); \
	MK_POINTER(&(ecopy->f));

#define TRAVERSE_PTR_GENERIC(fn, f, cpfn) \
	CASCADE_NULL(ecopy->f=fn(buf, p, pointers, ptr->f, cpfn, objectMap, generatePtrDesc)); \
	MK_POINTER(&(ecopy->f));

#define TRAVERSE_ARRAY_BEGIN(type, size, f) \
	TRAVERSE_ARRAY_CYCLIC(type, size, f, ecopy->f, key0, objectMap) { \
		allocateArrayInBuffer(type, size, ecopy->f, ptr->f); \
		insertIntoHashTable(objectMap, key0, ecopy->f);

#define TRAVERSE_ARRAY_END(type, size, f) \
	} \
	MK_POINTER(&(ecopy->f)); \

#define GET_VAR_ARRAY_LEN(type, size, f) \
	type* l = ptr->f; \
	while(*l != (type) 0) { \
		l++; \
	} \
	int size = l - ptr->f+1;

/*#define COPY_FUNC_PROTO(T) \
		T* copy##T(unsigned char *buf, unsigned char **p, unsigned char **pointers, T *e, int generatePtrDesc)*/

/*#define COPY_FUNC_BEGIN(T) \
		COPY_FUNC_NO_COPY_BEGIN(T) \
		allocateInBuffer(T, ecopy, e);*/

/*#define COPY_FUNC_NO_COPY_BEGIN(T) \
		COPY_FUNC_PROTO(T) {*/

#endif
