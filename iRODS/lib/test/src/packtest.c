/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* packtest.c - test the basic packing routines */

#include "rodsClient.h" 
struct myTest {
    char c1[17];
    char c2[18];
    int int1;
};

struct myStruct {
    int i1;
    int i2;
    rodsLong_t l1;
    keyValPair_t condInput;
};
 
#define TEST_PI  "int gopID; int gfid; int gobjID[OBJID_DIM]; str *gfullpath; str *dummyParent; int nGroupMembers; struct *h5Group_PF[nGroupMembers]; int nDatasetMembers; struct *h5Dataset_PF[nDatasetMembers]; int nattributes; struct *h5Attribute_PF[nattributes]; struct h5error_PF;"

int
writePackedRes (bytesBuf_t *packedResult, char *outFile);

int
main(int argc, char **argv)
{
    int status;
    packItem_t *packItemHead = NULL;
    startupPack_t myStartupPack, *outStartupPack;
    rError_t myError, *outError;
    bytesBuf_t *packedResult;
    genQueryOut_t myQueryOut, *outQueryOut;
    char *tmpValue;
    int i;
    modDataObjMeta_t modDataObjMetaInp, *outModDataObjMeta;
    dataObjInfo_t dataObjInfo;
    keyValPair_t regParam;
    authCheckInp_t authCheckInp, *outAuthCheckInp;
    genQueryInp_t genQueryInp, *outGenQueryInp;
    irodsProt_t irodsProt = XML_PROT;
    struct myStruct myStruct1;

    memset (&genQueryInp, 0, sizeof (genQueryInp));
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, "=xyz");
    addInxIval (&genQueryInp.selectInp, COL_COLL_ID, 1);
    genQueryInp.maxRows = 1234567890;
    status = packStruct (&genQueryInp, &packedResult, "GenQueryInp_PI",
      NULL, 0, irodsProt);

    printf ("packStruct GenQueryInp_PI, status = %d\n", status);
    if (irodsProt == XML_PROT)
        writePackedRes (packedResult, "queryInp.out");

    status = unpackStruct (packedResult->buf, (void **) &outGenQueryInp,
      "GenQueryInp_PI", NULL, irodsProt);

    printf ("unpackStruct of empty GenQueryInp_PI, status = %d\n", status);


    memset (&authCheckInp, 0, sizeof (authCheckInp));
    status = packStruct (&authCheckInp, &packedResult, "authCheckInp_PI",
      NULL, 0, irodsProt);

    printf ("packStruct of empty authCheckInp_PI, status = %d\n", status);
    if (irodsProt == XML_PROT)
        writePackedRes (packedResult, "auth.out");

    status = unpackStruct (packedResult->buf, (void **) &outAuthCheckInp,
      "authCheckInp_PI", NULL, irodsProt);

    printf ("unpackStruct of empty authCheckInp_PI, status = %d\n", status);

    memset (&dataObjInfo, 0, sizeof (dataObjInfo_t));
    memset (&regParam, 0, sizeof (keyValPair_t));

    strcpy (dataObjInfo.objPath, "/tmp/thisIsATest");
    modDataObjMetaInp.dataObjInfo = &dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;

    status = packStruct (&modDataObjMetaInp, &packedResult, "ModDataObjMeta_PI",
      NULL, 0, irodsProt);

    printf ("packStruct of ModDataObjMeta_PI, status = %d\n", status);
    if (irodsProt == XML_PROT)
        writePackedRes (packedResult, "modeData.out");

    status = unpackStruct (packedResult->buf, (void **) &outModDataObjMeta, 
      "ModDataObjMeta_PI", NULL, irodsProt);

    printf ("unpackStruct of ModDataObjMeta_PI, status = %d\n", status);
    
    memset (&myQueryOut, 0, sizeof (myQueryOut));
    /* pack and unpack empty struct */

    status = packStruct (&myQueryOut, &packedResult, "GenQueryOut_PI",
      NULL, 0, irodsProt);
    printf ("packStruct empty myQueryOut status = %d\n", status);
    if (irodsProt == XML_PROT)
        writePackedRes (packedResult, "genq.out");

    status = unpackStruct (packedResult->buf, (void **) &outQueryOut,
      "GenQueryOut_PI", NULL, irodsProt);

    printf ("unpackStruct empty myQueryOut status = %d\n", status);

    memset (&myQueryOut, 0, sizeof (myQueryOut));
    myQueryOut.rowCnt = 2;
    myQueryOut.attriCnt = 4;

    myQueryOut.sqlResult[0].attriInx = 10;
    myQueryOut.sqlResult[0].len = 20;
    myQueryOut.sqlResult[0].value = tmpValue = malloc (20*2);
    sprintf (tmpValue, "value 0,1");
    tmpValue += 20;
    sprintf (tmpValue, "value 0,2");

    myQueryOut.sqlResult[1].attriInx = 20;
    myQueryOut.sqlResult[1].len = 30;
    myQueryOut.sqlResult[1].value = tmpValue = malloc (30*2);
    sprintf (tmpValue, "value 1,1");
    tmpValue += 30;
    sprintf (tmpValue, "value 1,2");

    myQueryOut.sqlResult[2].attriInx = 30;
    myQueryOut.sqlResult[2].len = 40;
    myQueryOut.sqlResult[2].value = tmpValue = malloc (40*2);
    sprintf (tmpValue, "value 2,1");
    tmpValue += 40;
    sprintf (tmpValue, "value 2,2");

    myQueryOut.sqlResult[3].attriInx = 40;
    myQueryOut.sqlResult[3].len = 50;
    myQueryOut.sqlResult[3].value = tmpValue = malloc (50*2);
    memset (tmpValue, 0, 50*2);

    status = packStruct (&myQueryOut, &packedResult, "GenQueryOut_PI",
      NULL, 0, irodsProt);
    printf ("packStruct status = %d\n", status);
    if (irodsProt == XML_PROT)
        writePackedRes (packedResult, "genq.out"); 

    status = unpackStruct (packedResult->buf, (void **) &outQueryOut,
      "GenQueryOut_PI", NULL, irodsProt);

    printf ("unpackStruct status = %d\n", status);

    status = parsePackInstruct (TEST_PI, &packItemHead);

    printf ("parsePackInstruct status = %d\n", status);

    myStartupPack.connectCnt = 11;
    myStartupPack.irodsProt = XML_PROT;
    strcpy (myStartupPack.proxyUser, "proxyUser"); 
    strcpy (myStartupPack.proxyRodsZone, "proxyRodsZone"); 
    strcpy (myStartupPack.clientUser, "myclientUser"); 
    strcpy (myStartupPack.clientRodsZone, "clientRodsZone"); 
    strcpy (myStartupPack.relVersion, "relVersion"); 
    strcpy (myStartupPack.apiVersion, "apiVersion"); 
    strcpy (myStartupPack.option, "option"); 
    status = packStruct (&myStartupPack, &packedResult, "StartupPack_PI",
      NULL, 0, XML_PROT);
    printf ("packStruct of StartupPack_PI status = %d\n", status);

    if (irodsProt == XML_PROT)
        writePackedRes (packedResult, "startup.out"); 
    status = unpackStruct (packedResult->buf, (void **) &outStartupPack, 
      "StartupPack_PI", NULL, irodsProt);
    printf ("unpackStruct of StartupPack_PI status = %d\n", status);

    myError.len = 3;

    myError.errMsg = (rErrMsg_t **) malloc (sizeof (void *) * 3);
    for (i = 0; i < 3; i++) {
	myError.errMsg[i] = (rErrMsg_t *) malloc (sizeof (rErrMsg_t));  
	sprintf (myError.errMsg[i]->msg, "error %d\n", i);
	myError.errMsg[i]->status = i;
    }

    status = packStruct (&myError, &packedResult, "RError_PI",
      NULL, 0, irodsProt);
    printf ("packStruct of RErrMsg_PI status = %d\n", status);
    if (irodsProt == XML_PROT)
        writePackedRes (packedResult, "error.out");

    status = unpackStruct (packedResult->buf, (void **) &outError, "RError_PI",
      NULL, irodsProt);
    printf ("unpackStruct of RErrMsg_PI status = %d\n", status);
    
} 

int
writePackedRes (bytesBuf_t *packedResult, char *outFile)
{
    FILE *fptr;
    int len;
    int gotRule = 0;

    fptr = fopen (outFile, "w");

    if (fptr == NULL) {
        rodsLog (LOG_ERROR,
          "Cannot open input file %s. ernro = %d\n",
          outFile, errno);
        return (-1);
    }

    len = fwrite (packedResult->buf, packedResult->len, 1, fptr);
    len = fwrite (packedResult->buf, packedResult->len, 1, stdout);
    printf ("\n\n");
    fclose (fptr);

    return (len);
}


