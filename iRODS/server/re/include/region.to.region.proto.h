/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef REGION_TO_REGION_PROTO_H
#define REGION_TO_REGION_PROTO_H

typedef void *(RegionRegionCopyFuncType)(void *, Region *);

#endif

#define RE_STRUCT_FUNC(T) CONCAT(regionRegionCp, T)

#define RE_STRUCT_FUNC_TYPE RegionRegionCopyFuncType

#define RE_STRUCT_FUNC_PROTO(T) \
    T *RE_STRUCT_FUNC(T)(T *ptr, Region *r)

#define RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) \
	T *RE_STRUCT_FUNC(T)(T *ptr, Region *r, RegionRegionCopyFuncType *cpfn)

