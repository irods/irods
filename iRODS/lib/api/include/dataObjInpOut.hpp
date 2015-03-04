/**
 * @file  dataObjInpOut.h
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjInpOut.h - header file for generic dataObj type input/output struct.
 */



#ifndef DATA_OBJ_INP_OUT_HPP
#define DATA_OBJ_INP_OUT_HPP


#include "rodsDef.h"

#if defined(aix_platform)
#ifndef _AIX_PTHREADS_D7
#define pthread_mutexattr_default NULL
#define pthread_condattr_default NULL
#define pthread_attr_default NULL
#endif  /* _AIX_PTHREADS_D7 */
#else   /* aix_platform */
#define pthread_mutexattr_default NULL
#define pthread_condattr_default NULL
#define pthread_attr_default NULL
#endif  /* aix_platform */

typedef struct {
    int portNum;       /* the port number */
    int cookie;
    int sock;           /* The server's sock number. no meaning for client */
    int windowSize;
    char hostAddr[LONG_NAME_LEN];
} portList_t;

/**
 * \var dataObjInp_t
 * \brief Input struct for Data object operation
 * \since 1.0
 *
 * \remark none
 *
 * \note
 * Elements of dataObjInp_t:
 * \li char objPath[MAX_NAME_LEN] - full path of the data object.
 * \li int createMode - the file mode of the data object.
 * \li int openFlags - the flags for the I/O operation,
 *      Valid flags are O_RDONLY, O_WRONLY, O_RDWR and O_TRUNC.
 *      Also used for specCollInx in rcQuerySpecColl.
 * \li rodsLong_t offset - the offset.
 * \li rodsLong_t dataSize - the size of the data object.
 * \li int numThreads - the number of threads to use.
 * \li int oprType - the type of operation.
 * \li specColl_t *specColl - a pointer to a specColl_t if this path is
 *      in a special collection (e.g. mounted collection).
 * \li keyValPair_t condInput -  keyword/value pair input. Valid keywords
 *      depend on the API.
 *
 * \sa none
 * \bug  no known bugs
 */

typedef struct DataObjInp {
    char objPath[MAX_NAME_LEN];
    int createMode;
    int openFlags;      /* used for specCollInx in rcQuerySpecColl */
    rodsLong_t offset;
    rodsLong_t dataSize;
    int numThreads;
    int oprType;
    specColl_t *specColl;
    keyValPair_t condInput;   /* include chksum flag and value */
} dataObjInp_t;

/**
 * \var openedDataObjInp_t
 * \brief Input struct for Opened data object operation
 * \since 1.0
 *
 * \remark none
 *
 * \note
 * Elements of openedDataObjInp_t:
 * \li int l1descInx - the opened data object descriptor from rcDataObjOpen
 *        or rcDataObjCreate
 * \li int len - the length (number of bytes) to read/write.
 * \li int whence - vaild only for rcDataObjLseek (similar to lseek of UNIX).
 *      Valid values are SEEK_SET, SEEK_CUR and SEEK_END.
 * \li int oprType - The operation type. Valid values are - PUT_OPR, GET_OPR,
 *      REPLICATE_OPR, ... See dataObjInpOut.h for more.
 * \li rodsLong_t offset - the offset.
 * \li rodsLong_t bytesWritten - Number of bytes wriitten (vaild for
 *      rcDataObjClose).
 * \li keyValPair_t condInput -  keyword/value pair input. Valid keywords
 *      depend on the API.
 *
 * \sa none
 * \bug  no known bugs
 */

typedef struct OpenedDataObjInp {
    int l1descInx;              /* for read, write, close */
    int len;                    /* length of operation for read, write */
    int whence;                 /* used for lseek */
    int oprType;
    rodsLong_t offset;
    rodsLong_t bytesWritten;    /* for close */
    keyValPair_t condInput;   /* include chksum flag and value */
} openedDataObjInp_t;

typedef struct portalOprOut {
    int status;
    int l1descInx;
    int numThreads;
    char chksum[NAME_LEN];
    portList_t portList;
} portalOprOut_t;

typedef struct DataOprInp {
    int oprType;
    int numThreads;
    int srcL3descInx;
    int destL3descInx;
    int srcRescTypeInx;
    int destRescTypeInx;
    /* XXXXXXX offset and dataSize moved to here because of problem with
     * 64 bit susue linux that condInput has pointer's in it which
     * cause condInput to be aligned at 64 the beginning and end of condInput */
    rodsLong_t offset;
    rodsLong_t dataSize;
    keyValPair_t condInput;
} dataOprInp_t;

/**
 * \var collInp_t
 * \brief Input struct for collection operation
 * \since 1.0
 *
 * \remark none
 *
 * \note
 * Elements of collInp_t:
 * \li char collName[MAX_NAME_LEN] - full path of the collection.
 * \li int flags - Flags. Used only by rcOpenCollection.
 * \li int oprType - operation type. not used.
 * \li keyValPair_t condInput -  keyword/value pair input. Valid keywords
 *      depend on the API.
 *
 * \sa none
 * \bug  no known bugs
 */

typedef struct CollInp {
    char collName[MAX_NAME_LEN];
    int flags;
    int oprType;
    keyValPair_t condInput;
} collInp_t;

/* definition for oprType in dataObjInp_t, portalOpr_t and l1desc_t */

#define DONE_OPR                9999
#define PUT_OPR                 1
#define GET_OPR                 2
#define SAME_HOST_COPY_OPR      3
#define COPY_TO_LOCAL_OPR       4
#define COPY_TO_REM_OPR         5
#define REPLICATE_OPR           6
#define REPLICATE_DEST          7
#define REPLICATE_SRC           8
#define COPY_DEST               9
#define COPY_SRC                10
#define RENAME_DATA_OBJ         11
#define RENAME_COLL             12
#define MOVE_OPR                13
#define RSYNC_OPR               14
#define PHYMV_OPR               15
#define PHYMV_SRC               16
#define PHYMV_DEST              17
#define QUERY_DATA_OBJ          18
#define QUERY_DATA_OBJ_RECUR    19
#define QUERY_COLL_OBJ          20
#define QUERY_COLL_OBJ_RECUR    21
#define RENAME_UNKNOWN_TYPE     22
#define REMOTE_ZONE_OPR         24
#define UNREG_OPR               26

/* definition for openType in l1desc_t */
#define CREATE_TYPE             1
#define OPEN_FOR_READ_TYPE      2
#define OPEN_FOR_WRITE_TYPE     3

typedef struct PortalOpr {
    int oprType;
    dataOprInp_t dataOprInp;
    portList_t portList;
    char shared_secret[ NAME_LEN ]; // shared secret for encryption
} portalOpr_t;

/* definition for flags */
#define STREAMING_FLAG          0x1
#define NO_CHK_COPY_LEN_FLAG    0x2

typedef struct TransferHeader {
    int oprType;
    int flags;
    rodsLong_t offset;
    rodsLong_t length;
} transferHeader_t;

#endif  /* DATA_OBJ_INP_OUT_H */
