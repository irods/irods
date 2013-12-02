/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***
 *****************************************************************************/

#ifndef DBO_HIGHLEVEL_ROUTINES_H
#define DBO_HIGHLEVEL_ROUTINES_H

int dbrOpen(char *dbrName);
int dboExecute(rsComm_t *rsComm, char *dbrName, char *dboName, char *dborName,
	       int dobrOption, char *outBuf,
	       int maxOutBuf, char *args[10]);
int dbrClose(char *dbrName);
int dbrCommit(rsComm_t *rsComm, char *dbrName);
int dbrRollback(rsComm_t *rsComm, char *dbrName);

#endif /* DBO_HIGHLEVEL_ROUTINES_H */
