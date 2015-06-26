/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include "iFuse.Lib.hpp"
#include "iFuse.Lib.RodsClientAPI.hpp"
#include "iFuse.Lib.Conn.hpp"
#include "iFuse.Lib.Fd.hpp"
#include "iFuse.Lib.Util.hpp"
#include "rodsClient.h"

static rodsEnv g_iFuseEnv;
static iFuseOpt_t g_iFuseOpt;

rodsEnv *iFuseLibGetRodsEnv() {
    return &g_iFuseEnv;
}

void iFuseLibSetRodsEnv(rodsEnv *pEnv) {
    memcpy(&g_iFuseEnv, pEnv, sizeof (rodsEnv));
}

iFuseOpt_t *iFuseLibGetOption() {
    return &g_iFuseOpt;
}

void iFuseLibSetOption(iFuseOpt_t *pOpt) {
    memcpy(&g_iFuseOpt, pOpt, sizeof (iFuseOpt_t));
}

void iFuseLibInit() {
    iFuseRodsClientInit();
    iFuseConnInit();
    
    iFuseFdInit();
    iFuseDirInit();
}

void iFuseLibDestroy() {
    iFuseDirDestroy();
    iFuseFdDestroy();
    
    iFuseConnDestroy();
    iFuseRodsClientDestroy();
}
