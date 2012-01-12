acRegisterData {ON($dataType like "\*image*") {
  msiRegisterData ::: recover_msiRegisterData;
  acExtractMetadata; } }
acRegisterData {ON($objPath like "/home/raja@sdsc/myImportantFiles/*" && $dataSize > "10000000") {
  msiRegisterData ::: recover_msiRegisterData;
  msiQueue {
   msiReplicateData("hpss-sdsc") ::: recover_msiReplicateData; } } }
acRegisterData {ON($objPath like "/home/raja@sdsc/myImportantFiles/*") {
  msiRegisterData ::: recover_msiRegisterData;
  msiReplicateData("unix-sdsc") ::: recover_msiReplicateData; } }
acSetResourceList {ON($dataSize > "10000000") {
  msiSetResource("hpss-sdsc"); } }
acExtractMetadata {ON($dataType == "fits image") {
  msiExtractMetadataForFitsImage ::: recover_msiExtractMetadataForFitsImage; } }
acExtractMetadata {ON($dataType == "dicom image") {
  msiExtractMetadataForDicomData ::: recover_msiExtractMetadataForDicomData; } }
