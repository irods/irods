/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncInq.h
 */

#ifndef NC_INQ_H
#define NC_INQ_H

/* This is a NETCDF API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "ncOpen.h"
#include "ncInqId.h"
#include "ncGetVarsByType.h"

#define NC_VAR_TRANS_SZ         (4*1024*1024)

/* definition for paramType, can be or'ed to inquire more than 1 type */

#define NC_VAR_TYPE            0x1     /* nc variable */
#define NC_DIM_TYPE            0x2     /* nc dimension */
#define NC_ATT_TYPE            0x4     /* nc attribute */
#define NC_ALL_TYPE     (NC_VAR_TYPE|NC_DIM_TYPE|NC_ATT_TYPE) /* all types */

/* definition for flags */
#define NC_ALL_FLAG		0x1	/* inquire all items in each type, 
					 * myid and name are ignored */ 
typedef struct {
    int paramType;
    int ncid;
    int myid;           /* for NC_ALL_FLAG == 0, the id of the type */
    int flags;         
    char name[MAX_NAME_LEN];  /* for NC_ALL_FLAG == 0, the name */
    keyValPair_t condInput;
} ncInqInp_t;
   
#define NcInqInp_PI "int paramType; int ncid; int myId; int flags; str name[MAX_NAME_LEN]; struct KeyValPair_PI;"

typedef struct {
    rodsLong_t arrayLen;
    int id;
    int myint;	/* used to store the dim id of the target if dump to a file */
    char name[LONG_NAME_LEN];
} ncGenDimOut_t;

#define NcGenDimOut_PI "double arrayLen; int id; int myint; str name[LONG_NAME_LEN];"

typedef struct {
    int dataType;
    int id;
    int length;
    int myint;  /* not used */
    char name[LONG_NAME_LEN];
    ncGetVarOut_t value;
} ncGenAttOut_t;

#define NcGenAttOut_PI "int dataType; int id; int length;  int myint; str name[LONG_NAME_LEN]; struct NcGetVarOut_PI;"

typedef struct {
    int natts;
    int dataType;
    int id;
    int nvdims; 
    int myint;  /* used to store the var id of the target if dump to a file */
    int flags;  /* not used */
    char name[LONG_NAME_LEN];
    ncGenAttOut_t *att;		/* array of natts length */
    int *dimId;			/* arrays of dim id */
} ncGenVarOut_t;

#define NcGenVarOut_PI "int natts; int dataType; int id; int nvdims; int myint; int flags; str name[LONG_NAME_LEN]; struct *NcGenAttOut_PI(natts); int *dimId(nvdims);"

#define UNLIMITED_DIM_INX	-1	/* used in msiNcGetDimNameInInqOut
					 * to inq name of unlimdim */
typedef struct {
    int ndims;
    int nvars;
    int ngatts;	/* number of global gttr */
    int unlimdimid;
    int format;		/* one of NC_FORMAT_CLASSIC, NC_FORMAT_64BIT, 
                         * NC_FORMAT_NETCDF4, NC_FORMAT_NETCDF4_CLASSIC */
    int myint;  /* not used */
    ncGenDimOut_t *dim;		/* array of ndims length */
    ncGenVarOut_t *var;		/* array of ngvars length */
    ncGenAttOut_t *gatt;	/* array of ngatts length */
} ncInqOut_t;

#define NcInqOut_PI "int ndims; int nvars; int ngatts; int unlimdimid; int format; int myint; struct *NcGenDimOut_PI(ndims); struct *NcGenVarOut_PI(nvars); struct *NcGenAttOut_PI(ngatts);"

#define MAX_NUM_VAR	64
typedef struct {
    char subsetVarName[LONG_NAME_LEN];
    int start;
    int stride;
    int end;
    char startStr[NAME_LEN];
    char endStr[NAME_LEN];
} ncSubset_t;

typedef struct {
    int numVar;    /* number of variables in varName */
    char varName[MAX_NUM_VAR][LONG_NAME_LEN];  
    int numSubset;	  /* number of ncSubset */
    ncSubset_t ncSubset[MAX_NUM_VAR];
} ncVarSubset_t;

#if defined(RODS_SERVER) && defined(NETCDF_API)
#define RS_NC_INQ rsNcInq
/* prototype for the server handler */
int
rsNcInq (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut);
int
rsNcInqDataObj (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut);
int
rsNcInqColl (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut);
int
_rsNcInq (rsComm_t *rsComm, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut);
#else
#define RS_NC_INQ NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* rcNcInq - general netcdf inq for id (equivalent to nc_inq + nc_inq_format
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   ncInqInp_t struct:
 *     ncid - the the ncid.   
 * OutPut - ncInqOut_t.
 */
/* prototype for the client call */
int
rcNcInq (rcComm_t *conn, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut);

int
initNcInqOut (int ndims, int nvars, int ngatts, int unlimdimid, int format,
ncInqOut_t **ncInqOut);
int
freeNcInqOut (ncInqOut_t **ncInqOut);
int
dumpNcInqOut (rcComm_t *conn, char *fileName, int ncid, int dumpVarLen,
ncInqOut_t *ncInqOut);
int
prFirstNcLine (char *objPath);
int
prNcHeader (rcComm_t *conn, int ncid, int noattr,
ncInqOut_t *ncInqOut);
int
prSingleDimVar (rcComm_t *conn, int ncid, int varInx, 
int itemsPerLine, int printAsciTime, ncInqOut_t *ncInqOut);
int
prNcDimVar (rcComm_t *conn, char *fileName, int ncid, int printAsciTime,
ncInqOut_t *ncInqOut);
int
prNcVarData (rcComm_t *conn, char *fileName, int ncid, int printAsciTime,
ncInqOut_t *ncInqOut, ncVarSubset_t *ncVarSubset);
int
getSingleNcVarData (rcComm_t *conn, int ncid, int varInx, ncInqOut_t *ncInqOut,
ncVarSubset_t *ncVarSubset, ncGetVarOut_t **ncGetVarOut, rodsLong_t *start, 
rodsLong_t *stride, rodsLong_t *count);
int
getNcTypeStr (int dataType, char *outString);
int
ncValueToStr (int dataType, void **value, char *outString);
int
dumpNcInqOutToNcFile (rcComm_t *conn, int srcNcid, int noattrFlag,
ncInqOut_t *ncInqOut, char *outFileName);
int
dumpSubsetToFile (rcComm_t *conn, int srcNcid, int noattrFlag,
ncInqOut_t *ncInqOut, ncVarSubset_t *ncVarSubset, char *outFileName);
int
getAndPutVarToFile (rcComm_t *conn, int srcNcid, int srcVarid, int ndim,
int dataType, size_t *lstart, ptrdiff_t *lstride, size_t *lcount, 
int ncid, int varid);
int
ncFormatToCmode (int format);
int
closeAndRmNeFile (int ncid, char *outFileName);
int
printNice (char *str, char *margin, int charPerLine);
int
parseNcSubset (ncSubset_t *ncSubset);
int
parseVarStrForSubset (char *varStr, ncVarSubset_t *ncVarSubset);
int
parseSubsetStr (char *subsetStr, ncVarSubset_t *ncVarSubset);
int
timeToAsci (time_t mytime, char *asciTime);
int
asciToTime (char *asciTime, time_t *mytime);
int
resolveSubsetVar (rcComm_t *conn, int srcNcid, ncInqOut_t *ncInqOut,
ncVarSubset_t *ncVarSubset);
int
ncInq (ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut);
int
inqAtt (int ncid, int varid, int natt, char *name, int id, int allFlag,
ncGenAttOut_t *attOut);
int
getAttValue (int ncid, int varid, char *name, int dataType, int length,
ncGetVarOut_t *value);
unsigned int
getNcIntVar (int ncid, int varid, int dataType, rodsLong_t inx);
int
getTimeInxInVar (ncInqOut_t *ncInqOut, int varid);
unsigned int
ncValueToInt (int dataType, void **invalue);
rodsLong_t
getTimeStepSize (ncInqOut_t *ncInqOut);
#ifdef  __cplusplus
}
#endif

#endif	/* NC_INQ_H */
