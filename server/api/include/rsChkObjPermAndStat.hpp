#ifndef RS_CHK_OBJ_PERM_AND_STAT_HPP
#define RS_CHK_OBJ_PERM_AND_STAT_HPP

#include "chkObjPermAndStat.h"

int rsChkObjPermAndStat( rsComm_t *rsComm, chkObjPermAndStat_t *chkObjPermAndStatInp );
int _rsChkObjPermAndStat( rsComm_t *rsComm, chkObjPermAndStat_t *chkObjPermAndStatInp );
int chkCollForBundleOpr( rsComm_t *rsComm, chkObjPermAndStat_t *chkObjPermAndStatInp );

#endif
