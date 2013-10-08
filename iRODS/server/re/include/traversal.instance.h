/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef TRAVERSAL_INSTANCE_H
#define TRAVERSAL_INSTANCE_H

#define TRAVERSE_PTR_ARRAY(type, size, f) \
	  int i; \
	  for(i=0;i<size;i++) { \
		  MK_PTR(type, f[i]); \
	  } \

#define TRAVERSE_PTR_TAPP_ARRAY(type, size, f, cpfn) \
	  int i; \
	  for(i=0;i<size;i++) { \
		  MK_PTR_TAPP(type, f[i], cpfn); \
	  } \

#define TRAVERSE_CYCLIC(T, key, objectMap) \
	T *shared; \
	char key[KEY_SIZE]; \
	key##T(ptr, key); \
	if((shared = (T *)lookupFromHashTable(objectMap, key)) != NULL) { \
		return shared; \
	} \

#define TRAVERSE_ARRAY_CYCLIC(T, size, f, tgt, key, objectMap) \
	T *shared0; \
	char key[KEY_SIZE]; \
	keyBuf((unsigned char *) ptr->f, sizeof(T[size]), key); \
	if((shared0 = (T *)lookupFromHashTable(objectMap, key)) != NULL) { \
		tgt = shared0; \
	} else \

#endif

#define PARAM(T) (RE_STRUCT_FUNC_TYPE *)RE_STRUCT_FUNC(T)

#define TRAVERSE_NON_NULL(f) \
	if(ptr->f != NULL)

#ifndef MK_TRANSIENT_VAL
#define MK_TRANSIENT_VAL(type, f, val)
#endif

#ifndef MK_TRANSIENT_PTR
#define MK_TRANSIENT_PTR(type, f)
#endif

#ifndef MK_VAL
#define MK_VAL(type, f)
#endif

#define MK_PTR(type, f) \
	TRAVERSE_NON_NULL(f) { \
		TRAVERSE_PTR(RE_STRUCT_FUNC(type), f); \
	}

#define MK_PTR_TVAR(f, T) \
	TRAVERSE_NON_NULL(f) { \
		TRAVERSE_PTR(T, f); \
	}

#define MK_PTR_TAPP(type, f, cpfn) \
	TRAVERSE_NON_NULL(f) { \
		TRAVERSE_PTR_TAPP(RE_STRUCT_FUNC(type), f, cpfn); \
	}

#define MK_ARRAY(type, size, f) \
	  TRAVERSE_NON_NULL(f) \
	  { \
		TRAVERSE_ARRAY_BEGIN(type, ptr->size, f); \
		TRAVERSE_ARRAY_END(type, ptr->size, f); \
	  }

#define MK_ARRAY_IN_STRUCT(type, size, f)

#define MK_VAR_ARRAY(type, f) \
	  TRAVERSE_NON_NULL(f) { \
	  	  GET_VAR_ARRAY_LEN(type, len, f); \
		  TRAVERSE_ARRAY_BEGIN(type, len, f); \
		  TRAVERSE_ARRAY_END(type, len, f); \
	  }

#define MK_VAR_ARRAY_IN_STRUCT(type, f)

#define MK_PTR_ARRAY(type, size, f) \
	  TRAVERSE_NON_NULL(f) \
	  { \
	    TRAVERSE_ARRAY_BEGIN(CONCAT(type, Ptr), ptr->size, f); \
	    TRAVERSE_PTR_ARRAY(type, ptr->size, f); \
	    TRAVERSE_ARRAY_END(CONCAT(type, Ptr), ptr->size, f); \
	  }

#define MK_PTR_ARRAY_IN_STRUCT(type, size, f) \
      { \
		TRAVERSE_PTR_ARRAY(type, ptr->size, f); \
      }

#define MK_PTR_VAR_ARRAY(type, f) \
	  TRAVERSE_NON_NULL(f) \
	  { \
	    GET_VAR_ARRAY_LEN(type, len, f); \
		TRAVERSE_ARRAY_BEGIN(type, len, f); \
	    TRAVERSE_PTR_ARRAY(CONCAT(type, Ptr), len, f); \
	    TRAVERSE_ARRAY_END(type, len, f); \
	  }

#define MK_PTR_VAR_ARRAY_IN_STRUCT(type, f) \
      { \
		GET_VAR_ARRAY_LEN(type, f); \
		TRAVERSE_PTR_ARRAY(type, len, f); \
      }

#define MK_PTR_TAPP_ARRAY(type, size, f, cpfn) \
	  TRAVERSE_NON_NULL(f) \
	  { \
	    TRAVERSE_ARRAY_BEGIN(CONCAT(type, Ptr), ptr->size, f); \
	    TRAVERSE_PTR_TAPP_ARRAY(type, ptr->size, f, cpfn); \
	    TRAVERSE_ARRAY_END(CONCAT(type, Ptr), ptr->size, f); \
	  }

#define MK_PTR_TAPP_ARRAY_IN_STRUCT(type, size, f, cpfn) \
      { \
		TRAVERSE_PTR_TAPP_ARRAY(type, ptr->size, f, cpfn); \
	  }

#define RE_STRUCT_BEGIN(T) \
    RE_STRUCT_FUNC_PROTO(T) {  \
		TRAVERSE_BEGIN(T)

#define RE_STRUCT_GENERIC_BEGIN(T, cpfn) \
	RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) {	\
    	TRAVERSE_BEGIN(T)

#define RE_STRUCT_GENERIC_END(T, cpfn) \
		TRAVERSE_END(T); \
	}

#define RE_STRUCT_END(T) \
		TRAVERSE_END(T); \
	}


