#Rules for nsfdemo1 collection
#
# Rules for choosing resources by size for files in nsfdemo1:
acSetRescSchemeForCreate {ON(($objPath like "/tempZone/home/rods/nsfdemo1/*") && ($dataSize < 1000)) {
  msiSetDefaultResc("nsfdemo1rescGrp","preferred"); } }
acSetRescSchemeForCreate {ON(($objPath like "/tempZone/home/rods/nsfdemo1/*") && ($dataSize >= 1000)) {
  msiSetDefaultResc("tgReplResc","forced"); } }
#
# Rules for Post-ingestion in nsfdemo1: has three parts
acPostProcForPut {ON($objPath like "/tempZone/home/rods/nsfdemo1/*") {
  acPostPutNsfDemo1r1; acPostPutNsfDemo1r2; acPostPutNsfDemo1r3; } }
#   -- first rule for type-based metadata extraction.
acPostPutNsfDemo1r1 {ON($objPath like "*.email") {mDExtract("/tempZone/home/rods/tagFiles/email.tag",$objPath); } }
acPostPutNsfDemo1r1 {ON($objPath like "*.htmll") {mDExtract("/tempZone/home/rods/tagFiles/html.tag",$objPath); } }
acPostPutNsfDemo1r1 { }
#   -- second rule for replication (small immediate, large delayed)
acPostPutNsfDemo1r2 {ON($dataSize < 1000) {msiDataObjRepl($objPath,"demoResc2",*Status); } }
acPostPutNsfDemo1r2 {ON($dataSize >= 1000) {delay("<PLUSET>1m</PLUSET>") {msiDataObjRepl($objPath,"demoResc2",*Status); } } }
acPostPutNsfDemo1r2 { }
#  -- third rule for BackingUp a Version of the File based on ownership and size
acPostPutNsfDemo1r3 {ON(($userNameClient == "rods") && ($dataSize < 1000)) {acMakeBackUpOnPut; } }
acPostPutNsfDemo1r3 { }
#
# Helper Rules
#
mDExtract(*A,*B) {openObj(*A,*T_FD); readObj( *T_FD, "100000",*R1_BUF); getTagsForKV( *R1_BUF, *TSP); closeObj(*T_FD,*J1);
  openObj( *B,*M_FD); readObj( *M_FD, "100000", *R2_BUF); getKVPairsUsingTags( *R2_BUF,*TSP, *KVP); closeObj(*T_FD,*J1); closeObj(*M_FD,*J2);
  findObjType( *B,*OTYP); ingestBulkMD( *KVP, *B, *OTYP); }
acMakeVersionOnPut {msiGetIcatTime(*Time,"icat"); msiDataObjCopy($objPath,$objPath++".V"++*Time,"nvoReplResc",*JStat); }
acMakeBackUpOnPut {msiGetIcatTime(*Time,"icat"); msiDataObjCopy($objPath,"/tempZone/backup"++$objPath++".V"++*Time,"nvoReplResc",*JStat); }
#############################################################################
#Demo about ingesting into different collections
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/nara/*") {
#  msiSendMail("mark.conrad@nara.gov", "new file notification", "A new file $objPath has been added to collection"); } }
#acPostProcForPut {ON(($objPath like "/tempZone/home/rods/sizetest/*") && ($dataSize < 1000)) {
#  acWriteLine("serverLog","using small resource"); } }
#acPostProcForPut {ON(($objPath like "/tempZone/home/rods/sizetest/*") && ($dataSize >= 1000)) {
#  acWriteLine("serverLog","using large resource"); } }
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/versiontest/*") {
#  msiGetIcatTime(*Time,"icat"); msiDataObjCopy($objPath,$objPath++".V"++*Time,"nvoReplResc",*JStat); } }
#acPostProcForPut {ON($objPath like "*.email") {
#  mDExtract("/tempZone/home/rods/MDtest/email.tag",$objPath); } }
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/mytest/*") {
#  delay("<PLUSET>1m</PLUSET>") {
#    acWriteLine("serverLog","delayed by a minute message1"); acWriteLine("serverLog","delayed by a minute message2"); } } }
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/nvo/*") {
#  delay("<PLUSET>1m</PLUSET>"){
#    msiSysReplDataObj("nvoReplResc","null"); } } }
#acPostProcForPut {ON($objPath like "/tempZone/home/rods/tg/*") { msiSysReplDataObj("tgReplResc","null"); } }
#approval flag workflow rules
approvalFlagA(*MailTo) {msiMakeQuery("COLL_NAME","META_COLL_ATTR_NAME = 'AFlag' and META_COLL_ATTR_VALUE = '1'",*QueryC);
  msiExecStrCondQuery(*QueryC,*GenCOut);
  foreach(*GenCOut) {
     msiGetValByKey(*GenCOut,"COLL_NAME",*C);
     msiMakeQuery("DATA_NAME","COLL_NAME = '*C'",*QueryD);
     msiExecStrCondQuery(*QueryD,*GenDOut);
     foreach(*GenDOut) {
        msiGetValByKey(*GenDOut,"DATA_NAME",*D);
        msiDataObjRepl(*C++"/"++*D,"nvoReplResc",*Status);
        writeLine("stdout","Replicated *C/*D");
        msiString2KeyValPair("AFlag=1",*KVPD);
        msiAssociateKeyValuePairsToObj(*KVPD,*C++"/"++*D,"-d"); }
     msiString2KeyValPair("BFlag=1",*KVPC);
     msiAssociateKeyValuePairsToObj(*KVPC,*C,"-c");
     writeLine("stdout","Set BFlag = 1 for *C"); }
 sendStdoutAsEmail(*MailTo,"ApprovalFlagA Results"); }
approvalFlagA(*MailTo) { }
approvalFlagB(*TagFile,*MailTo) {msiMakeQuery("COLL_NAME","META_COLL_ATTR_NAME = 'BFlag' and META_COLL_ATTR_VALUE = '1'",*QueryC);
  msiExecStrCondQuery(*QueryC,*GenCOut);
  foreach(*GenCOut) {
    msiGetValByKey(*GenCOut,"COLL_NAME",*C);
    msiMakeQuery("DATA_NAME","COLL_NAME = '*C'",*QueryD);
    msiExecStrCondQuery(*QueryD,*GenDOut);
    foreach(*GenDOut) {
      msiGetValByKey(*GenDOut,"DATA_NAME",*D);
      assign(*DataVal,*C++"/"++*D);
      mDExtract(*TagFile,*DataVal);
      writeLine("stdout","Metadata Extracted for *C/*D");
      msiString2KeyValPair("BFlag=1",*KVPD);
      msiAssociateKeyValuePairsToObj(*KVPD,*C++"/"++*D,"-d"); }
    msiString2KeyValPair("CFlag=1",*KVP);
    msiAssociateKeyValuePairsToObj(*KVP,*C,"-c");
    writeLine("stdout","Set CFlag = 1 for *C"); }
  sendStdoutAsEmail(*MailTo,"ApprovalFlagB Results"); }
#approvalFlagB(*TagFile,*MailTo) { }
acStringBufferPut(*File,*Buffer,*Resc) {msiDataObjCreate(*File,*Resc,*D_FD); msiDataObjWrite(*D_FD,*Buffer,*WLen); msiDataObjClose(*D_FD,*Status); }
acStringBufferPut(*File,*Buffer,*Resc) {msiDataObjOpen(*File,*D_FD); msiDataObjLseek(*D_FD,0,SEEK_END,*Status); 
  msiDataObjWrite(*D_FD,*Buffer,*WLen); msiDataObjClose(*D_FD,*Status1); }
#aatest(*A,*B) {ON((*A == "one") %% (*A == "two")) { assign(*B,"100"); } }
#aatest(*A,*B) {ON(*A == "three") {assign(*B,"200"); } }
#aatest(*A,*B) {ON((*A > "100") && (*A < "300")) {assign(*B,"500"); } }
#aatest(*A,*B) {assign(*B,"9999"); }
#acWriteLine(*A,*B) {writeLine(*A,*B); }
