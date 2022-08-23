/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef _CONVERSION_HPP
#define	_CONVERSION_HPP
#include "irods/private/re/utils.hpp"
#include "irods/private/re/parser.hpp"


/** conversion functions */
int getCollectionSize( char *typ, void *inPtr );
Res* getValueFromCollection( char *typ, void *inPtr, int inx, Region *r );

int updateMsParamArrayToEnv( msParamArray_t *var, Env *env, Region *r );
int updateMsParamArrayToEnvAndFreeNonIRODSType( msParamArray_t *var, Env *env, Region *r );
int convertMsParamArrayToEnv( msParamArray_t *var, Env *env, Region *r );
int convertMsParamArrayToEnvAndFreeNonIRODSType( msParamArray_t *var, Env *env, Region *r );
int convertMsParamToRes( msParam_t *var, Res *res, Region *r );
int convertMsParamToResAndFreeNonIRODSType( msParam_t *mP, Res *res, Region *r );
Res* convertMsParamToRes( msParam_t *mP, Region *r );
/*void convertCollectionToRes( msParam_t *mP, Res* res );*/
void convertDoubleValue( Res *res, double inval, Region *r );
void convertStrValue( Res *res, char *val, Region *r );
/**
 * convert int value to Res
 */
void convertIntValue( Res *res, int inval, Region *r );

int convertResToMsParam( msParam_t *var, Res *res, rError_t *errmsg );
int updateResToMsParam( msParam_t *var, Res *res, rError_t *errmsg );
int convertEnvToMsParamArray( msParamArray_t *var, Env *env, rError_t *errmsg, Region *r );
int convertHashtableToMsParamArray( msParamArray_t *var, Hashtable *env, rError_t *errmsg );

char* convertResToString( Res *res );
int convertResToIntReturnValue( Res* res );
#endif	/* _CONVERSION_H */

