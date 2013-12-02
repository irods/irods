/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef CACHE_PROTO_H
#define CACHE_PROTO_H

typedef void * (CacheCopyFuncType)(unsigned char *, unsigned char **, unsigned char **, void *, Hashtable *, int);

#endif

#define RE_STRUCT_FUNC(T) CONCAT(copy, T)

#define RE_STRUCT_FUNC_TYPE CacheCopyFuncType

#define RE_STRUCT_FUNC_PROTO(T) \
		T* RE_STRUCT_FUNC(T)(unsigned char *buf, unsigned char **p, unsigned char **pointers, T *ptr, Hashtable *objectMap, int generatePtrDesc)

/* #define COPY_FUNC_BEGIN(T) \
	 COPY_FUNC_PROTO(T) {  \
		  allocateInBuffer(T, ecopy, e); */

#define RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) \
		T* RE_STRUCT_FUNC(T)(unsigned char *buf, unsigned char **p, unsigned char **pointers, T *ptr, RE_STRUCT_FUNC_TYPE *cpfn, Hashtable *objectMap, int generatePtrDesc)
