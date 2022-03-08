/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef REGION_TO_REGION2_INSTANCE_HPP
#define REGION_TO_REGION2_INSTANCE_HPP

#include "irods/private/re/region.to.region2.proto.hpp"
#include "irods/private/re/traversal.instance.hpp"

#define TRAVERSE_BEGIN(T) \
	T* ptr1; \
	/* If the old region is not r, it may be a global region. So it is not safe to do in place update. */ \
	if(IN_REGION(ptr, oldr)) { \
		CASCADE_NULL(ptr1 = (T*) region_alloc(r, sizeof(T))); \
		memcpy(ptr1, ptr, sizeof(T)); \
		ptr = ptr1; \
	} \

#define TRAVERSE_END(T) \
	return ptr;

#define TRAVERSE_PTR(fn, f) \
	CASCADE_NULL(ptr->f = fn(ptr->f, oldr, r));

#define TRAVERSE_ARRAY_BEGIN(type, size, f) \
	COPY_ONLY_FROM_REGION2(f) { \
		type *oldf = ptr->f; \
    void *newf; \
		CASCADE_NULL(newf = region_alloc(r, sizeof(type) * size)); \
		memcpy(newf, oldf, sizeof(type) * size); \
    ptr->f = (type *)newf; \
	}

#define TRAVERSE_ARRAY_END(type, size, f)

#define TRAVERSE_PTR_TAPP(fn, f, cpfn) \
	CASCADE_NULL(ptr->f = fn(ptr->f, oldr, r, cpfn));

#define GET_VAR_ARRAY_LEN(type, len, f) \
	  type* l = ptr->f; \
	  while(*l != (type) 0) { \
		  l++; \
	  } \
	  int len = l - ptr->f + 1; \

#define COPY_ONLY_FROM_REGION2(f) \
	if(IN_REGION(ptr->f, oldr))

/*ChkFuncType *regionChkCopier(CopyFuncType *fn) {
	if(fn == (CopyFuncType *)regionCpCondIndexVal) {
		return (ChkFuncType *) regionChkCondIndexVal;
	} else if(fn==(CopyFuncType *)regionCpNode) {
		return (ChkFuncType *) regionChkNode;
	}
	return NULL;
}*/

#endif
