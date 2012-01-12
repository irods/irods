/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef TO_REGION_INSTANCE_H
#define TO_REGION_INSTANCE_H

#include "to.region.proto.h"
#include "traversal.instance.h"


#define TRAVERSE_BEGIN(T) \
	TRAVERSE_CYCLIC(T, key, objectMap); \
	T* ptr1; \
	CASCADE_NULL(ptr1 = (T*) region_alloc(r, sizeof(T))); \
	insertIntoHashTable(objectMap, key, ptr1); \
	memcpy(ptr1, ptr, sizeof(T)); \
	ptr = ptr1; \

#define TRAVERSE_END(T) \
	return ptr;

#define TRAVERSE_PTR(fn, f) \
	CASCADE_NULL(ptr->f = fn(ptr->f, r, objectMap));

#define TRAVERSE_PTR_GENERIC(fn, f, cpfn) \
	CASCADE_NULL(ptr->f = fn(ptr->f, r, cpfn, objectMap));

#define TRAVERSE_ARRAY_BEGIN(T, size, f) \
	TRAVERSE_ARRAY_CYCLIC(T, size, f, ptr->f, key0, objectMap) { \
		T *oldf = ptr->f; \
		CASCADE_NULL(ptr->f = (T *) region_alloc(r, sizeof(T) * size)); \
		memcpy(ptr->f, oldf, sizeof(T) * size); \
		insertIntoHashTable(objectMap, key0, ptr->f); \

#define TRAVERSE_ARRAY_END(T, size, f) \
	} \

#define GET_VAR_ARRAY_LEN(T, len, f) \
	  T* l = ptr->f; \
	  while(*l != (T) 0) { \
		  l++; \
	  } \
	  int len = l - ptr->f + 1; \

#endif
