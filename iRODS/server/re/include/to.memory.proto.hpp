/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef TO_MEMORY_PROTO_H
#define TO_MEMORY_PROTO_H

typedef void *(MemCopyFuncType)(void *, Hashtable *);

#endif

#define RE_STRUCT_FUNC(T) CONCAT(memCp, T)

#define RE_STRUCT_FUNC_TYPE MemCopyFuncType

#define RE_STRUCT_FUNC_PROTO(T) \
    T *RE_STRUCT_FUNC(T)(T *ptr, Hashtable *objectMap)

#define RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) \
	T *RE_STRUCT_FUNC(T)(T *ptr, MemCopyFuncType *cpfn, Hashtable *objectMap)

