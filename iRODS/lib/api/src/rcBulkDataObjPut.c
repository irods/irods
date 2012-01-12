/**
 * @file  rcBulkDataObjPut.c
 *
 */

/* This is script-generated code.  */ 
/* See bulkDataObjPut.h for a description of this API call.*/

#include "bulkDataObjPut.h"

/**
 * \fn rcBulkDataObjPut (rcComm_t *conn, bulkOprInp_t *bulkOprInp,
 *       bytesBuf_t *bulkOprInpBBuf)
 *
 * \brief Put (upload) multiple data objects to iRODS with a single call.
 * Info on each file being uploaded are provided in the arrays in bulkOprInp
 * and the contents of each file are concatenated in the bulkOprInpBBuf
 * input.
 *
 * \user client
 *
 * \category data object operations
 *
 * \since 1.0
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Upload two local files - myfile1 and myfile2 to iRODS in the  
 * /myZone/home/john/mydir collection and myRescource resource.
 * \n
 * \n bulkOprInp_t bulkOprInp;
 * \n int status;
 * \n struct stat statbuf;
 * \n rodsLong_t size1, size2;
 * \n int mode1, mode2;
 * \n bytesBuf_t bulkOprInpBBuf;
 * \n char *bufPtr;
 * \n int fd;
 * \n
 * \n bzero (bulkOprInp, sizeof (bulkOprInp_t));
 * \n rstrcpy (bulkOprInp.objPath, "/myZone/home/john/mydir", MAX_NAME_LEN);
 * \n addKeyVal (&bulkOprInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n initAttriArrayOfBulkOprInp (bulkOprInp);
 * \n
 * \n status = stat ("myfile1", &statbuf);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n mode1 = statbuf.st_mode;
 * \n size1 = statbuf.st_size;
 * \n status = stat ("myfile2", &statbuf);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n mode2 = statbuf.st_mode;
 * \n size2 = statbuf.st_size;
 * \n # allocate the buffer for read
 * \n bzero (&bulkOprInpBBuf, sizeof (bulkOprInpBBuf));
 * \n bufPtr = bulkOprInpBBuf.buf = malloc ((int) (size1 + size2));
 * \n # read myfile1
 * \n fd = open ("myfile1", O_RDONLY, 0);
 * \n if (fd < 0) {
 * \n .... handle the error
 * \n }

 * \n status = read (fd, bufPtr, (uint) size1);
 * \n if (status != size1) {
 * \n .... handle the error
 * \n }
 * \n close (fd);
 * \n # add myfile1 to the input array
 * \n status = fillAttriArrayOfBulkOprInp ("/myZone/home/john/mydir/myfile1", 
 * \n      mode1, NULL, (int) size1, &bulkOprInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n # read in myfile2
 * \n bufPtr += size1;
 * \n fd = open ("myfile2", O_RDONLY, 0);
 * \n if (fd < 0) {
 * \n .... handle the error
 * \n }
 * \n status = read (fd, bufPtr, (uint) size2);
 * \n if (status != size2) {
 * \n .... handle the error
 * \n }
 * \n close (fd);
 * \n # add myfile2 to the input array
 * \n status = fillAttriArrayOfBulkOprInp ("/myZone/home/john/mydir/myfile2", 
 * \n      mode2, NULL, (int) size2, &bulkOprInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n # upload the 2 files
 * \n status = rcBulkDataObjPut (conn, &dataObjInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] bulkOprInp - Elements of bulkOprInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the top target 
 *         collection.
 *    \li attriArray - a genQueryOut_t containing arrays on info on the files
 *         being uploaded. The initAttriArrayOfBulkOprInp() call initializes
 *         this struct for bulk operations. Arrays needed for bulk operations
 *         are:
 *    \n COL_DATA_NAME - the full paths of each data object. 
 *    \n COL_DATA_MODE - the createMode of each data object.
 *    \n OFFSET_INX - The accummulative size of the data objects. e.g., 
 *          suppose we are bulk loading 2 files with sizes s1 and s2 
 *          repectively,. The 2 elements of this array are, s1 and s1+s2.
 *    \n COL_D_DATA_CHECKSUM - The checksum values of each data object.
 *           This array is only needed if REG_CHKSUM_KW or VERIFY_CHKSUM_KW
 *           is specified in the condInput.
 *    \n The fillAttriArrayOfBulkOprInp() call can be used to fill these
 *           arrays for one data object.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n DEST_RESC_NAME_KW - The resource to store this data object
 *    \n FORCE_FLAG_KW - overwrite existing copy. This keyWd has no value
 *    \n REG_CHKSUM_KW -  register the target checksum value after the copy.
 *            The value is the md5 checksum value of the local file.
 *    \n VERIFY_CHKSUM_KW - verify and register the target checksum value
 *            after the copy. The value is the md5 checksum value of the
 ^            local file.
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcBulkDataObjPut (rcComm_t *conn, bulkOprInp_t *bulkOprInp,
bytesBuf_t *bulkOprInpBBuf)
{
    int status;
    status = procApiRequest (conn, BULK_DATA_OBJ_PUT_AN, bulkOprInp, 
      bulkOprInpBBuf, (void **) NULL, NULL);

    return (status);
}
