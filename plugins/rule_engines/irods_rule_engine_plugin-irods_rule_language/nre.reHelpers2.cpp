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

    while ( getMsParamByLabel( msParamArray, v ) != nullptr ) {
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

