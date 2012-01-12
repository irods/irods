/* For copyright information please refer to files in the COPYRIGHT directory
 */

/* Define a non default key generation function for Node */
#if !defined(KEY_INSTANCE)
RE_STRUCT_BEGIN(Node)
	MK_VAL(int, nodeType)
    MK_VAL(int, degree)
	MK_VAL(int, option)
	MK_VAL(int, ival)
	MK_VAL(double, dval)
	MK_VAL(rodsLong_t, lval)
	MK_VAL(rodsLong_t, expr)
    MK_VAR_ARRAY_PTR(char, base)
    MK_VAR_ARRAY_PTR(char, text)
    MK_PTR(Node, exprType)
    MK_PTR(Node, coercionType)
    MK_PTR_ARRAY_PTR(Node, degree, subtrees)
	MK_PTR(RuleIndexList, ruleIndexList)
	MK_TRANSIENT_PTR(SmsiFuncType, func)
	MK_PTR(msParam_t, param)
/*      printf("inserting %s\n", key); */
/*
          printf("tvar %s is added to shared objects\n", tvarNameBuf);
*/
RE_STRUCT_END(Node)
#endif

RE_STRUCT_BEGIN(msParam_t)
	MK_VAR_ARRAY_PTR(char, label)
	MK_VAR_ARRAY_PTR(char, type)
	MK_TRANSIENT_PTR(void, inOutStruct)
	MK_TRANSIENT_PTR(bytesBuf_t, inpOutBuf)
RE_STRUCT_END(msParam_t)

RE_STRUCT_BEGIN(RuleDesc)
	MK_VAL(int, id)
  	MK_VAL(RuleType, ruleType)
  	MK_PTR(Node, node)
  	MK_PTR(Node, type)
	MK_VAL(int, dynamictyping)
RE_STRUCT_END(RuleDesc)

RE_STRUCT_BEGIN(RuleSet)
	MK_VAL(int, len)
    MK_PTR_ARRAY(RuleDesc, len, rules)
RE_STRUCT_END(RuleSet)

RE_STRUCT_BEGIN(RuleIndexListNode)
	MK_VAL(int, ruleIndex)
	MK_VAL(int, secondaryIndex)
    MK_PTR(RuleIndexListNode, next)
	MK_PTR(RuleIndexListNode, prev)
    MK_PTR(CondIndexVal, condIndex)
RE_STRUCT_END(RuleIndexListNode)

RE_STRUCT_BEGIN(RuleIndexList)
    MK_PTR(RuleIndexListNode, head)
    MK_PTR(RuleIndexListNode, tail)
    MK_VAR_ARRAY_PTR(char, ruleName)
RE_STRUCT_END(RuleIndexList)

RE_STRUCT_GENERIC_BEGIN(Env, T)
	MK_PTR_GENERIC(Env, previous, T)
	MK_PTR_GENERIC(Env, lower, T)
	MK_PTR_GENERIC(Hashtable, current, T)
RE_STRUCT_GENERIC_END(Env, T)

RE_STRUCT_GENERIC_BEGIN(Bucket, T)
	MK_VAR_ARRAY_PTR(char, key)
	MK_GENERIC_PTR(value, T)
    MK_PTR_GENERIC(Bucket, next, T)
RE_STRUCT_GENERIC_END(Bucket, T)

RE_STRUCT_GENERIC_BEGIN(Hashtable, T)
	MK_VAL(int, size)
	MK_VAL(int, len)
	MK_VAL(int, dynamic)
	MK_PTR_ARRAY_PTR_GENERIC(Bucket, size, buckets, T)
	MK_TRANSIENT_PTR(Region, bucketRegion)
RE_STRUCT_GENERIC_END(Hashtable, T)

RE_STRUCT_BEGIN(CondIndexVal)
    MK_PTR(Node, condExp)
    MK_PTR(Node, params)
    MK_PTR_GENERIC(Hashtable, valIndex,GENERIC( RuleIndexListNode))
RE_STRUCT_END(CondIndexVal)

RE_STRUCT_BEGIN(bytesBuf_t)
RE_STRUCT_END(bytesBuf_t)
