/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* objInfo.h - header file for general Obj Info
 */

/* dataObjInfo_t is for info about a data object.
   intKeyStrVal_t is a generic (integer keyword)/(string value) pair.
   It can be used for many things. For example, for the input "condition",
   the condKeywd_t is used for keyword. But it can also be used to
   input parameters for rcat registration.

   The routine addIntKeywdStrVal() in rcMisc.c can be used to add a
   keyword/value pair and getValByIntKeywd() can be used to get the
   value string based on a keyword.
*/

#ifndef OBJ_INFO_H__
#define OBJ_INFO_H__

#include "rodsType.h"
#include "rodsUser.h"

/* this defines the "copies" condition */
#define ALL_COPIES      -1      /* "all" */

/* defines some commonly used dataTypes */
#define GENERIC_DT_STR    "generic"
#define TAR_DT_STR        "tar file"
#define GZIP_TAR_DT_STR   "gzipTar"  // JMC - backport 4632
#define BZIP2_TAR_DT_STR  "bzip2Tar" // JMC - backport 4632
#define ZIP_DT_STR        "zipFile"  // JMC - backport 4633
#define MSSO_DT_STR       "msso file"
/* bundle are types for internal phybun use */ // JMC - backport 4658
#define TAR_BUNDLE_DT_STR       "tar bundle"       // JMC - backport 4658
#define GZIP_TAR_BUNDLE_DT_STR  "gzipTar bundle"   // JMC - backport 4658
#define BZIP2_TAR_BUNDLE_DT_STR "bzip2Tar bundle"  // JMC - backport 4658
#define ZIP_BUNDLE_DT_STR       "zipFile bundle"   // JMC - backport 4658

#define HAAW_DT_STR             "haaw file"
#define MAX_LINK_CNT    20      /* max number soft link in a path */


/* special collection */

typedef enum {         /* class of SpecColl */
    NO_SPEC_COLL,
    STRUCT_FILE_COLL,
    MOUNTED_COLL,
    LINKED_COLL
} specCollClass_t;

typedef enum {                /* structFile type */
    NONE_STRUCT_FILE_T = 0,   /* no known type */
    HAAW_STRUCT_FILE_T = 1,   /* the UK eScience structFile */
    TAR_STRUCT_FILE_T  = 2,   /* The tar structFile */
    MSSO_STRUCT_FILE_T = 3,   /* The workflow structFile */
} structFileType_t;

typedef enum {          /* specColl operation type */
    NOT_SPEC_COLL_OPR,
    NON_STRUCT_FILE_SPEC_COLL_OPR,
    STRUCT_FILE_SPEC_COLL_OPR,
    NORMAL_OPR_ON_STRUCT_FILE_COLL
} structFileOprType_t;

#define HAAW_STRUCT_FILE_STR      "haawStructFile"
#define TAR_STRUCT_FILE_STR       "tarStructFile"
#define MOUNT_POINT_STR           "mountPoint"
#define LINK_POINT_STR            "linkPoint"
#define INHERIT_PAR_SPEC_COLL_STR "inheritParentSpecColl"
#define MSSO_STRUCT_FILE_STR      "mssoStructFile"
#define MSO_STR                   "mso"

#define UNMOUNT_STR               "unmount"

typedef struct SpecColl {
    specCollClass_t collClass;
    structFileType_t type;
    char collection[MAX_NAME_LEN];  /* structured file or mounted collection */
    char objPath[MAX_NAME_LEN];      /* STRUCT_FILE_COLL-logical path of structFile
                                          * MOUNTED_COLL - NA
                                          */
    char resource[NAME_LEN];         /* the resource */
    char rescHier[MAX_NAME_LEN];     // the resource hierarchy
    char phyPath[MAX_NAME_LEN];      /* STRUCT_FILE_COLL-the phyPath of structFile
                                          * MOUNTED_COLL-the phyPath od mounted
                                          * directory
                                          */
    char cacheDir[MAX_NAME_LEN];     /* STRUCT_FILE_COLL-the directory where
                                          * the cache tree is kept
                                          */
    int cacheDirty;                  /* Whether the cache has been written */
    int replNum;
} specColl_t;

typedef enum {
    UNKNOWN_COLL_PERM,
    READ_COLL_PERM,
    WRITE_COLL_PERM
} specCollPerm_t;

typedef struct SpecCollCache {
    specCollPerm_t perm;
    specColl_t specColl;
    char collId[NAME_LEN];
    char ownerName[NAME_LEN];
    char ownerZone[NAME_LEN];
    char createTime[NAME_LEN];
    char modifyTime[NAME_LEN];
    struct SpecCollCache *next;
} specCollCache_t;

/* definition for replStatus (isDirty) */
#define OLD_COPY        0x0
#define NEWLY_CREATED_COPY      0x1
#define OPEN_EXISTING_COPY      0x10
#define FILE_PATH_HAS_CHG       0x20

/* keyValPair_t - str key, str value pair */
typedef struct KeyValPair {
    int len;
    char **keyWord;     /* array of keywords */
    char **value;       /* pointer to an array of values */
} keyValPair_t;

/* definition for flags in dataObjInfo_t */
#define NO_COMMIT_FLAG  0x1  /* used in chlModDataObjMeta and chlRegDataObj */

typedef struct DataObjInfo {
    char objPath[MAX_NAME_LEN];
    char rescName[NAME_LEN];
    char rescHier[MAX_NAME_LEN];   // The hierarchy of resources within which the object resides
    char dataType[NAME_LEN];
    rodsLong_t dataSize;
    char chksum[NAME_LEN];
    char version[NAME_LEN];
    char filePath[MAX_NAME_LEN];
    char dataOwnerName[NAME_LEN];
    char dataOwnerZone[NAME_LEN];
    int  replNum;
    int  replStatus;     /* isDirty flag */
    char statusString[NAME_LEN];
    rodsLong_t  dataId;
    rodsLong_t  collId;
    int  dataMapId;
    int flags;          /* used in chlModDataObjMeta and chlRegDataObj */
    char dataComments[LONG_NAME_LEN];
    char dataMode[SHORT_STR_LEN];
    char dataExpiry[TIME_LEN];
    char dataCreate[TIME_LEN];
    char dataModify[TIME_LEN];
    char dataAccess[NAME_LEN];
    int  dataAccessInx;
    int  writeFlag;
    char destRescName[NAME_LEN];
    char backupRescName[NAME_LEN];
    char subPath[MAX_NAME_LEN];
    specColl_t *specColl;
    int regUid;                /* the UNIX uid the registering user */
    int otherFlags;    /* not used for now */
    keyValPair_t condInput;
    char in_pdmo[MAX_NAME_LEN]; // If this is set then we are currently in a pdmo call at that level of hierarchy
    struct DataObjInfo *next;
    rodsLong_t rescId;
} dataObjInfo_t ;

/* collInfo_t definitions:
 * collInfo1:
 *   MOUNTED_COLL - physical directory path
 *   LINKED_COLL - linked logical path
 *   TAR_STRUCT_FILE_T - logical path of the tar file
 * collInfo2:
 *   MOUNTED_COLL - resource
 *   LINKED_COLL - none
 *   TAR_STRUCT_FILE_T - cacheDirPath;;;resource;;;cacheDirty
 */
typedef struct CollInfo {
    rodsLong_t collId;
    char collName[MAX_NAME_LEN];
    char collParentName[MAX_NAME_LEN];
    char collOwnerName[NAME_LEN];
    char collOwnerZone[NAME_LEN];
    int  collMapId;
    int  collAccessInx;
    char collComments[LONG_NAME_LEN];
    char collInheritance[LONG_NAME_LEN];
    char collExpiry[TIME_LEN];
    char collCreate[TIME_LEN];
    char collModify[TIME_LEN];
    char collAccess[NAME_LEN];
    char collType[NAME_LEN];
    char collInfo1[MAX_NAME_LEN];
    char collInfo2[MAX_NAME_LEN];
    keyValPair_t condInput;

    struct CollInfo *next;
} collInfo_t;

typedef struct RuleInfo {
    int TDB;
} ruleInfo_t;

/* inxIvalPair_t - int index, int value pair */

typedef struct InxIvalPair {
    int len;
    int *inx;           /* pointer to an array of int index */
    int *value;       /* pointer to an array of int value values */
} inxIvalPair_t;

/* inxValPair_t - int index, str value pair */

typedef struct InxValPair {
    int len;
    int *inx;          /* pointer to an array of int index */
    char **value;       /* pointer to an array of str value values */
} inxValPair_t;

/* strArray_t - just a single array */
typedef struct StrArray {
    int len;
    int size;
    char *value;        /* char aray of [len][size] */
} strArray_t;

/* intArray_t - just a single array */
typedef struct IntArray {
    int len;
    int *value;        /* int aray of [len] */
} intArray_t;


/* definition for RescTypeDef */

typedef enum {  /* resource category */
    FILE_CAT,
    DB_CAT
} rescCat_t;

#define DEFAULT_FILE_MODE       0600
#define DEFAULT_DIR_MODE        0750

/* definition for chkPathPerm */

#define DISALLOW_PATH_REG       0       /* disallow path registration */ // JMC - backport 4774
#define NO_CHK_PATH_PERM            1 // JMC - backport 4758
#define DO_CHK_PATH_PERM        2 // JMC - backport 4774
#define CHK_NON_VAULT_PATH_PERM 3    /* allow reg of user's vault path */// JMC - backport 4774

#define DISALLOW_PATH_REG_STR       "disallowPathReg"     // JMC - backport 4774
#define NO_CHK_PATH_PERM_STR        "noChkPathPerm"       // JMC - backport 4774
#define DO_CHK_PATH_PERM_STR        "doChkPathPerm"       // JMC - backport 4774
#define CHK_NON_VAULT_PATH_PERM_STR "chkNonVaultPathPerm" // JMC - backport 4774

/* definition for stageFlag to specify whether staging is required */
#define NO_STAGING        0
#define STAGE_SRC         1
#define SYNC_DEST         2

/* definition for trash policy */

#define DO_TRASH_CAN    0
#define NO_TRASH_CAN    1

typedef enum {
    NO_CREATE_PATH,
    CREATE_PATH
} createPath_t;

/* definition for classType */

#define CACHE_CL        0
#define ARCHIVAL_CL     1
#define BUNDLE_CL       2
#define COMPOUND_CL     3
#define DATABASE_CL     4

#define PRIMARY_FLAG	0x8000		/* primary class when this bit is set */
// JMC - legacy resource - typedef struct RescClass {
//    char *className;
//    int classType;
//} rescClass_t;

/* transStat_t is being replaced by transferStat_t because of the 64 bits
 * padding */
typedef struct {
    int numThreads;
    rodsLong_t bytesWritten;
} transStat_t;

typedef struct {
    int numThreads;
    int flags;          /* padding to 64 bits */
    rodsLong_t bytesWritten;
} transferStat_t;

#define FILE_CNT_PER_STAT_OUT   10      /* the default file count per 
    * collOprStat output */
typedef struct {
    int filesCnt;
    int totalFileCnt;
    rodsLong_t bytesWritten;
    char lastObjPath[MAX_NAME_LEN];
} collOprStat_t;

/* tagStruct_t - tagged keyword structure
   preTag  defines the reg exp to find beginning of value
   postTag defines the reg exp to be checked to find end of value
   the value found between the tags is associated with the KeyWord*/
typedef struct TagStruct {
    int len;
    char **preTag;     /* array of prestring tag */
    char **postTag;     /* array of poststring tag */
    char **keyWord;       /* pointer to an array of KeyWords */
} tagStruct_t;

typedef struct Subfile {
    rodsHostAddr_t addr;
    char subFilePath[MAX_NAME_LEN];
    int mode;
    int flags;
    rodsLong_t offset;
    specColl_t *specColl;
} subFile_t;

typedef struct StructFileTypeDef {
    char *typeName;
    structFileType_t type;
} structFileTypeDef_t;

#endif  // OBJ_INFO_H__

