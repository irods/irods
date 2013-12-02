/*
 * sysBackupMS.h
 *
 *  Header file for sysBackupMS.c
 *
 *  Copyright (c), The DICE Foundation.
 *  For more information please refer to files in the COPYRIGHT directory.
 *
 */



#ifndef SYSBACKUPMS_H_
#define SYSBACKUPMS_H_

#include "rods.h"
#include "reGlobalsExtern.h"

#define BCKP_COLL_NAME	"system_backups"

int msiServerBackup(msParam_t *options, msParam_t *keyValOut, ruleExecInfo_t *rei);
int loadDirToLocalResc(ruleExecInfo_t *rei, char *dirPath, size_t offset,
		char *resDirPath, char *timestamp, char *dbPath);
char *getDBHomeDir();
int getDefaultLocalRescInfo(rescInfo_t **rescInfo);

#endif /* SYSBACKUPMS_H_ */
