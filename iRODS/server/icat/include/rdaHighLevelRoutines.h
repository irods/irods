/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***
 *****************************************************************************/

#ifndef RDA_HIGHLEVEL_ROUTINES_H
#define RDA_HIGHLEVEL_ROUTINES_H

int rdaOpen(char *rdaName);
int rdaClose();
int rdaCommit();
int rdaRollback();
int rdaIsConnected();
int rdaSqlNoResults(char *sql, char *parm[], int nparms);
int rdaSqlWithResults(char *sql, char *parm[], int nparms, char **outBuf);
int rdaCheckAccess(char *rdaName, rsComm_t *rsComm);

int rdaDebug(char *debugMode);

#endif /* RDA_HIGHLEVEL_ROUTINES_H */
