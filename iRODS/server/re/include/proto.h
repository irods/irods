/* For copyright information please refer to files in the COPYRIGHT directory
 */

#define MK_TRANSIENT_PTR(type, f)

#define MK_TRANSIENT_VAL(type, f, val)

#define PARAMETER(T)

#define MK_VAL(type, f)

#define MK_PTR(type, f)

#define MK_PTR_TVAR(f, T)

#define MK_PTR_TAPP(type, f, cpr)

#define MK_ARRAY(type, size, f)

#define MK_ARRAY_IN_STRUCT(type, size, f)

#define MK_VAR_ARRAY(type, f)

#define MK_VAR_ARRAY_IN_STRUCT(type, f)

#define MK_PTR_ARRAY(type, size, f)

#define MK_PTR_ARRAY_IN_STRUCT(type, size, f)

#define MK_PTR_VAR_ARRAY(type, f)

#define MK_PTR_VAR_ARRAY_IN_STRUCT(type, f)

#define MK_PTR_TAPP_ARRAY(type, size, f, cpfn)

#define MK_PTR_TAPP_ARRAY_IN_STRUCT(type, size, f, cpfn)

#define RE_STRUCT_BEGIN(T) \
    RE_STRUCT_FUNC_PROTO(T);

#define RE_STRUCT_GENERIC_BEGIN(T, cpfn) \
	RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn);

#define RE_STRUCT_GENERIC_END(T, cpfn)

#define RE_STRUCT_END(T)
