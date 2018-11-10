#include "rcConnect.h"
#include "apiHeaderAll.h"
#include "apiHandler.hpp"
#include "boost/any.hpp"
#include <functional>


int call_miscSvrInfo_out(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    miscSvrInfo_t**    _out ) {
    return _api->call_handler<
               miscSvrInfo_t**>(
                   _comm,
                   _out );
}

int call_fileCreateInp_fileCreateOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileCreateInp_t*  _inp,
    fileCreateOut_t** _out ) {
    return _api->call_handler<
               fileCreateInp_t*,
               fileCreateOut_t**>(
                   _comm,
                   _inp,
                   _out );
}

int call_fileOpenInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileOpenInp_t*    _inp ) {
    return _api->call_handler<
               fileOpenInp_t*>(
                   _comm,
                   _inp);
}

int call_fileWriteInp_bytesBufInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileWriteInp_t*   _inp,
    bytesBuf_t*       _buf ) {
    return _api->call_handler<
               fileWriteInp_t*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _buf);
}

int call_fileCloseInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileCloseInp_t*    _inp ) {
    return _api->call_handler<
               fileCloseInp_t*>(
                   _comm,
                   _inp);
}

int call_fileLseekInp_fileLseekOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileLseekInp_t*   _inp,
    fileLseekOut_t**  _out ) {
    return _api->call_handler<
               fileLseekInp_t*,
               fileLseekOut_t**>(
                   _comm,
                   _inp,
                   _out );
}

int call_fileReadInp_bytesBufOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileReadInp_t*   _inp,
    bytesBuf_t*       _buf ) {
    return _api->call_handler<
               fileReadInp_t*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _buf);
}

int call_fileReadInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileReadInp_t*   _inp ) {
    return _api->call_handler<
               fileReadInp_t*>(
                   _comm,
                   _inp);
}

int call_fileUnlinkInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileUnlinkInp_t*  _inp ) {
    return _api->call_handler<
               fileUnlinkInp_t*>(
                   _comm,
                   _inp);
}

int call_fileMkdirInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileMkdirInp_t*   _inp ) {
    return _api->call_handler<
               fileMkdirInp_t*>(
                   _comm,
                   _inp);
}

int call_fileChmodInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileChmodInp_t*   _inp ) {
    return _api->call_handler<
               fileChmodInp_t*>(
                   _comm,
                   _inp);
}

int call_fileRmdirInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileRmdirInp_t*   _inp ) {
    return _api->call_handler<
               fileRmdirInp_t*>(
                   _comm,
                   _inp);
}

int call_fileStatInp_rodsStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileStatInp_t*    _inp,
    rodsStat_t**      _out ) {
    return _api->call_handler<
               fileStatInp_t*,
               rodsStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_fileGetFreespaceInp_fileGetFreespaceOut(
    irods::api_entry*         _api,
    rsComm_t*                 _comm,
    fileGetFsFreeSpaceInp_t*  _inp,
    fileGetFsFreeSpaceOut_t** _out ) {
    return _api->call_handler<
               fileGetFsFreeSpaceInp_t*,
               fileGetFsFreeSpaceOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_fileOpendirInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileOpendirInp_t* _inp ) {
    return _api->call_handler<
               fileOpendirInp_t*>(
                   _comm,
                   _inp);
}

int call_fileClosedirInp(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    fileClosedirInp_t* _inp ) {
    return _api->call_handler<
               fileClosedirInp_t*>(
                   _comm,
                   _inp);
}

int call_fileReaddirInp_rodsDirentOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileReaddirInp_t* _inp,
    rodsDirent_t**    _out ) {
    return _api->call_handler<
               fileReaddirInp_t*,
               rodsDirent_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_fileRenameInp_fileRenameOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileRenameInp_t*  _inp,
    fileRenameOut_t** _out ) {
    return _api->call_handler<
               fileRenameInp_t*,
               fileRenameOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_fileOpenInp_bytesBufInp_filePutOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileOpenInp_t*    _inp,
    bytesBuf_t*       _buf,
    filePutOut_t**    _out ) {
    return _api->call_handler<
               fileOpenInp_t*,
               bytesBuf_t*,
               filePutOut_t**>(
                   _comm,
                   _inp,
                   _buf,
                   _out);
}

int call_fileOpenInp_bytesBufOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileOpenInp_t*    _inp,
    bytesBuf_t*       _buf ) {
    return _api->call_handler<
               fileOpenInp_t*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _buf);
}

int call_fileStageSyncInp(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    fileStageSyncInp_t* _inp ) {
    return _api->call_handler<
               fileStageSyncInp_t*>(
                   _comm,
                   _inp);
}

int call_fileStageSyncInp_fileSyncOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    fileStageSyncInp_t* _inp,
    fileSyncOut_t**     _out ) {
    return _api->call_handler<
               fileStageSyncInp_t*>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataObjInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*     _inp ) {
    return _api->call_handler<
               dataObjInp_t*>(
                   _comm,
                   _inp);
}

int call_openedDataObjInp_bytesBufOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    openedDataObjInp_t* _inp,
    bytesBuf_t*         _buf ) {
    return _api->call_handler<
               openedDataObjInp_t*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _buf);
}

int call_openedDataObjInp(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    openedDataObjInp_t* _inp ) {
    return _api->call_handler<
               openedDataObjInp_t*>(
                   _comm,
                   _inp );
}

int call_dataObjInp_bytesBufInp_portalOprOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    dataObjInp_t*       _inp,
    bytesBuf_t*         _buf,
    portalOprOut_t**    _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               bytesBuf_t*,
               portalOprOut_t**>(
                   _comm,
                   _inp,
                   _buf,
                   _out);
}

int call_dataObjInp_portalOprOut_bytesBufOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    dataObjInp_t*       _inp,
    portalOprOut_t**    _out,
    bytesBuf_t*         _buf ) {
    return _api->call_handler<
               dataObjInp_t*,
               portalOprOut_t**,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _out,
                   _buf);
}

int call_dataObjInp_transferStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*     _inp,
    transferStat_t**  _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               transferStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_openedDataObjInp_fileLseekOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    openedDataObjInp_t* _inp,
    fileLseekOut_t**    _out ) {
    return _api->call_handler<
               openedDataObjInp_t*,
               fileLseekOut_t**>(
                   _comm,
                   _inp,
                   _out );
}

int call_dataObjCopyInp_transferStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjCopyInp_t* _inp,
    transferStat_t**  _out ) {
    return _api->call_handler<
               dataObjCopyInp_t*,
               transferStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataOprInp_portalOprOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    dataOprInp_t*       _inp,
    portalOprOut_t**    _out ) {
    return _api->call_handler<
               dataOprInp_t*,
               portalOprOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataCopyInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataCopyInp_t*    _inp ) {
    return _api->call_handler<
               dataCopyInp_t*>(
                   _comm,
                   _inp);
}

int call_simpleQueryInp_simpleQueryOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    simpleQueryInp_t*   _inp,
    simpleQueryOut_t**  _out ) {
    return _api->call_handler<
               simpleQueryInp_t*,
               simpleQueryOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_generalAdminInp(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    generalAdminInp_t* _inp ) {
    return _api->call_handler<
               generalAdminInp_t*>(
                   _comm,
                   _inp);
}

int call_genQueryInp_genQueryOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    genQueryInp_t*    _inp,
    genQueryOut_t**   _out ) {
    return _api->call_handler<
               genQueryInp_t*,
               genQueryOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_authRequestOut(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    authRequestOut_t** _out ) {
    return _api->call_handler<
               authRequestOut_t**>(
                   _comm,
                   _out);
}

int call_authResponseInp(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    authResponseInp_t* _inp ) {
    return _api->call_handler<
               authResponseInp_t*>(
                   _comm,
                   _inp);
}

int call_authCheckInp_authCheckOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    authCheckInp_t*   _inp,
    authCheckOut_t**  _out ) {
    return _api->call_handler<
               authCheckInp_t*,
               authCheckOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_endTransactionInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    endTransactionInp_t* _inp ) {
    return _api->call_handler<
               endTransactionInp_t*>(
                   _comm,
                   _inp);
}

int call_specificQueryInp_genQueryOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    specificQueryInp_t* _inp,
    genQueryOut_t**     _out ) {
    return _api->call_handler<
               specificQueryInp_t*,
               genQueryOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_ticketAdminInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    ticketAdminInp_t* _inp ) {
    return _api->call_handler<
               ticketAdminInp_t*>(
                   _comm,
                   _inp);
}

int call_getTempPasswordOut(
    irods::api_entry*      _api,
    rsComm_t*              _comm,
    getTempPasswordOut_t** _out ) {
    return _api->call_handler<
               getTempPasswordOut_t**>(
                   _comm,
                   _out);
}

int call_getTempPasswordForOtherInp_getTempPasswordForOtherOut(
    irods::api_entry*              _api,
    rsComm_t*                      _comm,
    getTempPasswordForOtherInp_t*  _inp,
    getTempPasswordForOtherOut_t** _out ) {
    return _api->call_handler<
               getTempPasswordForOtherInp_t*,
               getTempPasswordForOtherOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_pamAuthRequestInp_pamAuthRequestOut(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    pamAuthRequestInp_t*  _inp,
    pamAuthRequestOut_t** _out ) {
    return _api->call_handler<
               pamAuthRequestInp_t*,
               pamAuthRequestOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_getLimitedPasswordInp_getLimitedPasswordOut(
    irods::api_entry*         _api,
    rsComm_t*                 _comm,
    getLimitedPasswordInp_t*  _inp,
    getLimitedPasswordOut_t** _out ) {
    return _api->call_handler<
               getLimitedPasswordInp_t*,
               getLimitedPasswordOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_collInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    collInp_t*        _inp ) {
    return _api->call_handler<
               collInp_t*>(
                   _comm,
                   _inp);
}

int call_intp_collEntOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    int*              _inp,
    collEnt_t**       _out ) {
    return _api->call_handler<
               int*,
               collEnt_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_userAdminInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    userAdminInp_t*   _inp ) {
    return _api->call_handler<
               userAdminInp_t*>(
                   _comm,
                   _inp);
}

int call_generalRowInserInp(
    irods::api_entry*     _api,
    rsComm_t*             _comm,
    generalRowInsertInp_t* _inp ) {
    return _api->call_handler<
               generalRowInsertInp_t*>(
                   _comm,
                   _inp);
}

int call_generalRowPurgeInp(
    irods::api_entry*     _api,
    rsComm_t*             _comm,
    generalRowPurgeInp_t* _inp ) {
    return _api->call_handler<
               generalRowPurgeInp_t*>(
                   _comm,
                   _inp);
}

int call_intp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    int*              _inp ) {
    return _api->call_handler<
               int*>(
                   _comm,
                   _inp);
}

int call_collInp_collOprStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    collInp_t*        _inp,
    collOprStat_t**   _out ) {
    return _api->call_handler<
               collInp_t*,
               collOprStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_modAVUMetadataInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    modAVUMetadataInp_t* _inp ) {
    return _api->call_handler<
               modAVUMetadataInp_t*>(
                   _comm,
                   _inp);
}

int call_modAccessControlInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    modAccessControlInp_t* _inp ) {
    return _api->call_handler<
               modAccessControlInp_t*>(
                   _comm,
                   _inp);
}

int call_ruleExecModInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    ruleExecModInp_t* _inp ) {
    return _api->call_handler<
               ruleExecModInp_t*>(
                   _comm,
                   _inp);
}

int call_generalUpdateInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    generalUpdateInp_t* _inp ) {
    return _api->call_handler<
               generalUpdateInp_t*>(
                   _comm,
                   _inp);
}

int call_modDataObjMetaInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    modDataObjMeta_t* _inp ) {
    return _api->call_handler<
               modDataObjMeta_t*>(
                   _comm,
                   _inp);
}

int call_ruleExecSubmitInp_charOut(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    ruleExecSubmitInp_t* _inp,
    char**               _out ) {
    return _api->call_handler<
               ruleExecSubmitInp_t*,
               char**>(
                   _comm,
                   _inp,
                   _out);
}

int call_ruleExecDelInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    ruleExecDelInp_t* _inp ) {
    return _api->call_handler<
               ruleExecDelInp_t*>(
                   _comm,
                   _inp);
}

int call_execMyRuleInp_msParamArrayOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    execMyRuleInp_t*        _inp,
    msParamArray_t**   _out ) {
    return _api->call_handler<
               execMyRuleInp_t*,
               msParamArray_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataObjCopyInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjCopyInp_t* _inp) {
    return _api->call_handler<
               dataObjCopyInp_t*>(
                   _comm,
                   _inp);
}

int call_dataObjInp_msParamArrayOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*        _inp,
    msParamArray_t**   _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               msParamArray_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataObjInp_charOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*        _inp,
    char**   _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               char**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataObjInp_rodsObjStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*     _inp,
    rodsObjStat_t**   _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               rodsObjStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_execCmdInp_execCmdOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    execCmd_t*        _inp,
    execCmdOut_t**   _out ) {
    return _api->call_handler<
               execCmd_t*,
               execCmdOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_structFileOprInp(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    structFileOprInp_t* _inp ) {
    return _api->call_handler<
               structFileOprInp_t*>(
                   _comm,
                   _inp);
}
int call_subFileInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    subFile_t*        _inp ) {
    return _api->call_handler<
               subFile_t*>(
                   _comm,
                   _inp);
}

int call_subStructFileFdOprInp_bytesBufOut(
    irods::api_entry*        _api,
    rsComm_t*                _comm,
    subStructFileFdOprInp_t* _inp,
    bytesBuf_t*              _out ) {
    return _api->call_handler<
               subStructFileFdOprInp_t*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _out);
}

int call_subStructFileFdOprInp(
    irods::api_entry*        _api,
    rsComm_t*                _comm,
    subStructFileFdOprInp_t* _inp ) {
    return _api->call_handler<
               subStructFileFdOprInp_t*>(
                   _comm,
                   _inp);
}

int call_subFileInp_rodsStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    subFile_t*        _inp,
    rodsStat_t**      _out ) {
    return _api->call_handler<
               subFile_t*,
               rodsStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_subStructFileLseekInp_fileLseekOut(
    irods::api_entry*        _api,
    rsComm_t*                _comm,
    subStructFileLseekInp_t* _inp,
    fileLseekOut_t**         _out ) {
    return _api->call_handler<
               subStructFileLseekInp_t*,
               fileLseekOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_subStructFileRenameInp(
    irods::api_entry*        _api,
    rsComm_t*                _comm,
    subStructFileRenameInp_t* _inp ) {
    return _api->call_handler<
               subStructFileRenameInp_t*>(
                   _comm,
                   _inp);
}

int call_dataObjInp_genQueryOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*     _inp,
    genQueryOut_t**   _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               genQueryOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_subStructFileFdOprInp_rodsDirentOut(
    irods::api_entry*        _api,
    rsComm_t*                _comm,
    subStructFileFdOprInp_t* _inp,
    rodsDirent_t**           _out ) {
    return _api->call_handler<
               subStructFileFdOprInp_t*,
               rodsDirent_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_subFileInp_bytesBufOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    subFile_t* _inp,
    bytesBuf_t*  _out ) {
    return _api->call_handler<
               subFile_t*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _out);
}

int call_structFileExtAndRegInp(
    irods::api_entry*         _api,
    rsComm_t*                 _comm,
    structFileExtAndRegInp_t* _inp ) {
    return _api->call_handler<
               structFileExtAndRegInp_t*>(
                   _comm,
                   _inp);
}

int call_chkObjPermAndStat(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    chkObjPermAndStat_t* _inp ) {
    return _api->call_handler<
               chkObjPermAndStat_t*>(
                   _comm,
                   _inp);
}

int call_dataObjInp_rodsHostAddrOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*     _inp,
    rodsHostAddr_t**  _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               rodsHostAddr_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataObjInp_openStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*     _inp,
    openStat_t**  _out ) {
    return _api->call_handler<
               dataObjInp_t*,
               openStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_intInp_bytesBufOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    int*              _inp,
    bytesBuf_t*       _out ) {
    return _api->call_handler<
               int*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _out);
}

int call_getRescQuotaInp_rescQuotaOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    getRescQuotaInp_t* _inp,
    rescQuota_t**      _out ) {
    return _api->call_handler<
               getRescQuotaInp_t*,
               rescQuota_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_genQueryOutOut_genQueryOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    genQueryOut_t*    _inp,
    genQueryOut_t**   _out ) {
    return _api->call_handler<
               genQueryOut_t*,
               genQueryOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_bulkOprInp_bytesBufOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    bulkOprInp_t*     _inp,
    bytesBuf_t*       _out ) {
    return _api->call_handler<
               bulkOprInp_t*,
               bytesBuf_t*>(
                   _comm,
                   _inp,
                   _out);
}

int call_procStatInp_genQueryOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    procStatInp_t*    _inp,
    genQueryOut_t**   _out ) {
    return _api->call_handler<
               procStatInp_t*,
               genQueryOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataObjInfoInp_dataObjInfoOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInfo_t*    _inp,
    dataObjInfo_t**   _out ) {
    return _api->call_handler<
               dataObjInfo_t*,
               dataObjInfo_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_unregDataObjInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    unregDataObj_t*   _inp ) {
    return _api->call_handler<
               unregDataObj_t*>(
                   _comm,
                   _inp);
}

int call_regReplicaInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    regReplica_t*     _inp ) {
    return _api->call_handler<
               regReplica_t*>(
                   _comm,
                   _inp);
}

int call_fileChksumInp_charOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileChksumInp_t*  _inp,
    char**            _out ) {
    return _api->call_handler<
               fileChksumInp_t*,
               char**>(
                   _comm,
                   _inp,
                   _out);
}

int call_sslStartInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    sslStartInp_t*     _inp ) {
    return _api->call_handler<
               sslStartInp_t*>(
                   _comm,
                   _inp);
}

int call_sslEndInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    sslEndInp_t*     _inp ) {
    return _api->call_handler<
               sslEndInp_t*>(
                   _comm,
                   _inp);
}

int call_authPluginReqInp_authPluginReqOut(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    authPluginReqInp_t*  _inp,
    authPluginReqOut_t** _out ) {
    return _api->call_handler<
               authPluginReqInp_t*,
               authPluginReqOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_getHierarchyForRescInp_getHierarchyForRescOut(
    irods::api_entry*          _api,
    rsComm_t*                  _comm,
    getHierarchyForRescInp_t*  _inp,
    getHierarchyForRescOut_t** _out ) {
    return _api->call_handler<
               getHierarchyForRescInp_t*,
               getHierarchyForRescOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_bytesBufOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    bytesBuf_t**      _out ) {
    return _api->call_handler<
               bytesBuf_t**>(
                   _comm,
                   _out);
}

int call_get_hier_inp_get_hier_out(
    irods::api_entry*          _api,
    rsComm_t*                  _comm,
    get_hier_inp_t*  _inp,
    get_hier_out_t** _out ) {
    return _api->call_handler<
               get_hier_inp_t*,
               get_hier_out_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_setRoundRobinContextInp(
    irods::api_entry*          _api,
    rsComm_t*                  _comm,
    setRoundRobinContextInp_t*  _inp ) {
    return _api->call_handler<
               setRoundRobinContextInp_t*>(
                   _comm,
                   _inp);
}

int call_execRuleExpressionInp(
    irods::api_entry*       _api,
    rsComm_t*               _comm,
    exec_rule_expression_t* _inp) {
    return _api->call_handler<
               exec_rule_expression_t*>(
                   _comm,
                   _inp);

}
