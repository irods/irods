#ifndef IRODS_LS_UTIL_H
#define IRODS_LS_UTIL_H

#include "irods/rodsType.h"

struct RcComm;
struct RodsEnvironment;
struct RodsArguments;
struct RodsPathInp;
struct RodsPath;
struct CollEnt;
struct SpecColl;
struct GenQueryInp;
struct GenQueryOut;

#ifdef __cplusplus
extern "C" {
#endif

int lsUtil(struct RcComm* conn,
           struct RodsEnvironment* myRodsEnv,
           struct RodsArguments* myRodsArgs,
           struct RodsPathInp* rodsPathInp);

int lsDataObjUtil(struct RcComm* conn,
                  struct RodsPath* srcPath,
                  struct RodsArguments* rodsArgs);

int printLsStrShort(char* srcPath);

int initCondForLs(struct GenQueryInp* genQueryInp);

int lsDataObjUtilLong(struct RcComm* conn,
                      char* srcPath,
                      struct RodsArguments* rodsArgs,
                      struct GenQueryInp* genQueryInp);

int printLsLong(struct RcComm* conn,
                struct RodsArguments* rodsArgs,
                struct GenQueryOut* genQueryOut);

int printLsShort(struct RcComm* conn,
                 struct RodsArguments* rodsArgs,
                 struct GenQueryOut* genQueryOut);

int lsCollUtil(struct RcComm* conn,
               struct RodsPath* srcPath,
               struct RodsEnvironment* myRodsEnv,
               struct RodsArguments* rodsArgs);

int printDataAcl(struct RcComm* conn, char* dataId);

int printCollAcl(struct RcComm* conn, char* collId);

int printCollInheritance(struct RcComm* conn, char* collName);

int lsSpecDataObjUtilLong(struct RodsPath* srcPath,
                          struct RodsArguments* rodsArgs);

int printSpecLsLong(char* objPath,
                    char* ownerName,
                    char* objSize,
                    char* modifyTime,
                    struct SpecColl* specColl,
                    struct RodsArguments* rodsArgs);

void printCollOrDir(char* myName,
                    objType_t myType,
                    struct RodsArguments* rodsArgs,
                    struct SpecColl* specColl);

int printDataCollEnt(struct CollEnt* collEnt, int flags);

int printDataCollEntLong(struct CollEnt* collEnt, int flags);

int printCollCollEnt(struct CollEnt* collEnt, int flags);

int lsSubfilesInBundle(struct RcComm* conn, char* srcPath);

int setSessionTicket(struct RcComm* myConn, char* ticket);

#ifdef __cplusplus
} // extern "C"
#endif

#endif	// IRODS_LS_UTIL_H
