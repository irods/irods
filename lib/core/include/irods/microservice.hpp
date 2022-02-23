/**
features:
automatic input/output handling
automatic factory code generation

    MICROSERVICE_BEGIN(
      add,
      INT, a, INPUT,
      INT, b, INPUT,
      INT, c, OUTPUT ALLOC)

    c = a + b;
  MICROSERVICE_END


  MICROSERVICE_BEGIN(
      concat,
      STR, a, INPUT OUTPUT,
      STR, b, INPUT)

    int al = strlen(a);
    int bl = strlen(b);
    char *c = (char *) malloc(al + bl + 1);
    if(c==NULL) {
        RETURN( -1);
    }
    snprintf(c, "%s%s", a, b);
    a = c; // no need to free a
  MICROSERVICE_END

  MICROSERVICE_BEGIN(
      unusedParam,
      STR, a, INPUT OUTPUT,
      STR, b, INPUT)

    (void) a;
    (void) b;

  MICROSERVICE_END
*/
#ifndef MICROSERVICE_HPP_

#define MICROSERVICE_HPP_
#undef BOOST_PP_VARIADICS
#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor/comparison/greater_equal.hpp>
#include <boost/preprocessor/comparison/less_equal.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/list/at.hpp>
#include <boost/preprocessor/list/first_n.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/list/reverse.hpp>
#include <boost/preprocessor/list/size.hpp>
#include <boost/preprocessor/list/filter.hpp>
#include <boost/preprocessor/seq/to_list.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/while.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/arithmetic/div.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/mod.hpp>
#include <boost/preprocessor/arithmetic/mul.hpp>



typedef int INT;
typedef char* STR;

template<typename T>
class ParamType {
    public:
        typedef T* type;
        typedef T& refType;
        static type defaultValue() {
            return NULL;
        }
        static refType convert( type& obj ) {
            return *obj;
        }
};

template<>
class ParamType <STR> {
    public:
        typedef char* type;
        typedef STR& refType;
        static type defaultValue() {
            return NULL;
        }
        static refType convert( type& obj ) {
            return obj;
        }
};

#define INPUT (0)
#define OUTPUT (1)
#define ALLOC (2)            // automatically allocate memory for output variable
#define NO_ALLOC (3)         // do not automatically allocate memory for output variable, this is the default behavior
#define STRUCT (4)           // call malloc/free to alloc/deallocate memory, this is the default behavior
#define CLASS (5)            // call new/delete to alloc/deallocate memory
#define DEALLOC (6)          // automatically deallocate memory for variable, this is the default behavior
#define NO_DEALLOC (7)       // do not automatically deallocate memory for variable
#define REF (8)              // pass in reference, this is the default behavior, an output variable with no reference is implicitly a pointer
#define PTR (9)              // pass in pointer rather than reference, an output variable with no reference is implicitly a pointer

#define EXPAND(f) f

#define TEST_FLAG(a, f) \
    BOOST_PP_LIST_IS_CONS(BOOST_PP_LIST_FILTER(TEST_FLAG_PRED, EXPAND f, BOOST_PP_SEQ_TO_LIST(a))) \

#define TEST_FLAG_PRED(d, data, elem) \
    BOOST_PP_EQUAL(data, elem)

#define IRODS_TYPE(t) \
    BOOST_PP_STRINGIZE(t) "_PI" \

#define C_TYPE(t) \
    t

#define PARAM_NAME(n) \
    BOOST_PP_CAT(_msi_param_, n) \

#define BUF_NAME(n) \
    BOOST_PP_CAT(_msi_buf_, n) \

// param list
#define MSI_PARAM(z, i, data) \
    msParam_t* PARAM_NAME(BOOST_PP_LIST_AT(data, i)) , \

#define MSI_PARAM_LIST(list) \
    BOOST_PP_REPEAT(BOOST_PP_LIST_SIZE(list), MSI_PARAM, list) \

// define params
#define MSI_DEFINE_PARAM(z, i, data) \
    _MSI_DEFINE_PARAM( \
        C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
        BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
        BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data), i), \
        PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
        IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)) \
    ) \

#define _MSI_DEFINE_PARAM(_type, name, ioType, pname, irodsType) \
        ParamType<_type>::type BUF_NAME(name) = NULL; \

#define MSI_DEFINE_PARAM_LIST(typeList, nameList, ioList) \
    BOOST_PP_REPEAT(BOOST_PP_LIST_SIZE(typeList), MSI_DEFINE_PARAM, (typeList, nameList, ioList)) \

// extract params
#define MSI_EXTRACT_PARAM(z, i, data) \
  BOOST_PP_IF( \
    TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), INPUT), \
    BOOST_PP_IF( \
        TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), PTR), \
        _MSI_EXTRACT_PARAM_INPUT_PTR( \
            C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
            BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
            PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
            IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)) \
        ), \
        _MSI_EXTRACT_PARAM_INPUT( \
            C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
            BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
            PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
            IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)) \
        )\
    ), \
    BOOST_PP_IF( \
        TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), ALLOC), \
        BOOST_PP_IF( \
            TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), CLASS), \
            _MSI_EXTRACT_PARAM_ALLOC_CLASS( \
                C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
                BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
                PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
                IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)) \
            ), \
            _MSI_EXTRACT_PARAM_ALLOC( \
                C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
                BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
                PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
                IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)) \
            ) \
        ), \
        _MSI_EXTRACT_PARAM_NO_ALLOC( \
            C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
            BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
            PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
            IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)) \
        ) \
    ) \
  ) \

#define _MSI_EXTRACT_PARAM_INPUT(_type, name, pname, irodsType) \
        if ( pname ==  NULL || \
                strcmp( pname->type , irodsType ) != 0 || \
                pname->inOutStruct == NULL ) { \
            return( USER_PARAM_TYPE_ERR ); \
        } \
        BUF_NAME(name) = (ParamType<_type>::type) pname->inOutStruct; \
        _type& name = ParamType<_type>::convert(BUF_NAME(name)); \

#define _MSI_EXTRACT_PARAM_INPUT_PTR(_type, name, pname, irodsType) \
        if ( pname ==  NULL || \
                strcmp( pname->type , irodsType ) != 0 || \
                pname->inOutStruct == NULL ) { \
            return( USER_PARAM_TYPE_ERR ); \
        } \
        BUF_NAME(name) = (ParamType<_type>::type) pname->inOutStruct; \
        ParamType<_type>::type name = BUF_NAME(name); \

#define _MSI_EXTRACT_PARAM_ALLOC(_type, name, pname, irodsType) \
        BUF_NAME(name) = (ParamType<_type>::type) malloc(sizeof(_type)); \
        if ( BUF_NAME(name) ==  NULL ) { \
            return( USER_PARAM_TYPE_ERR ); \
        } \
        _type& name = ParamType<_type>::convert(BUF_NAME(name)); \

#define _MSI_EXTRACT_PARAM_ALLOC_CLASS(_type, name, pname, irodsType) \
        BUF_NAME(name) = new ParamType<_type>::type(); \
        _type& name = ParamType<_type>::convert(BUF_NAME(name)); \

#define _MSI_EXTRACT_PARAM_NO_ALLOC(_type, name, pname, irodsType) \
        BUF_NAME(name) = NULL; \
        ParamType<_type>::type& name = BUF_NAME(name); \

#define MSI_EXTRACT_PARAM_LIST(typeList, nameList, ioList) \
    BOOST_PP_REPEAT(BOOST_PP_LIST_SIZE(typeList), MSI_EXTRACT_PARAM, (typeList, nameList, ioList)) \

// output params
#define MSI_OUTPUT_PARAM(z, i, data) \
  BOOST_PP_IF( \
    TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), OUTPUT), \
    BOOST_PP_IF( \
        TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), CLASS), \
        _MSI_OUTPUT_PARAM_CLASS( \
            C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
            BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
            PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
            IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)), \
            TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), NO_DEALLOC) \
        ), \
        _MSI_OUTPUT_PARAM( \
            C_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data),i)), \
            BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i), \
            PARAM_NAME(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,1,data), i)), \
            IRODS_TYPE(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,0,data), i)), \
            TEST_FLAG(BOOST_PP_LIST_AT(BOOST_PP_TUPLE_ELEM(3,2,data),i), NO_DEALLOC) \
        ) \
    ), \
    BOOST_PP_EMPTY() \
  )\

#define _MSI_OUTPUT_PARAM(_type, name, pname, irodsType, dealloc) \
        if ( pname->type == NULL ) pname->type = irodsType ? strdup( irodsType ) : NULL; \
        if ( dealloc && pname->inOutStruct != NULL && pname->inOutStruct != (void *) BUF_NAME(name) ) free(pname->inOutStruct); \
        pname->inOutStruct = BUF_NAME(name); \

#define _MSI_OUTPUT_PARAM_CLASS(_type, name, pname, irodsType, dealloc) \
        if ( pname->type == NULL ) pname->type = irodsType ? strdup( irodsType ) : NULL; \
        if ( dealloc && pname->inOutStruct != NULL && pname->inOutStruct != (void *) BUF_NAME(name) ) delete ((ParamType<_type>::type) pname->inOutStruct;) \
        pname->inOutStruct = BUF_NAME(name); \

#define MSI_OUTPUT_PARAM_LIST(typeList, nameList, ioList) \
    BOOST_PP_REPEAT(BOOST_PP_LIST_SIZE(typeList), MSI_OUTPUT_PARAM, (typeList, nameList, ioList)) \

// distribute a list of type, name, io into three lists
#define DISTRIBUTE_OP(d, state) \
    (BOOST_PP_TUPLE_ELEM(4,1,state), \
     BOOST_PP_TUPLE_ELEM(4,2,state), \
     (BOOST_PP_LIST_FIRST(BOOST_PP_TUPLE_ELEM(4,3,state)), BOOST_PP_TUPLE_ELEM(4,0,state)), \
     BOOST_PP_LIST_REST(BOOST_PP_TUPLE_ELEM(4,3,state))) \

#define DISTRIBUTE_PRED(d, state) \
    BOOST_PP_LIST_IS_CONS(BOOST_PP_TUPLE_ELEM(4,3,state)) \

#define DISTRIBUTE(list) \
    FLIP_TRUNC_CONCAT(_DISTRIBUTE(list)) \

#define FLIP_TRUNC_CONCAT(t) \
    (BOOST_PP_TUPLE_ELEM(4, 2, t), BOOST_PP_TUPLE_ELEM(4, 1, t), BOOST_PP_TUPLE_ELEM(4, 0, t)) \

#define _DISTRIBUTE(list) \
    BOOST_PP_WHILE(DISTRIBUTE_PRED, DISTRIBUTE_OP, (BOOST_PP_NIL, BOOST_PP_NIL, BOOST_PP_NIL, BOOST_PP_LIST_REVERSE(list))) \

#define SIG(msName, nameList) \
    int msName ( \
        MSI_PARAM_LIST(nameList) \
        ruleExecInfo_t* rei ) \

#define SIG_NIL(msName) \
    int msName ( \
        ruleExecInfo_t* rei ) \

#define MICROSERVICE_NIL(msName) \
extern "C" { \
    MICROSERVICE_FACTORY( BOOST_PP_STRINGIZE(msName), 0 ) \
    SIG_NIL(msName) { \
    int _status; \
    goto start; \
    output: \
    return _status; \
    start: \

#define MICROSERVICE_LIST(msName, typeList, nameList, ioList) \
extern "C" { \
    MICROSERVICE_FACTORY( BOOST_PP_STRINGIZE(msName), BOOST_PP_LIST_SIZE(typeList) ) \
    SIG(msName, nameList) { \
    int _status; \
	MSI_DEFINE_PARAM_LIST(typeList, nameList, ioList) \
    goto start; \
    output: \
    MSI_OUTPUT_PARAM_LIST(typeList, nameList, ioList) \
    return _status; \
    start: \
	MSI_EXTRACT_PARAM_LIST(typeList, nameList, ioList) \

#define MICROSERVICE_FACTORY(msNameStr, nParams) \
    irods::ms_table_entry*  plugin_factory( ) { \
        irods::ms_table_entry* msvc = new irods::ms_table_entry( nParams ); \
        msvc->add_operation( msNameStr, msNameStr ); \
        return msvc; \
    } \


#        define BOOST_PP_VARIADIC_SIZE(...) BOOST_PP_VARIADIC_SIZE_I(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,)
#    define BOOST_PP_VARIADIC_SIZE_I(e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15, e16, e17, e18, e19, e20, e21, e22, e23, e24, e25, e26, e27, e28, e29, e30, e31, e32, e33, e34, e35, e36, e37, e38, e39, e40, e41, e42, e43, e44, e45, e46, e47, e48, e49, e50, e51, e52, e53, e54, e55, e56, e57, e58, e59, e60, e61, e62, e63, size, ...) size

#define MICROSERVICE_BEGIN(msName, ...) \
    BOOST_PP_IF( \
        BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), \
        _MICROSERVICE_BEGIN(msName, \
            DISTRIBUTE(BOOST_PP_TUPLE_TO_LIST(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), (__VA_ARGS__))) \
        ), \
        MICROSERVICE_NIL(msName) \
    ) \

#define _MICROSERVICE_BEGIN(msName, lists) \
    MICROSERVICE_LIST (msName, BOOST_PP_TUPLE_ELEM(3,0,lists),BOOST_PP_TUPLE_ELEM(3,1,lists),BOOST_PP_TUPLE_ELEM(3,2,lists)) \

#define RETURN(status) \
    _status = status; \
    goto output; \

#define MICROSERVICE_END \
    _status = 0; \
    goto output; \
    } \
} \

#endif // MICROSERVICE_HPP_
