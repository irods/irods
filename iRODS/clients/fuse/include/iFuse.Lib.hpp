/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_LIB_HPP
#define	IFUSE_LIB_HPP

#include "rodsClient.h"

typedef struct IFuseExtendedOpt {
    bool fuse;
    char *opt;
    struct IFuseExtendedOpt *next;
} iFuseExtendedOpt_t;

typedef struct IFuseOpt {
    char *program;
    bool debug;
    bool nonempty;
    bool foreground;
    bool bufferedFS;
    bool preload;
    int maxConn;
    int connTimeoutSec;
    int connKeepAliveSec;
    char *mountpoint;
    iFuseExtendedOpt_t *extendedOpts;
} iFuseOpt_t;

rodsEnv *iFuseLibGetRodsEnv();
void iFuseLibSetRodsEnv(rodsEnv *pEnv);
iFuseOpt_t *iFuseLibGetOption();
void iFuseLibSetOption(iFuseOpt_t *pOpt);

void iFuseLibInit();
void iFuseLibDestroy();


#endif	/* IFUSE_LIB_HPP */

