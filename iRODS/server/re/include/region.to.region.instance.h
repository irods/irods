/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef REGION_TO_REGION_INSTANCE_H
#define REGION_TO_REGION_INSTANCE_H

#include "region.to.region.proto.h"
#include "traversal.instance.h"

#define TRAVERSE_BEGIN(T) \
	T* ptr1; \
	/* If the old region is not r, it may be a global region. So it is not safe to do in place update. */ \
	if(!IN_REGION(ptr, r)) { \
		ptr1 = (T*) region_alloc(r, sizeof(T)); \
		memcpy(ptr1, ptr, sizeof(T)); \
		ptr = ptr1; \
	} \

#define TRAVERSE_END(T) \
	return ptr;

#define TRAVERSE_PTR(fn, f) \
	CASCADE_NULL(ptr->f = fn(ptr->f, r));

#define TRAVERSE_ARRAY_BEGIN(type, size, f) \
	NO_COPY_IN_REGION(f) { \
		type *oldf = ptr->f; \
		CASCADE_NULL(ptr->f = (type *) region_alloc(r, sizeof(type) * size)); \
		memcpy(ptr->f, oldf, sizeof(type) * size); \
	}

#define TRAVERSE_ARRAY_END(type, size, f)

#define TRAVERSE_PTR_GENERIC(fn, f, cpfn) \
	CASCADE_NULL(ptr->f = fn(ptr->f, r, cpfn));

#define GET_VAR_ARRAY_LEN(type, len, f) \
	  type* l = ptr->f; \
	  while(*l != (type) 0) { \
		  l++; \
	  } \
	  int len = l - ptr->f + 1; \

#define NO_COPY_IN_REGION(f) \
	if(!IN_REGION(ptr->f, r))

/*ChkFuncType *regionChkCopier(CopyFuncType *fn) {
	if(fn == (CopyFuncType *)regionCpCondIndexVal) {
		return (ChkFuncType *) regionChkCondIndexVal;
	} else if(fn==(CopyFuncType *)regionCpNode) {
		return (ChkFuncType *) regionChkNode;
	}
	return NULL;
}*/

#endif
