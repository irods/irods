/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.hpp"
#include "resource.hpp"
#include "dataObjOpr.hpp"

extern char *rmemmove( void *dest, void *src, int strLen, int maxLen );

static int staticVarNumber = 1;


int
getNewVarName( char *v, msParamArray_t *msParamArray ) {
    /*  msParam_t *mP;*/

    sprintf( v, "*RNDVAR%i", staticVarNumber );
    staticVarNumber++;

    while ( getMsParamByLabel( msParamArray, v ) != NULL ) {
        sprintf( v, "*RNDVAR%i", staticVarNumber );
        staticVarNumber++;
    }


    return 0;
}

int
removeTmpVarName( msParamArray_t *msParamArray ) {

    int i;

    for ( i = 0; i < msParamArray->len; i++ ) {
        if ( strncmp( msParamArray->msParam[i]->label, "*RNDVAR", 7 ) == 0 ) {
            rmMsParamByLabel( msParamArray, msParamArray->msParam[i]->label, 1 );
        }
    }
    return 0;

}


int
carryOverMsParam( msParamArray_t *sourceMsParamArray, msParamArray_t *targetMsParamArray ) {

    int i;
    msParam_t *mP, *mPt;
    char *a;
    char *b;
    if ( sourceMsParamArray == NULL ) {
        return 0;
    }
    /****
    for (i = 0; i < sourceMsParamArray->len ; i++) {
      mPt = sourceMsParamArray->msParam[i];
      if ((mP = getMsParamByLabel (targetMsParamArray, mPt->label)) != NULL)
    rmMsParamByLabel (targetMsParamArray, mPt->label, 1);
      addMsParam(targetMsParamArray, mPt->label, mPt->type, NULL,NULL);
      j = targetMsParamArray->len - 1;
      free(targetMsParamArray->msParam[j]->label);
      free(targetMsParamArray->msParam[j]->type);
      replMsParam(mPt,targetMsParamArray->msParam[j]);
    }
    ****/
    /****
    for (i = 0; i < sourceMsParamArray->len ; i++) {
      mPt = sourceMsParamArray->msParam[i];
      if ((mP = getMsParamByLabel (targetMsParamArray, mPt->label)) != NULL)
    rmMsParamByLabel (targetMsParamArray, mPt->label, 1);
      addMsParamToArray(targetMsParamArray,
    		mPt->label, mPt->type, mPt->inOutStruct, mPt->inpOutBuf, 1);
    }
    ****/
    for ( i = 0; i < sourceMsParamArray->len ; i++ ) {
        mPt = sourceMsParamArray->msParam[i];
        if ( ( mP = getMsParamByLabel( targetMsParamArray, mPt->label ) ) != NULL ) {
            a = mP->label;
            b = mP->type;
            mP->label = NULL;
            mP->type = NULL;
            /** free(mP->inOutStruct);**/
            free( mP->inpOutBuf );
            replMsParam( mPt, mP );
            free( mP->label );
            mP->label = a;
            mP->type = b;
        }
        else
            addMsParamToArray( targetMsParamArray,
                               mPt->label, mPt->type, mPt->inOutStruct, mPt->inpOutBuf, 1 );
    }

    return 0;
}
