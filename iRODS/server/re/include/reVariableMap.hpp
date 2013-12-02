/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef REVARIABLEMAP_H_

#include "restructs.h"

int setVarValue(char *varMap, ruleExecInfo_t *rei, Res *newVarValue);

int getIntLeafValue(Res **varValue, int leaf, Region *r);
int getLongLeafValue(Res **varValue, rodsLong_t leaf, Region *r);
int getStrLeafValue(Res **varValue, char *leaf, Region *r);
int getPtrLeafValue(Res **varValue, void *leaf, bytesBuf_t *buffer, char *irodsType, Region *r);

int setIntLeafValue(int *leafPtr, Res *newVarValue);
int setLongLeafValue(rodsLong_t *leafPtr, Res *newVarValue);
int setStrLeafValue(char *leafPtr, size_t len, Res *newVarValue);
int setStrDupLeafValue(char **leafPtr, Res *newVarValue);
int setStructPtrLeafValue(void **leafPtr, Res *newVarValue);
int setBufferPtrLeafValue(bytesBuf_t **leafPtr, Res *newVarValue);
int getVarMap(char *action, char *varName, char **varMap, int index);

ExprType *getVarType(char *varMap, Region *r);

int getVarValue(char *varMap, ruleExecInfo_t *rei, Res **varValue, Region *r);
int getVarNameFromVarMap(char *varMap, char *varName, char **varMapCPtr);

#define REVARIABLEMAP_H_




#endif /* REVARIABLEMAP_H_ */
