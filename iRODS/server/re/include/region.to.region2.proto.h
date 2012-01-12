/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef REGION_TO_REGION2_PROTO_H
#define REGION_TO_REGION2_PROTO_H

typedef void *(RegionRegion2CopyFuncType)(void *, Region *, Region *);

#endif

#define RE_STRUCT_FUNC(T) CONCAT(regionRegion2Cp, T)

#define RE_STRUCT_FUNC_TYPE RegionRegion2CopyFuncType

#define RE_STRUCT_FUNC_PROTO(T) \
    T *RE_STRUCT_FUNC(T)(T *ptr, Region *oldr, Region *r)

#define RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) \
	T *RE_STRUCT_FUNC(T)(T *ptr, Region *oldr, Region *r, RegionRegion2CopyFuncType *cpfn)

