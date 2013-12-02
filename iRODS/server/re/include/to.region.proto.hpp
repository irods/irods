/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef TO_REGION_PROTO_H
#define TO_REGION_PROTO_H

typedef void *(RegionCopyFuncType)(void *, Region *, Hashtable *);

#endif

#define RE_STRUCT_FUNC_TYPE RegionCopyFuncType

#define RE_STRUCT_FUNC(T) CONCAT(regionCp, T)

#define RE_STRUCT_FUNC_PROTO(T) \
    T *RE_STRUCT_FUNC(T)(T *ptr, Region *r, Hashtable *objectMap)

#define RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) \
	T *RE_STRUCT_FUNC(T)(T *ptr, Region *r, RegionCopyFuncType *cpfn, Hashtable *objectMap)

