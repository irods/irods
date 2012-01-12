/**
 * @file hdf5MS.c
 *
 */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hdf5MS.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

/**
 * \fn msiH5File_open (msParam_t *inpH5FileParam, msParam_t *inpFlagParam,
 * msParam_t *outH5FileParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice opens an HDF5 file.
 *
 * \module hdf5
 *
 * \since pre-2.1
 *
 * \author uiuc.edu Mike Wan
 * \date  Feb 2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpH5FileParam - The input H5File to open. Must be h5File_MS_T.
 * \param[in] inpFlagParam - Input flag - INT_MS_T.
 * \param[out] outH5FileParam - the output H5File - Must be h5File_MS_T.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa none
**/
int
msiH5File_open (msParam_t *inpH5FileParam, msParam_t *inpFlagParam,
msParam_t *outH5FileParam, ruleExecInfo_t *rei)
{
    rsComm_t *rsComm;
    H5File *inf = 0;
    H5File *outf;
    int inpFlag;
    int l1descInx;
    dataObjInp_t dataObjInp;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *dataObjInfo, *tmpDataObjInfo;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    int fid;

    RE_TEST_MACRO ("    Calling msiH5File_open")

    if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR,
          "msiH5File_open: input rei or rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    rsComm = rei->rsComm;

    if (inpH5FileParam == NULL || inpFlagParam == NULL ||
      outH5FileParam == NULL) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_open: NULL input/output Param");
        return (rei->status);
    }

    if (strcmp (inpH5FileParam->type, h5File_MS_T) == 0) {
        inf = (H5File*)inpH5FileParam->inOutStruct;
    } else {
        rei->status = USER_PARAM_TYPE_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_open: inpH5FileParam must be h5File_MS_T");
        return (rei->status);
    }     

    inpFlag = parseMspForPosInt (inpFlagParam);
    if (inpFlag < 0) {
	inpFlag = 0;
    }

    /* see if we need to do it remotely based on the objPath. Open the
     * obj but don't do it physically */

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, inf->filename, MAX_NAME_LEN);
    dataObjInp.openFlags = O_RDONLY;
    addKeyVal (&dataObjInp.condInput, NO_OPEN_FLAG_KW, "");
    rei->status = l1descInx = 
      _rsDataObjOpen (rsComm, &dataObjInp);

    if (rei->status < 0) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_open: _rsDataObjOpen of %s error",
	  dataObjInp.objPath);
        return (rei->status);
    }

    dataObjCloseInp.l1descInx = l1descInx;
    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if ((inpFlag & LOCAL_H5_OPEN) != 0) {
	/* need to find a local copy */
	tmpDataObjInfo = dataObjInfo;
	while (tmpDataObjInfo != NULL) {
	    remoteFlag = resolveHostByDataObjInfo (tmpDataObjInfo,
	      &rodsServerHost);
	    if (remoteFlag == LOCAL_HOST) {
		requeDataObjInfoByReplNum (&dataObjInfo, 
		  tmpDataObjInfo->replNum);
		/* have to update L1desc */
		L1desc[l1descInx].dataObjInfo = dataObjInfo;
		break;
	    }
	}
	if (remoteFlag != LOCAL_HOST) {
	    /* there is no local copy */
	    rei->status = SYS_COPY_NOT_EXIST_IN_RESC;
            rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
              "msiH5File_open: _local copy of %s does not exist",
	      dataObjInp.objPath);
            rsDataObjClose (rsComm, &dataObjCloseInp);
            return (rei->status);
	}
    } else {
	remoteFlag = resolveHostByDataObjInfo (dataObjInfo,
          &rodsServerHost);
    }

    if (remoteFlag == LOCAL_HOST) {
	outf = (H5File*)malloc (sizeof (H5File));
        /* replace iRODS file with local file */
        if (inf->filename != NULL) {
            free (inf->filename);
        }
        inf->filename = strdup (dataObjInfo->filePath);
        fid = H5File_open(inf, outf);
    } else {
	/* do the remote open */
        if ((rei->status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
            rsDataObjClose (rsComm, &dataObjCloseInp);
            return rei->status;
        }

	outf = NULL;
	fid = _clH5File_open (rodsServerHost->conn, inf, &outf, LOCAL_H5_OPEN);
    }

    if (fid >= 0) {
	L1desc[l1descInx].l3descInx = outf->fid;
    } else {
	rei->status = fid;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_open: H5File_open %s error",
          dataObjInp.objPath);
        rsDataObjClose (rsComm, &dataObjCloseInp);
	return rei->status;
    }

    /* prepare the output */

#if 0
     outf->fid = l1descInx;
#endif
    fillMsParam (outH5FileParam, NULL, h5File_MS_T, outf, NULL);
    
    if (inf) H5File_dtor(inf);

    return (rei->status);
}

/**
 * \fn msiH5File_close (msParam_t *inpH5FileParam, msParam_t *outH5FileParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice closes an HDF5 file.
 *
 * \module hdf5
 *
 * \since pre-2.1
 *
 * \author uiuc.edu Mike Wan
 * \date  2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpH5FileParam - The input H5File to close. Must be h5File_MS_T.
 * \param[out] outH5FileParam - the output H5File - Must be h5File_MS_T.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa msiH5File_open
**/
int
msiH5File_close (msParam_t *inpH5FileParam, msParam_t *outH5FileParam,
ruleExecInfo_t *rei)
{
    rsComm_t *rsComm;
    H5File *inf = 0;
    H5File *outf;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *dataObjInfo;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    int l1descInx;

    RE_TEST_MACRO ("    Calling msiH5File_close")

    if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR,
          "msiH5File_close: input rei or rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    rsComm = rei->rsComm;

    if (inpH5FileParam == NULL || outH5FileParam == NULL) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_close: NULL input/output Param");
        return (rei->status);
    }

    if (strcmp (inpH5FileParam->type, h5File_MS_T) == 0) {
        inf = (H5File*)inpH5FileParam->inOutStruct;
    } else {
        rei->status = USER_PARAM_TYPE_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_close: wrong inpH5FileParam type %s",
          inpH5FileParam->type);
        return (rei->status);
    }

    l1descInx = getL1descInxByFid (inf->fid);
    if (l1descInx < 0) {
	rei->status = SYS_BAD_FILE_DESCRIPTOR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_close: bad fid %d", inf->fid);
        return (rei->status);
    }
	
    dataObjCloseInp.l1descInx = l1descInx;
    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    remoteFlag = resolveHostByDataObjInfo (dataObjInfo,
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        outf = (H5File*)malloc (sizeof (H5File));
	/* switch the fid */
	inf->fid = L1desc[l1descInx].l3descInx;
        rei->status = H5File_close (inf, outf);
    } else {
        /* do the remote close */
        if ((rei->status = svrToSvrConnect (rsComm, rodsServerHost)) >= 0) {
            rei->status = _clH5File_close (rodsServerHost->conn, inf, &outf);
	}
    }

    if (rei->status < 0) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5File_close: H5File_close error for %d", l1descInx);
    }

    L1desc[l1descInx].l3descInx = 0;
    rsDataObjClose (rsComm, &dataObjCloseInp);

    /* prepare the output */

    fillMsParam (outH5FileParam, NULL, h5File_MS_T, outf, NULL);

    if (inf) H5File_dtor(inf);

    return rei->status;
}

/**
 * \fn msiH5Dataset_read (msParam_t *inpH5DatasetParam, msParam_t *outH5DatasetParam,
 * ruleExecInfo_t *rei)
 *
 * \brief This microservice is for reading a dataset from an opened HDF5 file.
 *
 * \module hdf5
 *
 * \since pre-2.1
 *
 * \author uiuc.edu Mike Wan
 * \date  2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpH5DatasetParam - The input H5Dataset. Must be h5Dataset_MS_T.
 * \param[out] outH5DatasetParam - The output H5Dataset - Must be h5Dataset_MS_T.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa none
**/
int
msiH5Dataset_read (msParam_t *inpH5DatasetParam, msParam_t *outH5DatasetParam,
ruleExecInfo_t *rei)
{
    rsComm_t *rsComm;
    H5Dataset *ind;
    H5Dataset *outd;
    int l1descInx;
    dataObjInfo_t *dataObjInfo;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    RE_TEST_MACRO ("    Calling msiH5Dataset_read")

    if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR,
          "msiH5Dataset_read: input rei or rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    rsComm = rei->rsComm;

    if (inpH5DatasetParam == NULL || outH5DatasetParam == NULL) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Dataset_read: NULL input/output Param");
        return (rei->status);
    }

    if (strcmp (inpH5DatasetParam->type, h5Dataset_MS_T) == 0) {
        ind = (H5Dataset*)inpH5DatasetParam->inOutStruct;
    } else {
        rei->status = USER_PARAM_TYPE_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Dataset_read: wrong inpH5DatasetParam type %s",
	  inpH5DatasetParam->type);
        return (rei->status);
    }

    l1descInx = getL1descInxByFid (ind->fid);
    if (l1descInx < 0) {
        rei->status = SYS_BAD_FILE_DESCRIPTOR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Dataset_read: bad fid %d", ind->fid);
        return (rei->status);
    }

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    remoteFlag = resolveHostByDataObjInfo (dataObjInfo,
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
	outd = (H5Dataset*)malloc (sizeof (H5Dataset));
#if 0
        /* switch the fid */
        ind->fid = L1desc[l1descInx].l3descInx;
#endif
        rei->status = H5Dataset_read (ind, outd);
    } else {
        /* do the remote close */
	outd = NULL;
        if ((rei->status = svrToSvrConnect (rsComm, rodsServerHost)) >= 0) {
            rei->status = _clH5Dataset_read (rodsServerHost->conn, ind, &outd);
        }
    }

    /* prepare the output */

    fillMsParam (outH5DatasetParam, NULL, h5Dataset_MS_T, outd, NULL);

    if (ind) H5Dataset_dtor(ind);

    return rei->status;
}

/**
 * \fn msiH5Dataset_read_attribute (msParam_t *inpH5DatasetParam, msParam_t *outH5DatasetParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice is for reading attribute of a dataset from an opened HDF5 file.
 *
 * \module hdf5
 *
 * \since pre-2.1
 *
 * \author uiuc.edu Mike Wan
 * \date  2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpH5DatasetParam - The input H5Dataset. Must be h5Dataset_MS_T.
 * \param[out] outH5DatasetParam - The output H5Dataset - Must be h5Dataset_MS_T.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa none
**/
int
msiH5Dataset_read_attribute (msParam_t *inpH5DatasetParam, msParam_t *outH5DatasetParam, ruleExecInfo_t *rei)
{
    rsComm_t *rsComm;
    H5Dataset *ind;
    H5Dataset *outd;
    int l1descInx;
    dataObjInfo_t *dataObjInfo;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    RE_TEST_MACRO ("    Calling msiH5Dataset_read_attribute")

    if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR,
          "msiH5Dataset_read_attribute: input rei or rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    rsComm = rei->rsComm;

    if (inpH5DatasetParam == NULL || outH5DatasetParam == NULL) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Dataset_read_attribute: NULL input/output Param");
        return (rei->status);
    }

    if (strcmp (inpH5DatasetParam->type, h5Dataset_MS_T) == 0) {
        ind = (H5Dataset*)inpH5DatasetParam->inOutStruct;
    } else {
        rei->status = USER_PARAM_TYPE_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Dataset_read_attribute: wrong inpH5DatasetParam type %s",
	  inpH5DatasetParam->type);
        return (rei->status);
    }

    l1descInx = getL1descInxByFid (ind->fid);
    if (l1descInx < 0) {
        rei->status = SYS_BAD_FILE_DESCRIPTOR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Dataset_read_attribute: bad fid %d", ind->fid);
        return (rei->status);
    }

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    remoteFlag = resolveHostByDataObjInfo (dataObjInfo,
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
	outd = (H5Dataset*)malloc (sizeof (H5Dataset));
#if 0
        /* switch the fid */
        ind->fid = L1desc[l1descInx].l3descInx;
#endif
        rei->status = H5Dataset_read_attribute (ind, outd);
    } else {
        /* do the remote close */
	outd = NULL;
        if ((rei->status = svrToSvrConnect (rsComm, rodsServerHost)) >= 0) {
            rei->status = _clH5Dataset_read_attribute (rodsServerHost->conn, 
	      ind, &outd);
        }
    }

    /* prepare the output */

    fillMsParam (outH5DatasetParam, NULL, h5Dataset_MS_T, outd, NULL);

    if (ind) H5Dataset_dtor(ind);

    return rei->status;
}

/**
 * \fn msiH5Group_read_attribute (msParam_t *inpH5GroupParam, msParam_t *outH5GroupParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice is for reading attribute of a dataset from an opened H5Group.
 *
 * \module hdf5
 *
 * \since pre-2.1
 *
 * \author uiuc.edu Mike wan
 * \date   2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpH5GroupParam - The input H5Group. Must be h5Dataset_MS_T.
 * \param[out] outH5GroupParam - The output H5Group - Must be h5Dataset_MS_T.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa none
**/
int
msiH5Group_read_attribute (msParam_t *inpH5GroupParam, 
msParam_t *outH5GroupParam, ruleExecInfo_t *rei)
{
    rsComm_t *rsComm;
    H5Group *ing;
    H5Group *outg;
    int l1descInx;
    dataObjInfo_t *dataObjInfo;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    RE_TEST_MACRO ("    Calling msiH5Group_read_attribute")

    if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR,
          "msiH5Group_read_attribute: input rei or rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    rsComm = rei->rsComm;

    if (inpH5GroupParam == NULL || outH5GroupParam == NULL) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Group_read_attribute: NULL input/output Param");
        return (rei->status);
    }

#if 0
    if (strcmp (inpH5GroupParam->type, h5Dataset_MS_T) == 0) {
#endif
    if (strcmp (inpH5GroupParam->type, h5Group_MS_T) == 0) {
        ing = (H5Group*)inpH5GroupParam->inOutStruct;
    } else {
        rei->status = USER_PARAM_TYPE_ERR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Group_read_attribute: wrong inpH5GroupParam type %s",
	  inpH5GroupParam->type);
        return (rei->status);
    }

    l1descInx = getL1descInxByFid (ing->fid);
    if (l1descInx < 0) {
        rei->status = SYS_BAD_FILE_DESCRIPTOR;
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiH5Group_read_attribute: bad fid %d", ing->fid);
        return (rei->status);
    }


    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    remoteFlag = resolveHostByDataObjInfo (dataObjInfo,
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
	outg = (H5Group*)malloc (sizeof (H5Group));
#if 0
        /* switch the fid */
        ing->fid = L1desc[l1descInx].l3descInx;
#endif
        rei->status = H5Group_read_attribute (ing, outg);
    } else {
        /* do the remote close */
	outg = NULL;
        if ((rei->status = svrToSvrConnect (rsComm, rodsServerHost)) >= 0) {
            rei->status = _clH5Group_read_attribute (rodsServerHost->conn, 
	      ing, &outg);
        }
    }

    /* prepare the output */

    fillMsParam (outH5GroupParam, NULL, h5Group_MS_T, outg, NULL);

    if (ing) H5Group_dtor(ing);

    return rei->status;
}

int
getL1descInxByFid (int fid)
{
    int i;

    for (i = 3; i < NUM_L1_DESC; i++) {
	if (L1desc[i].inuseFlag == 1 && L1desc[i].l3descInx == fid) return i;
    }

    return (-1);
}
	      
