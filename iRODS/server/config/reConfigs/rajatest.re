pvacuum(*arg1,*arg2) {delayExecNew(msiTest,*arg2); }
acRegisterData {ON($dataType like "\*image*") {
  msiRegisterData ::: recover_msiRegisterData;
  acExtractMetadataForImageForRaja; } }
acRegisterData {ON($objPath like "/home/raja@sdsc/myImportantFiles*" && $dataSize > "10000000") {
  msiRegisterData ::: recover_msiRegisterData;
  msiQueue {
    msiReplicateData("hpss-sdsc") ::: recover_msiReplicateData; } } }
acRegisterData {ON($objPath like "/home/raja@sdsc/myImportantFiles*") {
  msiRegisterData ::: recover_msiRegisterData;
  msiReplicateData("unix-sdsc") ::: recover_msiReplicateData; } }
acDeleteData {ON($objPath like "/home/raja@sdsc/myProtectedFiles*") {
  cut; fail; } }
acRegisterData {ON($objPath like "/home/collections.nvo/2mass/fits-images/*") {
  cut; acCheckDataType("fits image");
  msiSetResource("nvo-image-resource");
  msiRegisterData ::: recover_msiRegisterData; 
  msiAddACLForDataToUser("2massusers.nvo","write") ::: recover_msiAddACLForDataToUser;
  msiExtractMetadataForFitsImage ::: recover_msiExtractMetadataForFitsImage; } }
acRegisterData {ON($objPath like "/home/collections.nvo/2mass/*") {
  acGetResource;
  msiRegisterData ::: recover_msiRegisterData;
  msiAddACLForDataToUser("2massusers.nvo","write") ::: recover_msiAddACLForDataToUser; } }
acDeleteData {ON($objPath like "/home/collections.nvo/2mass/*") {
  cut; msiCheckPermission("curate");
  msiDeleteData ::: recover_msiDeleteData; } }
acGetResource {ON($dataSize > "10000000") {
  msiSetResource("hpss-sdsc"); } }
acGetResource {ON($rescName == "null") {msiGetClosestResourceToClient; } }
acGetResource { }
acExtractMetadata {ON($dataType == "fits image") {
  msiExtractMetadataForFitsImage ::: recover_msiExtractMetadataForFitsImage; } }
acExtractMetadata {ON($dataType == "dicom image") {
  msiExtractMetadataForDicomData ::: recover_msiExtractMetadataForDicomData; } }
acDataObjCreate {acSetCreateConditions; acDOC; }
acSetCreateConditions {msiGetNewObjDescriptor ::: recover_msiGetNewObjDescriptornop;
  acSetResourceList; }
acDOC {msiPhyDataObjCreate ::: recover_msiPhyDataObjCreate;
  acRegisterData; 
  msiCommit ::: msiRollback; }
acSetResourceList {msiSetResourceList; }
acSetCopyNumber {msiSetCopyNumber; }
acRegisterData {msiRegisterData ::: msiRollback; }
acPostProcForPut {delay("<PLUSET>30s</PLUSET>") {msiReplDataObj(demoResc8); } }
