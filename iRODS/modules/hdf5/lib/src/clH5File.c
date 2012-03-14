/* clH5File.c
 */
#include "h5File.h"
#include <assert.h>

/*------------------------------------------------------------------------------
 * Purpose: Client open an existing file and retrieve the file structure 
 * (root group)
 * Parameters: H5File* f -- The file to open
 *     to server: { opID, filename }                                        *
 *     from server : { fid, root, error }                                      *
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int clH5File_open(rcComm_t *conn, H5File* f)
{
    int ret_value = 0;
    H5File *outf = NULL;


    assert(f);

    ret_value = _clH5File_open(conn, f,  &outf, 0);
	
	if( outf == NULL )
		return 0;


    if (ret_value < 0) 
       return (ret_value);

    f->fid = outf->fid;
    f->root = outf->root;
    outf->root = NULL;
    f->error = outf->error;

    H5File_dtor(outf);
    if (outf)
        free(outf);

    return ret_value;
}

int _clH5File_open(rcComm_t *conn, H5File* inf,  H5File** outf, int flag)
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
     "H5File_open||msiH5File_open(*INF,*FLAG,*OUTF)|nop", META_STR_LEN);
    /* specify *OUTF as returned value */
    rstrcpy (execMyRuleInp.outParamDesc, "*OUTF", LONG_NAME_LEN);

    addMsParamToArray (execMyRuleInp.inpParamArray, "*INF", h5File_MS_T, inf, 
      NULL, 0);
    addMsParamToArray (execMyRuleInp.inpParamArray, "*FLAG", INT_MS_T, &flag, 
      NULL, 0);

    status = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status, 
            "_clH5File_open: rcExecMyRule error for %s.",
            inf->filename);
            clearMsParamArray (execMyRuleInp.inpParamArray, 0);
        return (status);
    }

    if ((outMsParam = getMsParamByLabel (outParamArray, "*OUTF")) == NULL) {
        status = USER_PARAM_LABEL_ERR;
            rodsLogError (LOG_ERROR, status,
            "_clH5File_open: outParamArray does not contain OUTF for %s.",
             inf->filename);
    } else {
        *outf = (H5File*) outMsParam->inOutStruct;
        clearMsParamArray (outParamArray, 0);
        /* XXXX free outParamArray */
    //    free (outParamArray);
    }

    clearMsParamArray (execMyRuleInp.inpParamArray, 0);

    return (status);
}


/*------------------------------------------------------------------------------
 * Purpose: Client Close an opened H5file.
 * Parameters: H5File* f -- The opend H5file
 *     to server: { opID, fid }                                             *
 *     to client: { error }                                                 *
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int clH5File_close (rcComm_t *conn, H5File* f)
{
    int ret_value = 0;
    H5File inf, *outf = NULL;


    assert(f);

    memset (&inf, 0, sizeof (inf));

    inf.fid = f->fid;
    inf.opID = f->opID;
 
    ret_value = _clH5File_close (conn, &inf, &outf);
	if( NULL == outf ) { // JMC cppcheck nullptr ref
		rodsLog( LOG_ERROR, "clH5File_close :: outf is null" );
		return 0;
	}

    if (ret_value < 0) 
    return (ret_value);

    f->error = outf->error;

    H5File_dtor(outf);
    if (outf)
        free(outf);

    return ret_value;
}

int _clH5File_close (rcComm_t *conn, H5File* inf, H5File** outf)
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
     "H5File_close||msiH5File_close(*INF,*OUTF)|nop", META_STR_LEN);
    /* specify *OUTF as returned value */
    rstrcpy (execMyRuleInp.outParamDesc, "*OUTF", LONG_NAME_LEN);

    addMsParamToArray (execMyRuleInp.inpParamArray, "*INF", h5File_MS_T, inf,
      NULL, 0);

    status = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "_clH5File_close: rcExecMyRule error for %s.",
          inf->filename);
        clearMsParamArray (execMyRuleInp.inpParamArray, 0);
        return (status);
    }

    if ((outMsParam = getMsParamByLabel (outParamArray, "*OUTF")) == NULL) {
        status = USER_PARAM_LABEL_ERR;
        rodsLogError (LOG_ERROR, status,
          "_clH5File_close: outParamArray does not contain OUTF");
    } else {
        *outf = (H5File*)outMsParam->inOutStruct;
        clearMsParamArray (outParamArray, 0);
        free (outParamArray);
    }

    clearMsParamArray (execMyRuleInp.inpParamArray, 0);

    return (status);
}

