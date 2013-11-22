/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See ooiGenServReq.h for a description of this API call.*/
#include "ooiGenServReq.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "specColl.h"
#include "resource.h"
#include "miscServerFunct.h"

int
rsOoiGenServReq (rsComm_t *rsComm, ooiGenServReqInp_t *ooiGenServReqInp,
ooiGenServReqOut_t **ooiGenServReqOut)
{
    int status;
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    rodsHostAddr_t rescAddr;
    rescGrpInfo_t *rescGrpInfo = NULL;

    rescGrpInfo = new rescGrpInfo_t;
    rescGrpInfo->rescInfo = new rescInfo_t;
    eirods::error err = eirods::get_resc_grp_info ( ooiGenServReqInp->irodsRescName, &rescGrpInfo);
    if ( !err.ok() ) {
         rodsLogError ( LOG_ERROR, status,
                        "rsOoiGenServReq: _getRescInfo of %s error", 
                        ooiGenServReqInp->irodsRescName);
        delete rescGrpInfo->rescInfo;
        delete rescGrpInfo;
        return status;
    }

    bzero (&rescAddr, sizeof (rescAddr));
    rstrcpy (rescAddr.hostAddr, rescGrpInfo->rescInfo->rescLoc, NAME_LEN);
    remoteFlag = resolveHost (&rescAddr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsOoiGenServReq (rsComm, ooiGenServReqInp, ooiGenServReqOut,
          rescGrpInfo);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteOoiGenServReq (rsComm, ooiGenServReqInp, 
          ooiGenServReqOut, rodsServerHost);
    } else if (remoteFlag < 0) {
            status = remoteFlag;
    }

    delete rescGrpInfo->rescInfo;
    delete rescGrpInfo;
    return status;
}

int
remoteOoiGenServReq (rsComm_t *rsComm, ooiGenServReqInp_t *ooiGenServReqInp,
ooiGenServReqOut_t **ooiGenServReqOut, rodsServerHost_t *rodsServerHost)
{
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteOoiGenServReq: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcOoiGenServReq (rodsServerHost->conn, ooiGenServReqInp, 
      ooiGenServReqOut);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
         "remoteOoiGenServReq: rcOoiGenServReq failed for %s, status = %d");
   }

    return status;
}

int
_rsOoiGenServReq (rsComm_t *rsComm, ooiGenServReqInp_t *ooiGenServReqInp,
ooiGenServReqOut_t **ooiGenServReqOut, rescGrpInfo_t *rescGrpInfo)
{
    CURL *easyhandle;
    CURLcode res;
    char myUrl[MAX_NAME_LEN];
    int status;
    char *postStr = NULL;
    ooiGenServReqStruct_t ooiGenServReqStruct;
    char *vaultPath;
    int rescTypeInx;

    if (ooiGenServReqInp == NULL || ooiGenServReqOut == NULL ||
      rescGrpInfo == NULL) return USER__NULL_INPUT_ERR;

    rescTypeInx = rescGrpInfo->rescInfo->rescTypeInx;
    if (RescTypeDef[rescTypeInx].driverType != OOICI_FILE_TYPE) {
        status = SYS_INVALID_RESC_TYPE;
        rodsLogError (LOG_ERROR, status,
          "_rsOoiGenServReq: rescType %s is not ooici type", 
          rescGrpInfo->rescInfo->rescType);
        return status;
    }
    easyhandle = curl_easy_init();
    if(!easyhandle) {
        rodsLog (LOG_ERROR, 
          "_rsOoiGenServReq: curl_easy_init error");
        return OOI_CURL_EASY_INIT_ERR;
    }
   vaultPath = rescGrpInfo->rescInfo->rescVaultPath;
    /* the vault path must be a url e.g., http://localhost:5000 */

    snprintf (myUrl, MAX_NAME_LEN, "%s/%s/%s/%s",
      vaultPath, ION_SERVICE_STR,
        ooiGenServReqInp->servName, ooiGenServReqInp->servOpr);

    if (ooiGenServReqInp->params.len > 0) {
        /* do POST */
        status = jsonPackOoiServReqForPost (ooiGenServReqInp->servName,
                                        ooiGenServReqInp->servOpr,
                                        &ooiGenServReqInp->params, &postStr);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "_rsOoiGenServReq: jsonPackOoiServReq error");
            return status;
        }
        curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, postStr);
    }
    curl_easy_setopt(easyhandle, CURLOPT_URL, myUrl);
    curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, ooiGenServReqFunc);
    bzero (&ooiGenServReqStruct, sizeof (ooiGenServReqStruct));
    ooiGenServReqStruct.outType = ooiGenServReqInp->outType;
    ooiGenServReqStruct.flags = ooiGenServReqInp->flags;

    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &ooiGenServReqStruct);

    res = curl_easy_perform (easyhandle);
    free (postStr);

    if (res != CURLE_OK) {
	/* res is +ive for error */
        rodsLog (LOG_ERROR, 
          "_rsOoiGenServReq: curl_easy_perform error: %d", res);
	free (*ooiGenServReqOut);
        *ooiGenServReqOut = NULL;
        return OOI_CURL_EASY_PERFORM_ERR - res;
    }
    *ooiGenServReqOut = ooiGenServReqStruct.ooiGenServReqOut;
    curl_easy_cleanup (easyhandle);

    return 0;
}

size_t
ooiGenServReqFunc (void *buffer, size_t size, size_t nmemb, void *userp)
{
    char *type_PI;
    int status;
    void *ptr = NULL;
    json_t *root, *dataObj, *responseObj;
    json_error_t jerror;

    ooiGenServReqStruct_t *ooiGenServReqStruct = 
      (ooiGenServReqStruct_t *) userp;

    root = json_loads((const char*) buffer, 0, &jerror);
    if (!root) {
        rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespStr: json_loads error. %s", jerror.text);
        return OOI_JSON_LOAD_ERR;
    }
    dataObj = json_object_get (root, OOI_DATA_TAG);
    if (!dataObj) {
       rodsLog (LOG_ERROR,
          "jsonUnpackOoiRespStr: json_object_get data failed.");
        json_decref (root);
        return OOI_JSON_GET_ERR;
    }
    responseObj = json_object_get(dataObj, OOI_GATEWAY_RESPONSE_TAG);
    if (!responseObj) {
        responseObj = json_object_get(dataObj, OOI_GATEWAY_ERROR_TAG);
        if (!responseObj) {
            json_decref (root);
            rodsLog (LOG_ERROR,
              "jsonUnpackOoiRespStr: json_object_get GatewayResponse failed.");
            return OOI_JSON_GET_ERR;
	} else {
            rodsLog (LOG_ERROR,
              "jsonUnpackOoiRespStr: Gateway returns %s", (char *) buffer);
            ooiGenServReqStruct->ooiGenServReqOut =
              (ooiGenServReqOut_t *) calloc (1, sizeof (ooiGenServReqOut_t));

            rstrcpy (ooiGenServReqStruct->ooiGenServReqOut->type_PI, 
              STR_MS_T, NAME_LEN);
            ooiGenServReqStruct->ooiGenServReqOut->ptr = 
              strdup ((char *)buffer);
            return OOI_JSON_GET_ERR;
        }
    }

    switch (ooiGenServReqStruct->outType) {
      case OOI_STR_TYPE:
	type_PI = STR_MS_T;
        status = jsonUnpackOoiRespStr (responseObj, (char **) &ptr);
        break;
      case OOI_DICT_TYPE:
	type_PI = Dictionary_MS_T;
        status = jsonUnpackOoiRespDict (responseObj, (dictionary_t **) &ptr);
        break;
#if 0	/* use OOI_ARRAY_TYPE */
      case OOI_DICT_ARRAY_TYPE:
	type_PI = DictArray_MS_T;
        status = jsonUnpackOoiRespDictArray (responseObj, 
             (dictArray_t **) &ptr);
        break;
      case OOI_DICT_ARRAY_IN_ARRAY:
	type_PI = DictArray_MS_T;
       status = jsonUnpackOoiRespDictArrInArr (responseObj, 
           (dictArray_t **) &ptr, ooiGenServReqStruct->outInx);
        break;
#endif
      case OOI_ARRAY_TYPE:
        type_PI = GenArray_MS_T;
        status = jsonUnpackOoiRespArray (responseObj, (genArray_t **) &ptr);
        break;
      case OOI_INT_TYPE:
        type_PI = INT_MS_T;
        status = jsonUnpackOoiRespInt (responseObj, (int **) &ptr);
        break;
      case OOI_FLOAT_TYPE:
        type_PI = FLOAT_MS_T;
        status = jsonUnpackOoiRespFloat (responseObj, (float **) &ptr);
        break;
      case OOI_BOOL_TYPE:
        type_PI = BOOL_MS_T;
        status = jsonUnpackOoiRespBool (responseObj, (int **) &ptr);
        break;
      default:
        rodsLog (LOG_ERROR,
          "ooiGenServReqFunc: outType %d not supported", 
          ooiGenServReqStruct->outType);
        status = OOI_JSON_TYPE_ERR;
    }
    json_decref (root);
    if (status < 0) return 0;

    ooiGenServReqStruct->ooiGenServReqOut =
      (ooiGenServReqOut_t *) calloc (1, sizeof (ooiGenServReqOut_t));

    rstrcpy (ooiGenServReqStruct->ooiGenServReqOut->type_PI, type_PI, NAME_LEN);
    ooiGenServReqStruct->ooiGenServReqOut->ptr = ptr;

    return nmemb*size;
}

