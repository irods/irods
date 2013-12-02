/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef _CONVERSION_H
#define	_CONVERSION_H
#include "utils.h"
#include "parser.h"


/** conversion functions */
int getCollectionSize(char *typ, void *inPtr, Region *r);
Res* getValueFromCollection(char *typ, void *inPtr, int inx, Region *r);

int updateMsParamArrayToEnv(msParamArray_t *var, Env *env, rError_t *errmsg, Region *r);
int updateMsParamArrayToEnvAndFreeNonIRODSType(msParamArray_t *var, Env *env, rError_t *errmsg, Region *r);
int convertMsParamArrayToEnv(msParamArray_t *var, Env *env, rError_t *errmsg, Region *r);
int convertMsParamArrayToEnvAndFreeNonIRODSType(msParamArray_t *var, Env *env, rError_t *errmsg, Region *r);
int convertMsParamToRes(msParam_t *var, Res *res, rError_t *errmsg, Region *r);
int convertMsParamToResAndFreeNonIRODSType(msParam_t *mP, Res *res, rError_t *errmsg, Region *r);
void convertCollectionToRes(msParam_t *mP, Res* res);
void convertDoubleValue(Res *res, double inval, Region *r);
void convertStrValue(Res *res, char *val, Region *r);
/**
 * convert int value to Res
 */
void convertIntValue(Res *res, int inval, Region *r);

int convertResToMsParam(msParam_t *var, Res *res, rError_t *errmsg);
int updateResToMsParam(msParam_t *var, Res *res, rError_t *errmsg);
int convertEnvToMsParamArray(msParamArray_t *var, Env *env, rError_t *errmsg, Region *r);
int convertHashtableToMsParamArray(msParamArray_t *var, Hashtable *env, rError_t *errmsg, Region *r);

char* convertResToString(Res *res);
int convertResToIntReturnValue(Res* res);
#endif	/* _CONVERSION_H */

