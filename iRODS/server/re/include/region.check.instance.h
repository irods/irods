/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef REGION_CHECK_INSTANCE_H
#define REGION_CHECK_INSTANCE_H

#include "region.check.proto.h"

#define CHECK_NON_NULL(f) \
	  if(ptr->f == NULL) { \
    	  return 1; \
      } else

#define CHECK_NOT_IN_REGION(f) \
	if(!IN_REGION(ptr->f, r)) { \
		return 0; \
	} else

#define GENERIC(T) (ChkFuncType *) regionChk##T
#define MK_TRANSIENT_VALUE(type, f, val) \

#define MK_TRANSIENT_PTR(type, f) \

#define MK_VAL(type, f)

#define MK_PTR(type, f) \
	CHECK_NON_NULL(f) \
	{ \
		return CONCAT(regionChk, type)(ptr->f, r); \
	}

#define MK_PTR_ARRAY_PTR(type, size, f) \
	CHECK_NON_NULL(f) \
	CHECK_NOT_IN_REGION(f) { \
		int i; \
		for(i=0;i<ptr->size;i++) { \
			if(ptr->f[i]!=NULL && !IN_REGION(ptr->f[i], r)) { \
				return 0; \
			} \
		} \
		return 1; \
	}

#define MK_ARRAY_PTR(type, size, f) \
	CHECK_NON_NULL(f) \
	CHECK_NOT_IN_REGION(f) \
	{ \
		return 1; \
	}

#define MK_PTR_ARRAY(type, size, f) \
	{ \
		int i; \
		for(i=0;i<ptr->size;i++) { \
			if(ptr->f[i]!=NULL && !IN_REGION(ptr->f[i], r)) { \
				return 0; \
			} \
		} \
		return 1; \
	}

#define MK_ARRAY(type, size, f)

#define MK_PTR_VAR_ARRAY_PTR(type, f) \
	  CHECK_NON_NULL(f) \
	  CHECK_NOT_IN_REGION(f) \
	  { \
		int i; \
		for(i=0;ptr->f[i] != (type) 0;i++) { \
			if(ptr->f[i]!=NULL && !CONCAT(regionChk, type)(ptr->f, r)) { \
				return 0; \
			} \
		} \
		return 1; \
	  }

#define MK_VAR_ARRAY_PTR(type, f) \
	  CHECK_NON_NULL(f) \
	  CHECK_NOT_IN_REGION(f) { \
		return 1; \
	  }

#define MK_PTR_VAR_ARRAY(type, f) \
      { \
		int i; \
		for(i=0;ptr->f[i] != (type) 0;i++) { \
			if(ptr->f[i]!=NULL && !CONCAT(regionChk, type)(ptr->f, r)) { \
				return 0; \
			} \
		} \
		return 1; \
      }

#define MK_VAR_ARRAY(type, f)

#define MK_PTR_GENERIC(type, f, cpfn) \
	CHECK_NON_NULL(f) \
	{ \
		return CONCAT(regionChk, type)(ptr->f, r, cpfn); \
	}

#define MK_PTR_ARRAY_PTR_GENERIC(type, size, f, cpfn) \
	CHECK_NON_NULL(f) \
	CHECK_NOT_IN_REGION(f) { \
		int i; \
		for(i=0;i<ptr->size;i++) { \
			if(ptr->f[i]!=NULL && !CONCAT(regionChk, type)(ptr->f[i], r, cpfn)) { \
				return 0; \
			} \
		} \
		return 1; \
	}

#define MK_PTR_ARRAY_GENERIC(type, size, f, cpfn) \
	{ \
		int i; \
		for(i=0;i<ptr->size;i++) { \
			if(ptr->f[i]!=NULL && !CONCAT(regionChk, type)(ptr->f[i], r, cpfn)) { \
				return 0; \
			} \
		} \
		return 1; \
	}

#define MK_GENERIC_PTR(f, T) \
	CHECK_NON_NULL(f) { \
		return T(ptr->f, r); \
	}

#define RE_STRUCT_BEGIN(T) \
    RE_STRUCT_FUNC_PROTO(T) {  \

#define RE_STRUCT_GENERIC_BEGIN(T, cpfn) \
	RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) {	\

#define RE_STRUCT_GENERIC_END(T, cpfn) \
	RE_STRUCT_END(T)

#define RE_STRUCT_END(T) \
	return 1; \
	}

#endif
