/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*****************************************************************************

   This file contains all the  externs of helper routines 
   used by  ICAT midlevel  calls

*****************************************************************************/

#ifndef ICAT_MIDLEVEL_HELPERS_H
#define ICAT_MIDLEVEL_HELPERS_H

#include "icatStructs.h"
#include "icatLowLevel.h"

char *cmlArrToSepStr(char *str, 
		  char *preStr,
		  char *arr[], 
		  int   arrLen, 
		  char *sep, 
		  int  maxLen,
		  int  opt);

char *cmlArr2ToSepStr(char *str, 
		   char *preStr,
		   char *arr[], 
		   char *arr2[],
		   int   arrLen, 
		   char *sep, 
		   int  maxLen);

char *cmlArr2ToSep2Str(char *str, 
		   char *preStr,
		   char *arr[], 
		   char *arr2[],
		   int   arrLen, 
		   char *sep, 
		   char *sep2, 
		   int  maxLen);

char *cmlArraysToStrWithBind(char *str, 
		   char *preStr,
		   char *arr[], 
		   char *arr2[],
		   int   arrLen, 
		   char *sep, 
		   char *sep2, 
		   int  maxLen);

int cmlGetRowFromSql (char *sql, 
		   char *cVal[], 
		   int cValSize[], 
		   int numOfCols,
		   icatSessionStruct *icss);

#endif /* ICAT_MIDLEVEL_HEPLERS_H */


