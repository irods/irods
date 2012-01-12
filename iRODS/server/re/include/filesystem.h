/* For copyright information please refer to files in the COPYRIGHT directory
 */


#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "datetime.h"

int getModifiedTime(char *fn, time_type *timestamp);
char *getRuleBasePath(char *ruleBaseName, char rulesFileName[MAX_NAME_LEN]);
void getResourceName(char buf[1024], char *rname);


#endif /* FILESYSTEM_H_ */
