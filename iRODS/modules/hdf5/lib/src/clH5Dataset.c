/* clH5Dataset.c
 */
#include "h5Dataset.h"
#include "h5Object.h"
#include "h5String.h"
#include <assert.h>

/*------------------------------------------------------------------------------
 * Purpose: Client siderRead data from file
 * Parameters: H5Dataset* d -- The dataset whose value to be retrieved from file
 *     to server: { opID, fid, fullpath, type, space }                      *
 *     to client: { value, error }                                          *
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int clH5Dataset_read(rcComm_t *conn, H5Dataset* d)
{
    int ret_value = 0;
    H5Dataset *outd = NULL;

    assert(d);

    d->opID = H5DATASET_OP_READ;

    ret_value = _clH5Dataset_read (conn, d, &outd);

    if (ret_value < 0) 
    return (ret_value);

    /* psss on the value */
    d->nvalue = outd->nvalue;
    d->value = outd->value;
    d->error = outd->error;
    d->time = outd->time;
    outd->value = NULL;
    outd->nvalue = 0;

    H5Dataset_dtor (outd);
    free (outd);

    return ret_value;
}

int _clH5Dataset_read (rcComm_t *conn, H5Dataset* ind, H5Dataset** outd)
{
    execMyRuleInp_t execMyRuleInp;
    msParamArray_t *outParamArray = NULL;
    msParamArray_t msParamArray;
    msParam_t *outMsParam;
    int status = 0;


    memset (&execMyRuleInp, 0, sizeof (execMyRuleInp));
    memset (&msParamArray, 0, sizeof (msParamArray));
    execMyRuleInp.inpParamArray = &msParamArray;

    /* specify the msi to run */
    rstrcpy (execMyRuleInp.myRule,
     "H5File_open||msiH5Dataset_read(*IND,*OUTD)|nop", META_STR_LEN);
    /* specify *OUTF as returned value */
    rstrcpy (execMyRuleInp.outParamDesc, "*OUTD", LONG_NAME_LEN);

    addMsParamToArray (execMyRuleInp.inpParamArray, "*IND", h5Dataset_MS_T, ind,
      NULL, 0);

    status = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "_clH5Dataset_read: rcExecMyRule error for %s.",
          ind->fullpath);
        clearMsParamArray (execMyRuleInp.inpParamArray, 0);
        return (status);
    }

    if ((outMsParam = getMsParamByLabel (outParamArray, "*OUTD")) == NULL) {
        status = USER_PARAM_LABEL_ERR;
        rodsLogError (LOG_ERROR, status,
          "_clH5Dataset_read: outParamArray does not contain OUTD for %s.",
          ind->fullpath);
    } else {
        *outd = (H5Dataset *)outMsParam->inOutStruct;
        clearMsParamArray (outParamArray, 0);
        /* XXXXXX free outParamArray */
        free (outParamArray);
    }

    clearMsParamArray (execMyRuleInp.inpParamArray, 0);

    return (status);

}

int clH5Dataset_read_attribute(rcComm_t *conn, H5Dataset* ind)
{
    int ret_value = 0;
    H5Dataset *outd = NULL;

    assert(ind);

    ind->opID = H5DATASET_OP_READ_ATTRIBUTE;

    ret_value = _clH5Dataset_read_attribute (conn, ind, &outd);

    if (ret_value < 0)
        return (ret_value);

    /* psss on the value */
    ind->tclass = outd->tclass;
    ind->error = outd->error;
    ind->nattributes = outd->nattributes;
    ind->attributes = outd->attributes;

    outd->attributes = NULL;
    outd->nattributes = 0;

    H5Dataset_dtor (outd);
    free (outd);

    return ret_value;
}

int _clH5Dataset_read_attribute (rcComm_t *conn, H5Dataset* ind, 
H5Dataset** outd)
{
    execMyRuleInp_t execMyRuleInp;
    msParamArray_t *outParamArray = NULL;
    msParamArray_t msParamArray;
    msParam_t *outMsParam;
    int status = 0;


    memset (&execMyRuleInp, 0, sizeof (execMyRuleInp));
    memset (&msParamArray, 0, sizeof (msParamArray));
    execMyRuleInp.inpParamArray = &msParamArray;

    /* specify the msi to run */
    rstrcpy (execMyRuleInp.myRule,
     "H5File_open||msiH5Dataset_read_attribute(*IND,*OUTD)|nop", META_STR_LEN);
    /* specify *OUTF as returned value */
    rstrcpy (execMyRuleInp.outParamDesc, "*OUTD", LONG_NAME_LEN);

    addMsParamToArray (execMyRuleInp.inpParamArray, "*IND", h5Dataset_MS_T, ind,
      NULL, 0);

    status = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "_clH5Dataset_read_attribute: rcExecMyRule error for %s.",
          ind->fullpath);
        clearMsParamArray (execMyRuleInp.inpParamArray, 0);
        return (status);
    }

    if ((outMsParam = getMsParamByLabel (outParamArray, "*OUTD")) == NULL) {
        status = USER_PARAM_LABEL_ERR;
        rodsLogError (LOG_ERROR, status,
          "_clH5Dataset_read_attribute: outParamArray does not contain OUTD for %s.",
          ind->fullpath);
    } else {
        *outd = (H5Dataset *)outMsParam->inOutStruct;
        clearMsParamArray (outParamArray, 0);
        /* XXXXXX free outParamArray */
        free (outParamArray);
    }

    clearMsParamArray (execMyRuleInp.inpParamArray, 0);

    return (status);
}

