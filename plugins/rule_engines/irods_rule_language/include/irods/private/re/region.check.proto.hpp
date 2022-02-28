/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef REGION_CHECK_PROTO_HPP
#define REGION_CHECK_PROTO_HPP

typedef int ( ChkFuncType )( void *ptr, Region *r );
#endif

#include "irods/private/re/proto.hpp"

#define RE_STRUCT_FUNC_PROTO(T) \
    int regionChk##T(T *ptr, Region *r)

#define RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) \
    int regionChk##T(T *ptr, Region *r, ChkFuncType *cpfn)

