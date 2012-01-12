#Demo about ingesting into different collection$
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/nara/*") {msiSendMail("mark.conrad@nara.gov", "new file notification", "A new file $objPath has been added to collection"); }}
#acPostProcForPut {ON(($objPath like "/tempZone/home/rods/sizetest/*") && ($dataSize < "1000”)) { acWriteLine(“serverLog”,"using small resource"); }}
#acPostProcForPut {ON(($objPath like "/tempZone/home/rods/sizetest/*") && ($dataSize >= "1000”)) { acWriteLine(“serverLog”,"using large resource"); }}
acPostProcForPut {ON($objPath like "/tempZone/home/rods/versiontest/*") {
  msiGetIcatTime(*Time,"icat");
  msiDataObjCopy($objPath, $objPath++"\.V"++*Time, "nvoReplResc" ,*JStat); } }
acPostProcForPut {ON($objPath like "\*.email") {mDExtract("/tempZone/home/rods/MDtest/email\.tag", $objPath); }}
acPostProcForPut {ON($objPath like "/tempZone/home/rods/mytest/*") {
   delay("<PLUSET>1m</PLUSET>" ) {
   acWriteLine("serverLog","delayed by a minute message1");
   acWriteLine("serverLog","delayed by a minute message2"); } } }
acPostProcForPut {ON($objPath like "/tempZone/home/rods/nvo/*") {
  delay("<PLUSET>1m</PLUSET>") {
  msiSysReplDataObj("nvoReplResc","null"); } } }
acPostProcForPut {ON($objPath like "/tempZone/home/rods/tg/*") {
  msiSysReplDataObj("tgReplResc","null"); } }
mDExtract(*A,*B) {openObj(*A,*T_FD);  readObj( *T_FD, "10000",*R1_BUF);
  getTagsForKV( *R1_BUF, *TSP);  closeObj(*T_FD,*J1);  openObj( *B,*M_FD);
  readObj( *M_FD, "10000", *R2_BUF);  getKVPairsUsingTags( *R2_BUF,*TSP, *KVP);
  closeObj(*T_FD,*J1);  closeObj(*M_FD,*J2);  findObjType( *B,*OTYP);
  ingestBulkMD( *KVP, *B, *OTYP); }
#approval flag workflow rules
approvalFlagA(*MailTo) {
  msiMakeQuery("COLL_NAME", "META_COLL_ATTR_NAME = 'AFlag' and META_COLL_ATTR_VALUE = '1'", *QueryC);
  msiExecStrCondQuery(*QueryC,*GenCOut);
  foreach(*GenCOut) {
    msiGetValByKey(*GenCOut,"COLL_NAME",*C);
    msiMakeQuery("DATA_NAME","COLL_NAME = '*C'",*QueryD);
    msiExecStrCondQuery(*QueryD,*GenDOut);
    foreach(*GenDOut) {
      msiGetValByKey(*GenDOut,"DATA_NAME",*D);
      *Path = *C ++ "/" ++ *D;
      msiDataObjRepl(*Path,"nvoReplResc",*junk1);
      writeLine("stdout","Replicated *Path");
      msiString2KeyValPair("AFlag=1",*KVPD);
      msiAssociateKeyValuePairsToObj(*KVPD,*Path,"-d"); }
    msiString2KeyValPair("BFlag=1",*KVPC);
    msiAssociateKeyValuePairsToObj(*KVPC,*C,"-c");
    writeLine("stdout","Set BFlag = 1 for *C"); }
  sendStdoutAsEmail(*MailTo,"ApprovalFlagA Results"); }
approvalFlagA(*MailTo){ }
approvalFlagB(*TagFile,*MailTo) {
  msiMakeQuery("COLL_NAME", "META_COLL_ATTR_NAME = 'BFlag' and META_COLL_ATTR_VALUE = '1'",*QueryC);
  msiExecStrCondQuery(*QueryC,*GenCOut);
  foreach(*GenCOut) {
    msiGetValByKey(*GenCOut,"COLL_NAME",*C);
    msiMakeQuery("DATA_NAME","COLL_NAME = '*C'",*QueryD);
    msiExecStrCondQuery(*QueryD,*GenDOut);
    foreach(*GenDOut) {
       msiGetValByKey(*GenDOut,"DATA_NAME",*D);
       *Path = *C ++ "/" ++ *D;
       mDExtract(*TagFile,*Path);
       writeLine("stdout","Metadata Extracted for *C/*D");
       msiString2KeyValPair("BFlag=1",*KVPD);
       msiAssociateKeyValuePairsToObj(*KVPD,*Path,"-d"); }
    msiString2KeyValPair("CFlag=1",*KVP);
    msiAssociateKeyValuePairsToObj(*KVP,*C,"-c");
    writeLine("stdout","Set CFlag = 1 for *C"); }
  sendStdoutAsEmail(*MailTo,"ApprovalFlagB Results"); }
approvalFlagB(*TagFile,*MailTo) {  }
acStringBufferPut(*File,*Buffer,*Resc) {
  msiDataObjCreate(*File,*Resc,*D_FD);
  msiDataObjWrite(*D_FD,*Buffer,*WLen);
  msiDataObjClose(*D_FD,*Status2); }
acStringBufferPut(*File,*Buffer,*Resc) {
  msiDataObjOpen(*File,*D_FD);
  msiDataObjLseek(*D_FD,"0","SEEK_END",*Status1);
  msiDataObjWrite(*D_FD,*Buffer,*WLen);
  msiDataObjClose(*D_FD,*Status2); }
acMakeVersionOnPut {msiGetIcatTime(*Time,"icat"); msiDataObjCopy($objPath,$objPath++".V"++*Time,"nvoReplResc",*JStat);}
aatest(*A,*B) {ON((*A == "one") %% (*A == "two")) { assign(*B,"100"); } }
aatest(*A,*B) {ON(*A == "three") { assign(*B,"200"); } }
aatest(*A,*B) {ON((*A > "100") && (*A < "300")) {assign(*B,"500"); } }
aatest(*A,*B) { assign(*B,"9999"); }
acWriteLine(*A,*B) {writeLine(*A,*B); }
