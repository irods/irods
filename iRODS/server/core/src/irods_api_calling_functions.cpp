

#include "irods_api_calling_functions.hpp"

#include "boost/any.hpp"
#include <functional>


int call_miscSvrInfo_out(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    miscSvrInfo_t**    _out ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               fileReadInp_t*>(
                   _comm,
                   _inp);
}

int call_fileMkdirInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileMkdirInp_t*   _inp ) {
    return _api->call_handler<
               rsComm_t*,
               fileMkdirInp_t*>(
                   _comm,
                   _inp);
}

int call_fileChmodInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileChmodInp_t*   _inp ) {
    return _api->call_handler<
               rsComm_t*,
               fileChmodInp_t*>(
                   _comm,
                   _inp);
}

int call_fileRmdirInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    fileRmdirInp_t*   _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               fileOpendirInp_t*>(
                   _comm,
                   _inp);
}

int call_fileClosedirInp(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    fileClosedirInp_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               dataObjCopyInp_t*,
               transferStat_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataOprInp_portalOprOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    dataObjInp_t*       _inp,
    portalOprOut_t**    _out ) {
    return _api->call_handler<
               rsComm_t*,
               dataObjInp_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               authRequestOut_t**>(
                   _comm,
                   _out);
}

int call_authResponseInp(
    irods::api_entry*  _api,
    rsComm_t*          _comm,
    authResponseInp_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               ticketAdminInp_t*>(
                   _comm,
                   _inp);
}

int call_getTempPasswordOut(
    irods::api_entry*      _api,
    rsComm_t*              _comm,
    getTempPasswordOut_t** _out ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               userAdminInp_t*>(
                   _comm,
                   _inp);
}

int call_generalRowInserInp(
    irods::api_entry*     _api,
    rsComm_t*             _comm,
    generalRowInsertInp_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
               generalRowInsertInp_t*>(
                   _comm,
                   _inp);
}

int call_generalRowPurgeInp(
    irods::api_entry*     _api,
    rsComm_t*             _comm,
    generalRowPurgeInp_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
               generalRowPurgeInp_t*>(
                   _comm,
                   _inp);
}

int call_intp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    int*              _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               modAVUMetadataInp_t*>(
                   _comm,
                   _inp);
}

int call_modAccessControlInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    modAccessControlInp_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
               modAccessControlInp_t*>(
                   _comm,
                   _inp);
}

int call_ruleExecModInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    ruleExecModInp_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
               ruleExecModInp_t*>(
                   _comm,
                   _inp);
}

int call_generalUpdateInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    generalUpdateInp_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
               generalUpdateInp_t*>(
                   _comm,
                   _inp);
}

int call_modDataObjMetaInp(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    modDataObjMeta_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               dataObjInp_t*,
               char**>(
                   _comm,
                   _inp,
                   _out);
}

int call_dataObjInp_rodsObjStatOut(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    dataObjInp_t*        _inp,
    rodsObjStat_t**   _out ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
               execCmd_t*,
               execCmdOut_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_subFileInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    subFile_t*        _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               subStructFileFdOprInp_t*,
               rodsDirent_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_getXmsgTicketInp_xmsgTicketInfoOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    getXmsgTicketInp_t* _inp,
    xmsgTicketInfo_t**  _out ) {
    return _api->call_handler<
               rsComm_t*,
               getXmsgTicketInp_t*,
               xmsgTicketInfo_t**>(
                   _comm,
                   _inp,
                   _out);
}

int call_sendXmsgInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    sendXmsgInp_t*    _inp ) {
    return _api->call_handler<
               rsComm_t*,
               sendXmsgInp_t*>(
                   _comm,
                   _inp);
}

int call_rcvXmsgInp_rcvXmsgOut(
    irods::api_entry*   _api,
    rsComm_t*           _comm,
    rcvXmsgInp_t* _inp,
    rcvXmsgOut_t**  _out ) {
    return _api->call_handler<
               rsComm_t*,
               rcvXmsgInp_t*,
               rcvXmsgOut_t**>(
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
               rsComm_t*,
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
               rsComm_t*,
               structFileExtAndRegInp_t*>(
                   _comm,
                   _inp);
}

int call_chkObjPermAndStat(
    irods::api_entry*    _api,
    rsComm_t*            _comm,
    chkObjPermAndStat_t* _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               unregDataObj_t*>(
                   _comm,
                   _inp);
}

int call_regReplicaInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    regReplica_t*     _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               sslStartInp_t*>(
                   _comm,
                   _inp);
}

int call_sslEndInp(
    irods::api_entry* _api,
    rsComm_t*         _comm,
    sslEndInp_t*     _inp ) {
    return _api->call_handler<
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
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
               rsComm_t*,
               setRoundRobinContextInp_t*>(
                   _comm,
                   _inp);
}








































































