/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reFuncDefs.hpp"
#include "reGlobalsExtern.hpp"
#include "conversion.hpp"
#include "reVariableMap.gen.hpp"
#include "reVariables.hpp"
#include "rcMisc.hpp"
#ifdef DEBUG
#include "re.hpp"
#endif

int getIntLeafValue( Res **varValue, int leaf, Region *r ) {
    *varValue = newIntRes( r, leaf );
    return 0;
}
int getStrLeafValue( Res **varValue, char *leaf, Region *r ) {
    *varValue = newStringRes( r, leaf );
    return 0;
}
int getLongLeafValue( Res **varValue, rodsLong_t leaf, Region *r ) {
    *varValue = newDoubleRes( r, leaf );
    return 0;
}
int getPtrLeafValue( Res **varValue, void *leaf, bytesBuf_t *buf, char *irodsType, Region *r ) {
    *varValue = newUninterpretedRes( r, irodsType, leaf, buf );
    return 0;
}
int setBufferPtrLeafValue( bytesBuf_t **leafPtr, Res *newVarValue ) {
    *leafPtr = RES_UNINTER_BUFFER( newVarValue );
    return 0;
}
int setIntLeafValue( int *leafPtr, Res *newVarValue ) {
    *leafPtr = RES_INT_VAL( newVarValue );
    return 0;
}
int setStrLeafValue( char *leafPtr, size_t len, Res *newVarValue ) {
    rstrcpy( leafPtr, newVarValue->text, len );
    return 0;
}
int setStrDupLeafValue( char **leafPtr, Res *newVarValue ) {
    if ( *leafPtr != NULL ) {
        free( *leafPtr );
    }
    *leafPtr = strdup( newVarValue->text );
    return 0;
}
int setLongLeafValue( rodsLong_t *leafPtr, Res *newVarValue ) {
    *leafPtr = ( long )RES_DOUBLE_VAL( newVarValue );
    return 0;
}
int setStructPtrLeafValue( void **leafPtr, Res *newVarValue ) {
    *leafPtr = RES_UNINTER_STRUCT( newVarValue );
    return 0;
}
int
mapExternalFuncToInternalProc( char *funcName ) {
    int i;

    for ( i = 0; i < appRuleFuncMapDef.MaxNumOfFMaps; i++ ) {
        if ( strstr( appRuleFuncMapDef.funcName[i], funcName ) != NULL ) {
            strcpy( funcName, appRuleFuncMapDef.func2CMap[i] );
            return 1;
        }
    }
    for ( i = 0; i < coreRuleFuncMapDef.MaxNumOfFMaps; i++ ) {
        if ( strstr( coreRuleFuncMapDef.funcName[i], funcName ) != NULL ) {
            strcpy( funcName, coreRuleFuncMapDef.func2CMap[i] );
            return 1;
        }
    }
    return 0;
}

int
getVarMap( char *action, char *inVarName, char **varMap, int index ) {
    int i;
    char *varName;

    if ( inVarName[0] == '$' ) {
        varName = inVarName + 1;
    }
    else {
        varName = inVarName;
    }
    if ( index < 1000 ) {
        for ( i = index; i < appRuleVarDef.MaxNumOfDVars; i++ ) {
            if ( !strcmp( appRuleVarDef.varName[i], varName ) ) {
                if ( strlen( appRuleVarDef.action[i] ) == 0 ||
                        strstr( appRuleVarDef.action[i], action ) != NULL ) {
                    *varMap = strdup( appRuleVarDef.var2CMap[i] );
                    return i;
                }
            }
        }
        index = 1000;
    }
    i = index - 1000;
    for ( ; i < coreRuleVarDef.MaxNumOfDVars; i++ ) {
        if ( !strcmp( coreRuleVarDef.varName[i], varName ) ) {
            if ( strlen( coreRuleVarDef.action[i] ) == 0 ||
                    strstr( coreRuleVarDef.action[i], action ) != NULL ) {
                *varMap = strdup( coreRuleVarDef.var2CMap[i] );
                return i + 1000;
            }
        }
    }
    return UNKNOWN_VARIABLE_MAP_ERR;
}



int
getVarNameFromVarMap( char *varMap, char *varName, char **varMapCPtr ) {

    char *p;

    if ( ( p = strstr( varMap, "->" ) ) == NULL ) {
        rstrcpy( varName, varMap, NAME_LEN );
        *varMapCPtr = NULL;
    }
    else {
        *p = '\0';
        rstrcpy( varName, varMap, NAME_LEN );
        *p = '-';
        p++;
        p++;
        *varMapCPtr = p;
    }
    trimWS( varName );
    return 0;

}

ExprType *getVarType( char *varMap, Region *r ) {
    char varName[NAME_LEN];
    char *varMapCPtr;
    int i;

    i = getVarNameFromVarMap( varMap, varName, &varMapCPtr );
    if ( i != 0 ) {
        return newErrorRes( r, i );
    }

    if ( strcmp( varName, "rei" ) == 0 ) {
        return getVarTypeFromRuleExecInfo( varMapCPtr, r );
    }
    else {
        return newErrorRes( r, UNDEFINED_VARIABLE_MAP_ERR );
    }

}
int
getVarValue( char *varMap, ruleExecInfo_t *rei, Res **varValue, Region *r ) {

    char varName[NAME_LEN];
    char *varMapCPtr;
    int i;

    i = getVarNameFromVarMap( varMap, varName, &varMapCPtr );
    if ( i != 0 ) {
        return i;
    }

    if ( !strcmp( varName, "rei" ) ) {
        i = getValFromRuleExecInfo( varMapCPtr, rei, varValue, r );
        return i;
    }
    else {
        return UNDEFINED_VARIABLE_MAP_ERR;
    }
}

int
setVarValue( char *varMap, ruleExecInfo_t *rei, Res *newVarValue ) {
    char varName[NAME_LEN];
    char *varMapCPtr;
    int status = getVarNameFromVarMap( varMap, varName, &varMapCPtr );
    if ( status != 0 ) {
        return status;
    }
    if ( !strcmp( varName, "rei" ) ) {
        return setValFromRuleExecInfo( varMapCPtr, &rei, newVarValue );
    }
    else {
        return UNDEFINED_VARIABLE_MAP_ERR;
    }
}




/****************
int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return i;
  }

  if (ptr == NULL)
    return NULL_VALUE_ERR;
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return i;
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *) ptr-> , newVarValue,RE_STR);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *) ptr-> , newVarValue,RE_STR);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr->  , newVarValue,RE_STR);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
    i = getSetLeafValue(varValue,&(ptr->), (void *)ptr-> , newVarValue, RE_INT);
  else  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else
    return UNDEFINED_VARIABLE_MAP_ERR;
  return i;

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return i;
  }

  if (ptr == NULL)
    return NULL_VALUE_ERR;
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return i;
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else
    return UNDEFINED_VARIABLE_MAP_ERR;
  return i;

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return i;
  }

  if (ptr == NULL)
    return NULL_VALUE_ERR;
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return i;
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else
    return UNDEFINED_VARIABLE_MAP_ERR;
  return i;

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return i;
  }

  if (ptr == NULL)
    return NULL_VALUE_ERR;
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return i;
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else
    return UNDEFINED_VARIABLE_MAP_ERR;
  return i;

}


int
getSetValFrom(char *varMap, _t **inptr, char **varValue, void *newVarValue)
{
  char varName[NAME_LEN];
  char *varMapCPtr;
  int i;
  _t *ptr;

  ptr = *inptr;

  if (varMap == NULL) {
    i = getSetLeafValue(varValue,inptr, (void *) inptr, newVarValue, RE_PTR);
    return i;
  }

  if (ptr == NULL)
    return NULL_VALUE_ERR;
  i = getVarNameFromVarMap(varMap, varName, &varMapCPtr);
  if (i != 0)
    return i;
  if (!strcmp(varName, "") )
      g;
  else  if (!strcmp(varName, "") )
      g;
  else
    return UNDEFINED_VARIABLE_MAP_ERR;
  return i;

}
*****************/

int
getAllSessionVarValue( ruleExecInfo_t *rei, keyValPair_t *varKeyVal ) {
    int i, status;
    char *varValue;
    char *lastVar = NULL; 	/* last var that has data */

    if ( varKeyVal == NULL || rei == NULL ) {
        rodsLog( LOG_ERROR,
                 "getAllSessionVarValue: input rei or varKeyVal is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    for ( i = 0; i < coreRuleVarDef.MaxNumOfDVars; i++ ) {
        if ( lastVar == NULL || strcmp( lastVar, coreRuleVarDef.varName[i] ) != 0 ) {
            status = getSessionVarValue( "", coreRuleVarDef.varName[i], rei,
                                         &varValue );
            if ( status >= 0 && varValue != NULL ) {
                lastVar = coreRuleVarDef.varName[i];
                addKeyVal( varKeyVal, lastVar, varValue );
                free( varValue );
            }
        }
    }
    return 0;
}

int
getSessionVarValue( char *action, char *varName, ruleExecInfo_t *rei,
                    char **varValue ) {
    Region *r = make_region( 0, NULL );
    char *varMap = NULL;
    int vinx = getVarMap( action, varName, &varMap, 0 );
    while ( vinx >= 0 ) {
        Res *res;
        int i = getVarValue( varMap, rei, &res, r );
        free( varMap );
        varMap = NULL;
        if ( i != NULL_VALUE_ERR ) {
            if ( i >= 0 ) {
                *varValue = convertResToString( res );
            }
            region_free( r );
            return i;
        }
        vinx = getVarMap( action, varName, &varMap, vinx + 1 );
    }
    free( varMap );
    region_free( r );
    return vinx;
}


