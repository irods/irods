#include "physPath.h"
#include "dataObjFsync.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "fileFsync.h"
#include "fileFstat.h"
#include "modDataObjMeta.h"

int
rsDataObjFsync (rsComm_t *rsComm, openedDataObjInp_t *dataObjFsyncInp)
{
    int rv = 0;
    int l1descInx = dataObjFsyncInp->l1descInx;

    if ((l1descInx < 2) || (l1descInx >= NUM_L1_DESC))
    {
        rodsLog (LOG_NOTICE,
                 "rsDataObjFsync: l1descInx %d out of range",
                 l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (L1desc[l1descInx].inuseFlag != FD_INUSE)
        return BAD_INPUT_DESC_INDEX;
    if (L1desc[l1descInx].remoteZoneHost != NULL)
    {
        /* cross zone operation */
        dataObjFsyncInp->l1descInx = L1desc[l1descInx].remoteL1descInx;
        rv = rcDataObjFsync (L1desc[l1descInx].remoteZoneHost->conn,
                             dataObjFsyncInp);
        dataObjFsyncInp->l1descInx = l1descInx;
    } else {
        rv = _rsDataObjFsync (rsComm, dataObjFsyncInp);

#if 0
	if (rv >= 0)
	{
            ruleExecInfo_t rei;
	    initReiWithDataObjInp (&rei, rsComm,
				   L1desc[l1descInx].dataObjInp);
	    rei.doi = L1desc[l1descInx].dataObjInfo;
	    rei.status = rv;
	    rei.status = applyRule ("acPostProcForUpdate", NULL, &rei,
				    NO_SAVE_REI);
	    /* doi might have changed */
/*	    L1desc[l1descInx].dataObjInfo = rei.doi; */
	}
#endif
    }

    return (rv);
}


int
_rsDataObjFsync (rsComm_t *rsComm, openedDataObjInp_t *dataObjFsyncInp)
{
    int status = 0;
    int l1descInx, l3descInx;
    dataObjInfo_t *dataObjInfo;
    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    char tmpStr[MAX_NAME_LEN];
    rodsLong_t vsize;

    l1descInx = dataObjFsyncInp->l1descInx;
    l3descInx = L1desc[l1descInx].l3descInx;

    /* first sync the buffers to media */
    if (l3descInx > 3)
    {
        status = l3Fsync (rsComm, l1descInx);

        if (status < 0)
        {
            rodsLog (LOG_NOTICE,
                     "_rsDataObjFsync: l3Fsync of %d failed, status = %d",
                     l3descInx, status);
            return (status);
        }
    }

    if (L1desc[l1descInx].oprStatus < 0)
    {
        /* an error has occurred */ 
        rodsLog (LOG_NOTICE, "_rsDataObjFsync oprStatus error");
	return L1desc[l1descInx].oprStatus;
    }

    /** @todo _rsDataObjClose has a lot of test for oprType and
     * specColl in dataObjInfo.  It doesn't seem like that should be
     * an issue for this operation, but I'm not 100% sure.  It's also
     * not 100% clear that all of these metadata need to be updated.
     * At the very least I'd like data_size to be modified, and it
     * seems like the modification time should therefore be updated.
     * I definitely do NOT recommend updating the checksum with each
     * flush, for performance reasons.  If the checksum is *not*
     * updated, should it be cleared or left alone? */
    dataObjInfo = L1desc[l1descInx].dataObjInfo;
    vsize = getSizeInVault(rsComm, dataObjInfo);
    snprintf (tmpStr, MAX_NAME_LEN, "%lld", vsize);
    addKeyVal (&regParam, DATA_SIZE_KW, tmpStr);

    snprintf (tmpStr, MAX_NAME_LEN, "%d", (int) time (NULL));
    addKeyVal (&regParam, DATA_MODIFY_KW, tmpStr);

    addKeyVal (&regParam, ALL_REPL_STATUS_KW, "");

    modDataObjMetaInp.dataObjInfo = dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    status = rsModDataObjMeta (rsComm, &modDataObjMetaInp);

    return (status);
}


int
l3Fsync (rsComm_t *rsComm, int l1descInx)
{
    int rescTypeInx;
    fileFsyncInp_t fileFsyncInp;
    int rv = 0;

    /** @todo anything to do with special collections? */
    rescTypeInx = L1desc[l1descInx].dataObjInfo->rescInfo->rescTypeInx;

    switch (RescTypeDef[rescTypeInx].rescCat)
    {
        case FILE_CAT:
            memset (&fileFsyncInp, 0, sizeof (fileFsyncInp));
            fileFsyncInp.fileInx = L1desc[l1descInx].l3descInx;
            rv = rsFileFsync (rsComm, &fileFsyncInp);
            break;
        default:
            rodsLog (LOG_NOTICE,
                     "l3Write: rescCat type %d is not recognized",
                     RescTypeDef[rescTypeInx].rescCat);
            rv = SYS_INVALID_RESC_TYPE;
            break;
    }

    return (rv);
}
