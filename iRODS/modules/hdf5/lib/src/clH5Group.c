/* clH5Group.c
 */
#include "h5Attribute.h"
#include "h5Object.h"
#include "h5Group.h"
#include "h5String.h"
#include <assert.h>

int clH5Group_read_attribute(rcComm_t *conn, H5Group* ing)
{
    int ret_value = 0;
    H5Group* outg = NULL;

    assert(ing);

    ing->opID = H5GROUP_OP_READ_ATTRIBUTE;

    ret_value = _clH5Group_read_attribute (conn,  ing, &outg);

    if (ret_value < 0)
        return (ret_value);
    
    /* psss on the value */
    ing->error = outg->error;
    ing->nattributes = outg->nattributes;
    ing->attributes = outg->attributes;

    outg->attributes = NULL;
    outg->nattributes = 0;

    H5Group_dtor (outg);
    free (outg);

    return ret_value;
}

int _clH5Group_read_attribute(rcComm_t *conn, H5Group* ing, H5Group** outg)
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
     "H5File_open||msiH5Group_read_attribute(*ING,*OUTG)|nop", META_STR_LEN);
    /* specify *OUTF as returned value */
    rstrcpy (execMyRuleInp.outParamDesc, "*OUTG", LONG_NAME_LEN);
#if 0
    addMsParamToArray (execMyRuleInp.inpParamArray, "*ING", h5Dataset_MS_T, ing,
#endif
    addMsParamToArray (execMyRuleInp.inpParamArray, "*ING", h5Group_MS_T, ing,
      NULL, 0);

    status = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "_clH5Group_read_attribute: rcExecMyRule error for %s.",
          ing->fullpath);
        clearMsParamArray (execMyRuleInp.inpParamArray, 0);
        return (status);
    }

    if ((outMsParam = getMsParamByLabel (outParamArray, "*OUTG")) == NULL) {
        status = USER_PARAM_LABEL_ERR;
        rodsLogError (LOG_ERROR, status,
          "_clH5Group_read_attribute: outParamArray does not contain OUTG for %s.",
          ing->fullpath);
    } else {
        *outg = (H5Group*)outMsParam->inOutStruct;
        clearMsParamArray (outParamArray, 0);
	    free (outParamArray);
    }

    clearMsParamArray (execMyRuleInp.inpParamArray, 0);

    return (status);
}

