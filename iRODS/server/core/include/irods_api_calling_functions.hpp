
#ifndef IRODS_API_CALLING_FUNCTIONS_HPP
#define IRODS_API_CALLING_FUNCTIONS_HPP

#include "rcConnect.h"
#include "getMiscSvrInfo.h"
#include "fileCreate.h"
#include "fileOpen.h"
#include "apiHeaderAll.h"

#include "apiHandler.hpp"

#ifdef RODS_SERVER
int call_miscSvrInfo_out(
    irods::api_entry*,
    rsComm_t*,
    miscSvrInfo_t**);
#define CALL_MISCSVRINFOOUT call_miscSvrInfo_out
#else
#define CALL_MISCSVRINFOOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileCreateInp_fileCreateOut(
    irods::api_entry*,
    rsComm_t*,
    fileCreateInp_t*,
    fileCreateOut_t**);
#define CALL_FILECREATEINP_FILECREATEOUT call_fileCreateInp_fileCreateOut
#else
#define CALL_FILECREATEINP_FILECREATEOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileOpenInp(
    irods::api_entry*,
    rsComm_t*,
    fileOpenInp_t*);
#define CALL_FILEOPENINP call_fileOpenInp
#else
#define CALL_FILEOPENINP NULL
#endif

#ifdef RODS_SERVER
int call_fileWriteInp_bytesBufInp(
    irods::api_entry*,
    rsComm_t*,
    fileWriteInp_t*,
    bytesBuf_t*);
#define CALL_FILEWRITEINP_BYTESBUFINP call_fileWriteInp_bytesBufInp
#else
#define CALL_FILEWRITEINP_BYTESBUFINP NULL
#endif

#ifdef RODS_SERVER
int call_fileCloseInp(
    irods::api_entry*,
    rsComm_t*,
    fileCloseInp_t*);
#define CALL_FILECLOSEINP call_fileCloseInp
#else
#define CALL_FILECLOSEINP NULL
#endif

#ifdef RODS_SERVER
int call_fileLseekInp_fileLseekOut(
    irods::api_entry*,
    rsComm_t*,
    fileLseekInp_t*,
    fileLseekOut_t**);
#define CALL_FILELSEEKINP_FILELSEEKOUT call_fileLseekInp_fileLseekOut
#else
#define CALL_FILELSEEKINP_FILELSEEKOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileReadInp_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    fileReadInp_t*,
    bytesBuf_t*);
#define CALL_FILEREADINP_BYTESBUFOUT call_fileReadInp_bytesBufOut
#else
#define CALL_FILEREADINP_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileReadInp(
    irods::api_entry*,
    rsComm_t*,
    fileReadInp_t*);
#define CALL_FILEREADINP call_fileReadInp
#else
#define CALL_FILEREADINP NULL
#endif

#ifdef RODS_SERVER
int call_fileMkdirInp(
    irods::api_entry*,
    rsComm_t*,
    fileMkdirInp_t*);
#define CALL_FILEMKDIRINP call_fileMkdirInp
#else
#define CALL_FILEMKDIRINP NULL
#endif

#ifdef RODS_SERVER
int call_fileMkdirInp(
    irods::api_entry*,
    rsComm_t*,
    fileMkdirInp_t*);
#define CALL_FILEMKDIRINP call_fileMkdirInp
#else
#define CALL_FILEMKDIRINP NULL
#endif

#ifdef RODS_SERVER
int call_fileChmodInp(
    irods::api_entry*,
    rsComm_t*,
    fileChmodInp_t*);
#define CALL_FILECHMODINP call_fileChmodInp
#else
#define CALL_FILECHMODINP NULL
#endif

#ifdef RODS_SERVER
int call_fileRmdirInp(
    irods::api_entry*,
    rsComm_t*,
    fileRmdirInp_t*);
#define CALL_FILERMDIRINP call_fileRmdirInp
#else
#define CALL_FILERMDIRINP NULL
#endif

#ifdef RODS_SERVER
int call_fileStatInp_rodsStatOut(
    irods::api_entry*,
    rsComm_t*,
    fileStatInp_t*,
    rodsStat_t**);
#define CALL_FILESTATINP_RODSSTATOUT call_fileStatInp_rodsStatOut
#else
#define CALL_FILESTATINP_RODSSTATOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileGetFreespaceInp_fileGetFreespaceOut(
    irods::api_entry*,
    rsComm_t*,
    fileGetFsFreeSpaceInp_t*,
    fileGetFsFreeSpaceOut_t**);
#define CALL_FILEGETFREESPACEINP_FILEGETFREESPACEOUT call_fileGetFreespaceInp_fileGetFreespaceOut
#else
#define CALL_FILEGETFREESPACEINP_FILEGETFREESPACEOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileOpendirInp(
    irods::api_entry*,
    rsComm_t*,
    fileOpendirInp_t*);
#define CALL_FILEOPENDIRINP call_fileOpendirInp
#else
#define CALL_FILEOPENDIRINP NULL
#endif

#ifdef RODS_SERVER
int call_fileClosedirInp(
    irods::api_entry*,
    rsComm_t*,
    fileClosedirInp_t*);
#define CALL_FILECLOSEDIRINP call_fileClosedirInp
#else
#define CALL_FILECLOSEDIRINP NULL
#endif

#ifdef RODS_SERVER
int call_fileReaddirInp_rodsDirentOut(
    irods::api_entry*,
    rsComm_t*,
    fileReaddirInp_t*,
    rodsDirent_t**);
#define CALL_FILEREADDIRINP_RODSDIRENTOUT call_fileReaddirInp_rodsDirentOut
#else
#define CALL_FILEREADDIRINP_RODSDIRENTOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileRenameInp_fileRenameOut(
    irods::api_entry*,
    rsComm_t*,
    fileRenameInp_t*,
    fileRenameOut_t**); 
#define CALL_FILERENAMEINP_FILERENAMEOUT call_fileRenameInp_fileRenameOut
#else
#define CALL_FILERENAMEINP_FILERENAMEOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileOpenInp_bytesBufInp_filePutOut(
    irods::api_entry*,
    rsComm_t*,
    fileOpenInp_t*,
    bytesBuf_t*,
    filePutOut_t**);
#define CALL_FILEOPENINP_BYTESBUFINP_FILEPUTOUT call_fileOpenInp_bytesBufInp_filePutOut
#else
#define CALL_FILEOPENINP_BYTESBUFINP_FILEPUTOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileOpenInp_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    fileOpenInp_t*,
    bytesBuf_t*);
#define CALL_FILEOPENINP_BYTESBUFOUT call_fileOpenInp_bytesBufOut
#else
#define CALL_FILEOPENINP_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_fileStageSyncInp(
    irods::api_entry*,
    rsComm_t*,
    fileStageSyncInp_t*);
#define CALL_FILESTAGESYNCINP call_fileStageSyncInp
#else
#define CALL_FILESTAGESYNCINP NULL
#endif

#ifdef RODS_SERVER
int call_fileStageSyncInp_fileSyncOut(
    irods::api_entry*,
    rsComm_t*,
    fileStageSyncInp_t*,
    fileSyncOut_t**);
#define CALL_FILESTAGESYNCINP_FILESYNCOUT call_fileStageSyncInp_fileSyncOut
#else
#define CALL_FILESTAGESYNCINP_FILESYNCOUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*);
#define CALL_DATAOBJINP call_dataObjInp
#else
#define CALL_DATAOBJINP NULL
#endif

#ifdef RODS_SERVER
int call_openedDataObjInp_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    openedDataObjInp_t*,
    bytesBuf_t*);
#define CALL_OPENEDDATAOBJINP_BYTESBUFOUT call_openedDataObjInp_bytesBufOut
#else
#define CALL_OPENEDDATAOBJINP_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_openedDataObjInp(
    irods::api_entry*,
    rsComm_t*,
    openedDataObjInp_t*);
#define CALL_OPENEDDATAOBJINP call_openedDataObjInp
#else
#define CALL_OPENEDDATAOBJINP NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_bytesBufInp_portalOprOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    bytesBuf_t*,
    portalOprOut_t**);
#define CALL_DATAOBJINP_BYTESBUFINP_PORTALOPROUT call_dataObjInp_bytesBufInp_portalOprOut
#else
#define CALL_DATAOBJINP_BYTESBUFINP_PORTALOPROUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_portalOprOut_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    portalOprOut_t**,
    bytesBuf_t*);
#define CALL_DATAOBJINP_PORTALOPROUT_BYTESBUFOUT call_dataObjInp_portalOprOut_bytesBufOut
#else
#define CALL_DATAOBJINP_PORTALOPROUT_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_transferStatOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    transferStat_t**);
#define CALL_DATAOBJINP_TRANSFERSTATOUT call_dataObjInp_transferStatOut
#else
#define CALL_DATAOBJINP_TRANSFERSTATOUT NULL
#endif

#ifdef RODS_SERVER
int call_openedDataObjInp_fileLseekOut(
    irods::api_entry*,
    rsComm_t*,
    openedDataObjInp_t*,
    fileLseekOut_t**);
#define CALL_OPENEDDATAOBJINP_FILELSEEKOUT call_openedDataObjInp_fileLseekOut
#else
#define CALL_OPENEDDATAOBJINP_FILELSEEKOUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjCopyInp_transferStatOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjCopyInp_t*,
    transferStat_t**);
#define CALL_DATAOBJCOPYINP_TRANFERSTATOUT call_dataObjCopyInp_transferStatOut
#else
#define CALL_DATAOBJCOPYINP_TRANFERSTATOUT NULL
#endif

#ifdef RODS_SERVER
int call_dataOprInp_portalOprOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    portalOprOut_t**);
#define CALL_DATAOPRINP_PORTALOPROUT call_dataOprInp_portalOprOut
#else
#define CALL_DATAOPRINP_PORTALOPROUT NULL
#endif

#ifdef RODS_SERVER
int call_dataCopyInp(
    irods::api_entry*,
    rsComm_t*,
    dataCopyInp_t*);
#define CALL_DATACOPYINP call_dataCopyInp
#else
#define CALL_DATACOPYINP NULL
#endif

#ifdef RODS_SERVER
int call_simpleQueryInp_simpleQueryOut(
    irods::api_entry*,
    rsComm_t*,
    simpleQueryInp_t*,
    simpleQueryOut_t**);
#define CALL_SIMPLEQUERYINP_SIMPLEQUERYOUT call_simpleQueryInp_simpleQueryOut
#else
#define CALL_SIMPLEQUERYINP_SIMPLEQUERYOUT NULL
#endif

#ifdef RODS_SERVER
int call_generalAdminInp(
    irods::api_entry*,
    rsComm_t*,
    generalAdminInp_t*);
#define CALL_GENERALADMININP call_generalAdminInp
#else
#define CALL_GENERALADMININP NULL
#endif

#ifdef RODS_SERVER
int call_genQueryInp_genQueryOut(
    irods::api_entry*,
    rsComm_t*,
    genQueryInp_t*,
    genQueryOut_t**);
#define CALL_GENQUERYINP_GENQUERYOUT call_genQueryInp_genQueryOut
#else
#define CALL_GENQUERYINP_GENQUERYOUT NULL
#endif

#ifdef RODS_SERVER
int call_authRequestOut(
    irods::api_entry*,
    rsComm_t*,
    authRequestOut_t**);
#define CALL_AUTHREQUESTOUT call_authRequestOut
#else
#define CALL_AUTHREQUESTOUT NULL
#endif

#ifdef RODS_SERVER
int call_authResponseInp(
    irods::api_entry*,
    rsComm_t*,
    authResponseInp_t*);
#define CALL_AUTHRESPONSEINP call_authResponseInp
#else
#define CALL_AUTHRESPONSEINP NULL
#endif

#ifdef RODS_SERVER
int call_authCheckInp_authCheckOut(
    irods::api_entry*,
    rsComm_t*,
    authCheckInp_t*,
    authCheckOut_t**);
#define CALL_AUTHCHECKINP_AUTHCHECKOUT call_authCheckInp_authCheckOut
#else
#define CALL_AUTHCHECKINP_AUTHCHECKOUT NULL
#endif

#ifdef RODS_SERVER
int call_endTransactionInp(
    irods::api_entry*,
    rsComm_t*,
    endTransactionInp_t*);
#define CALL_ENDTRANSACTIONINP call_endTransactionInp
#else
#define CALL_ENDTRANSACTIONINP NULL
#endif

#ifdef RODS_SERVER
int call_specificQueryInp_genQueryOut(
    irods::api_entry*,
    rsComm_t*,
    specificQueryInp_t*,
    genQueryOut_t**);
#define CALL_SPECIFICQUERYINP_GENQUERYOUT call_specificQueryInp_genQueryOut
#else
#define CALL_SPECIFICQUERYINP_GENQUERYOUT NULL
#endif

#ifdef RODS_SERVER
int call_ticketAdminInp(
    irods::api_entry*,
    rsComm_t*,
    ticketAdminInp_t*);
#define CALL_TICKETADMININP call_ticketAdminInp
#else
#define CALL_TICKETADMININP NULL
#endif

#ifdef RODS_SERVER
int call_getTempPasswordForOtherInp_getTempPasswordForOtherOut(
    irods::api_entry*,
    rsComm_t*,
    getTempPasswordForOtherInp_t*,
    getTempPasswordForOtherOut_t**);
#define CALL_GETTEMPPASSWORDFOROTHERINP_GETTEMPPASSWORDFOROTHEROUT call_getTempPasswordForOtherInp_getTempPasswordForOtherOut
#else
#define CALL_GETTEMPPASSWORDFOROTHERINP_GETTEMPPASSWORDFOROTHEROUT NULL
#endif

#ifdef RODS_SERVER
int call_pamAuthRequestInp_pamAuthRequestOut(
    irods::api_entry*,
    rsComm_t*,
    pamAuthRequestInp_t*,
    pamAuthRequestOut_t**);
#define CALL_PAMAUTHREQUESTINP_PAMAUTHREQUESTOUT call_pamAuthRequestInp_pamAuthRequestOut
#else
#define CALL_PAMAUTHREQUESTINP_PAMAUTHREQUESTOUT NULL
#endif

#ifdef RODS_SERVER
int call_getLimitedPasswordInp_getLimitedPasswordOut(
    irods::api_entry*,
    rsComm_t*,
    getLimitedPasswordInp_t*,
    getLimitedPasswordOut_t**);
#define CALL_GETLIMITEDPASSWORDINP_GETLIMITEDPASSWORDOUT call_getLimitedPasswordInp_getLimitedPasswordOut
#else
#define CALL_GETLIMITEDPASSWORDINP_GETLIMITEDPASSWORDOUT NULL
#endif

#ifdef RODS_SERVER
int call_collInp(
    irods::api_entry*,
    rsComm_t*,
    collInp_t*);
#define CALL_COLLINP call_collInp
#else
#define CALL_COLLINP NULL
#endif

#ifdef RODS_SERVER
int call_intp_collEntOut(
    irods::api_entry*,
    rsComm_t*,
    int*,
    collEnt_t**);
#define CALL_INTP_COLLENTOUT call_intp_collEntOut
#else
#define CALL_INTP_COLLENTOUT NULL
#endif

#ifdef RODS_SERVER
int call_userAdminInp(
    irods::api_entry*,
    rsComm_t*,
    userAdminInp_t*);
#define CALL_USERADMININP call_userAdminInp
#else
#define CALL_USERADMININP NULL
#endif

#ifdef RODS_SERVER
int call_generalRowInserInp(
    irods::api_entry*,
    rsComm_t*,
    generalRowInsertInp_t*);
#define CALL_GENERALROWINSERTINP call_generalRowInserInp
#else
#define CALL_GENERALROWINSERTINP NULL
#endif

#ifdef RODS_SERVER
int call_generalRowPurgeInp(
    irods::api_entry*,
    rsComm_t*,
    generalRowPurgeInp_t*);
#define CALL_GENERALROWPURGEINP call_generalRowPurgeInp
#else
#define CALL_GENERALROWPURGEINP NULL
#endif

#ifdef RODS_SERVER
int call_intp(
    irods::api_entry*,
    rsComm_t*,
    int*);
#define CALL_INTP call_intp
#else
#define CALL_INTP NULL
#endif

#ifdef RODS_SERVER
int call_collInp_collOprStatOut(
    irods::api_entry*,
    rsComm_t*,
    collInp_t*,
    collOprStat_t**);
#define CALL_COLLINP_COLLOPRSTATOUT call_collInp_collOprStatOut
#else
#define CALL_COLLINP_COLLOPRSTATOUT NULL
#endif

#ifdef RODS_SERVER
int call_modAVUMetadataInp(
    irods::api_entry*,
    rsComm_t*,
    modAVUMetadataInp_t*);
#define CALL_MODAVUMETADATAINP call_modAVUMetadataInp
#else
#define CALL_MODAVUMETADATAINP NULL
#endif

#ifdef RODS_SERVER
int call_modAccessControlInp(
    irods::api_entry*,
    rsComm_t*,
    modAccessControlInp_t*);
#define CALL_MODACCESSCONTROLINP call_modAccessControlInp
#else
#define CALL_MODACCESSCONTROLINP NULL
#endif

#ifdef RODS_SERVER
int call_ruleExecModInp(
    irods::api_entry*,
    rsComm_t*,
    ruleExecModInp_t*);
#define CALL_RULEEXECMODINP call_ruleExecModInp
#else
#define CALL_RULEEXECMODINP NULL
#endif

#ifdef RODS_SERVER
int call_getTempPasswordOut(
    irods::api_entry*,
    rsComm_t*,
    getTempPasswordOut_t**);
#define CALL_GETTEMPPASSWORDOUT call_getTempPasswordOut
#else
#define CALL_GETTEMPPASSWORDOUT NULL
#endif

#ifdef RODS_SERVER
int call_generalUpdateInp(
    irods::api_entry*,
    rsComm_t*,
    generalUpdateInp_t*);
#define CALL_GENERALUPDATEINP call_generalUpdateInp
#else
#define CALL_GENERALUPDATEINP NULL
#endif

#ifdef RODS_SERVER
int call_modDataObjMetaInp(
    irods::api_entry*,
    rsComm_t*,
    modDataObjMeta_t*);
#define CALL_MODDATAOBJMETAINP call_modDataObjMetaInp
#else
#define CALL_MODDATAOBJMETAINP NULL
#endif

#ifdef RODS_SERVER
int call_ruleExecSubmitInp_charOut(
    irods::api_entry*,
    rsComm_t*,
    ruleExecSubmitInp_t*,
    char**);
#define CALL_RULEEXECSUBMITINP_CHAROUT call_ruleExecSubmitInp_charOut
#else
#define CALL_RULEEXECSUBMITINP_CHAROUT NULL
#endif

#ifdef RODS_SERVER
int call_ruleExecDelInp(
    irods::api_entry*,
    rsComm_t*,
    ruleExecDelInp_t*);
#define CALL_RULEEXECDELINP call_ruleExecDelInp
#else
#define CALL_RULEEXECDELINP NULL
#endif

#ifdef RODS_SERVER
int call_execMyRuleInp_msParamArrayOut(
    irods::api_entry*,
    rsComm_t*,
    execMyRuleInp_t*,
    msParamArray_t**);
#define CALL_EXECMYRULEINP_MSPARAMARRAYOUT call_execMyRuleInp_msParamArrayOut
#else
#define CALL_EXECMYRULEINP_MSPARAMARRAYOUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjCopyInp(
    irods::api_entry*,
    rsComm_t*,
    dataObjCopyInp_t*);
#define CALL_DATAOBJCOPYINP call_dataObjCopyInp
#else
#define CALL_DATAOBJCOPYINP NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_msParamArrayOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    msParamArray_t**);
#define CALL_DATAOBJINP_MSPARAMARRAYOUT call_dataObjInp_msParamArrayOut
#else
#define CALL_DATAOBJINP_MSPARAMARRAYOUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_charOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    char**);
#define CALL_DATAOBJINP_CHAROUT call_dataObjInp_charOut
#else
#define CALL_DATAOBJINP_CHAROUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_rodsObjStatOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    rodsObjStat_t**);
#define CALL_DATAOBJINP_RODSOBJSTATOUT call_dataObjInp_rodsObjStatOut
#else
#define CALL_DATAOBJINP_RODSOBJSTATOUT NULL
#endif

#ifdef RODS_SERVER
int call_execCmdInp_execCmdOut(
    irods::api_entry*,
    rsComm_t*,
    execCmd_t*,
    execCmdOut_t**);
#define CALL_EXECCMDINP_EXECCMDOUT call_execCmdInp_execCmdOut
#else
#define CALL_EXECCMDINP_EXECCMDOUT NULL
#endif

#ifdef RODS_SERVER
int call_subFileInp(
    irods::api_entry*,
    rsComm_t*,
    subFile_t*);
#define CALL_SUBFILEINP call_subFileInp
#else
#define CALL_SUBFILEINP NULL
#endif

#ifdef RODS_SERVER
int call_subStructFileFdOprInp_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    subStructFileFdOprInp_t*,
    bytesBuf_t*);
#define CALL_SUBSTRUCTFILEFDOPRINP_BYTESBUFOUT call_subStructFileFdOprInp_bytesBufOut
#else
#define CALL_SUBSTRUCTFILEFDOPRINP_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_subStructFileFdOprInp(
    irods::api_entry*,
    rsComm_t*,
    subStructFileFdOprInp_t*);
#define CALL_SUBSTRUCTFILEFDOPRINP call_subStructFileFdOprInp
#else
#define CALL_SUBSTRUCTFILEFDOPRINP NULL
#endif

#ifdef RODS_SERVER
int call_subFileInp_rodsStatOut(
    irods::api_entry*,
    rsComm_t*,
    subFile_t*,
    rodsStat_t**);
#define CALL_SUBFILEINP_RODSSTATOUT call_subFileInp_rodsStatOut
#else
#define CALL_SUBFILEINP_RODSSTATOUT NULL
#endif

#ifdef RODS_SERVER
int call_subStructFileLseekInp_fileLseekOut(
    irods::api_entry*,
    rsComm_t*,
    subStructFileLseekInp_t*,
    fileLseekOut_t**);
#define CALL_SUBSTRUCTLSEEKINP_FILELSEEKOUT call_subStructFileLseekInp_fileLseekOut
#else
#define CALL_SUBSTRUCTLSEEKINP_FILELSEEKOUT NULL
#endif

#ifdef RODS_SERVER
int call_subStructFileRenameInp(
    irods::api_entry*,
    rsComm_t*,
    subStructFileRenameInp_t*);
#define CALL_SUBSTRUCTFILERENAMEINP call_subStructFileRenameInp
#else
#define CALL_SUBSTRUCTFILERENAMEINP NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_genQueryOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    genQueryOut_t**);
#define CALL_DATAOBJINP_GENQUERYOUT call_dataObjInp_genQueryOut
#else
#define CALL_DATAOBJINP_GENQUERYOUT NULL
#endif

#ifdef RODS_SERVER
int call_subStructFileFdOprInp_rodsDirentOut(
    irods::api_entry*,
    rsComm_t*,
    subStructFileFdOprInp_t*,
    rodsDirent_t**);
#define CALL_SUBSTRUCTFILEFDOPRINP_RODSDIRENTOUT call_subStructFileFdOprInp_rodsDirentOut
#else
#define CALL_SUBSTRUCTFILEFDOPRINP_RODSDIRENTOUT NULL
#endif

#ifdef RODS_SERVER
int call_getXmsgTicketInp_xmsgTicketInfoOut(
    irods::api_entry*,
    rsComm_t*,
    getXmsgTicketInp_t*,
    xmsgTicketInfo_t**);
#define CALL_GETXMSGTICKETINP_XMSGTICKETINFOOUT call_getXmsgTicketInp_xmsgTicketInfoOut
#else
#define CALL_GETXMSGTICKETINP_XMSGTICKETINFOOUT NULL
#endif

#ifdef RODS_SERVER
int call_sendXmsgInp(
    irods::api_entry*,
    rsComm_t*,
    sendXmsgInp_t*);
#define CALL_SENDXMSGINP call_sendXmsgInp
#else
#define CALL_SENDXMSGINP NULL
#endif

#ifdef RODS_SERVER
int call_rcvXmsgInp_rcvXmsgOut(
    irods::api_entry*,
    rsComm_t*,
    rcvXmsgInp_t*,
    rcvXmsgOut_t**);
#define CALL_RCVXMSGINP_RCVXMSGOUT call_rcvXmsgInp_rcvXmsgOut
#else
#define CALL_RCVXMSGINP_RCVXMSGOUT NULL
#endif

#ifdef RODS_SERVER
int call_subFileInp_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    subFile_t*,
    bytesBuf_t*);
#define CALL_SUBFILEINP_BYTESBUFOUT call_subFileInp_bytesBufOut
#else
#define CALL_SUBFILEINP_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_structFileExtAndRegInp(
    irods::api_entry*,
    rsComm_t*,
    structFileExtAndRegInp_t*);
#define CALL_STRUCTFILEEXTANDREGINP call_structFileExtAndRegInp
#else
#define CALL_STRUCTFILEEXTANDREGINP NULL
#endif

#ifdef RODS_SERVER
int call_chkObjPermAndStat(
    irods::api_entry*,
    rsComm_t*,
    chkObjPermAndStat_t*);
#define CALL_CHKOBJPERMANDSTATINP call_chkObjPermAndStat
#else
#define CALL_CHKOBJPERMANDSTATINP NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_rodsHostAddrOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    rodsHostAddr_t**);
#define CALL_DATAOBJINP_RODSHOSTADDROUT call_dataObjInp_rodsHostAddrOut
#else
#define CALL_DATAOBJINP_RODSHOSTADDROUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInp_openStatOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInp_t*,
    openStat_t**);
#define CALL_DATAOBJINP_OPENSTATOUT call_dataObjInp_openStatOut
#else
#define CALL_DATAOBJINP_OPENSTATOUT NULL
#endif

#ifdef RODS_SERVER
int call_intInp_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    int*,
    bytesBuf_t*);
#define CALL_INTINP_BYTESBUFOUT call_intInp_bytesBufOut
#else
#define CALL_INTINP_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_getRescQuotaInp_rescQuotaOut(
    irods::api_entry*,
    rsComm_t*,
    getRescQuotaInp_t*,
    rescQuota_t**);
#define CALL_GETRESCQUOTAINP_RESCQUOTAOUT call_getRescQuotaInp_rescQuotaOut
#else
#define CALL_GETRESCQUOTAINP_RESCQUOTAOUT NULL
#endif

#ifdef RODS_SERVER
int call_genQueryOutOut_genQueryOut(
    irods::api_entry*,
    rsComm_t*,
    genQueryOut_t*,
    genQueryOut_t**);
#define CALL_GENQUERYOUTINP_GENQUERYOUT call_genQueryOutOut_genQueryOut
#else
#define CALL_GENQUERYOUTINP_GENQUERYOUT NULL
#endif

#ifdef RODS_SERVER
int call_bulkOprInp_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    bulkOprInp_t*,
    bytesBuf_t*);
#define CALL_BULKOPRINP_BYTESBUFOUT call_bulkOprInp_bytesBufOut
#else
#define CALL_BULKOPRINP_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_procStatInp_genQueryOut(
    irods::api_entry*,
    rsComm_t*,
    procStatInp_t*,
    genQueryOut_t**);
#define CALL_PROCSTATINP_GENQUERYOUT call_procStatInp_genQueryOut
#else
#define CALL_PROCSTATINP_GENQUERYOUT NULL
#endif

#ifdef RODS_SERVER
int call_dataObjInfoInp_dataObjInfoOut(
    irods::api_entry*,
    rsComm_t*,
    dataObjInfo_t*,
    dataObjInfo_t**);
#define CALL_DATAOBJINFOINP_DATAOBJINFOOUT call_dataObjInfoInp_dataObjInfoOut
#else
#define CALL_DATAOBJINFOINP_DATAOBJINFOOUT NULL
#endif

#ifdef RODS_SERVER
int call_unregDataObjInp(
    irods::api_entry*,
    rsComm_t*,
    unregDataObj_t*);
#define CALL_UNREGDATAOBJINP call_unregDataObjInp
#else
#define CALL_UNREGDATAOBJINP NULL
#endif

#ifdef RODS_SERVER
int call_regReplicaInp(
    irods::api_entry*,
    rsComm_t*,
    regReplica_t*);
#define CALL_REGREPLICAINP call_regReplicaInp
#else
#define CALL_REGREPLICAINP NULL
#endif

#ifdef RODS_SERVER
int call_fileChksumInp_charOut(
    irods::api_entry*,
    rsComm_t*,
    fileChksumInp_t*,
    char**);
#define CALL_FILECHKSUMINP_CHAROUT call_fileChksumInp_charOut
#else
#define CALL_FILECHKSUMINP_CHAROUT NULL
#endif

#ifdef RODS_SERVER
int call_sslStartInp(
    irods::api_entry*,
    rsComm_t*,
    sslStartInp_t*);
#define CALL_SSLSTARTINP call_sslStartInp
#else
#define CALL_SSLSTARTINP NULL
#endif

#ifdef RODS_SERVER
int call_sslEndInp(
    irods::api_entry*,
    rsComm_t*,
    sslEndInp_t*);
#define CALL_SSLENDINP call_sslEndInp
#else
#define CALL_SSLENDINP NULL
#endif

#ifdef RODS_SERVER
int call_authPluginReqInp_authPluginReqOut(
    irods::api_entry*,
    rsComm_t*,
    authPluginReqInp_t*,
    authPluginReqOut_t**);
#define CALL_AUTHPLUGINREQINP_AUTHPLUGINREQOUT call_authPluginReqInp_authPluginReqOut
#else
#define CALL_AUTHPLUGINREQINP_AUTHPLUGINREQOUT NULL
#endif

#ifdef RODS_SERVER
int call_getHierarchyForRescInp_getHierarchyForRescOut(
    irods::api_entry*,
    rsComm_t*,
    getHierarchyForRescInp_t*,
    getHierarchyForRescOut_t**);
#define CALL_GETHIERARCHYFORRESCINP_GETHIERARCHYFORRESCOUT call_getHierarchyForRescInp_getHierarchyForRescOut
#else
#define CALL_GETHIERARCHYFORRESCINP_GETHIERARCHYFORRESCOUT NULL
#endif

#ifdef RODS_SERVER
int call_bytesBufOut(
    irods::api_entry*,
    rsComm_t*,
    bytesBuf_t**);
#define CALL_BYTESBUFOUT call_bytesBufOut
#else
#define CALL_BYTESBUFOUT NULL
#endif

#ifdef RODS_SERVER
int call_get_hier_inp_get_hier_out(
    irods::api_entry*,
    rsComm_t*,
    get_hier_inp_t*,
    get_hier_out_t**);
#define CALL_GETHIERINP_GETHIEROUT call_get_hier_inp_get_hier_out
#else
#define CALL_GETHIERINP_GETHIEROUT NULL
#endif

#ifdef RODS_SERVER
int call_setRoundRobinContextInp(
    irods::api_entry*,
    rsComm_t*,
    setRoundRobinContextInp_t*);
#define CALL_SETROUNDROBINCONTEXTINP call_setRoundRobinContextInp
#else
#define CALL_SETROUNDROBINCONTEXTINP NULL
#endif






























































#endif // IRODS_API_CALLING_FUNCTIONS_HPP


