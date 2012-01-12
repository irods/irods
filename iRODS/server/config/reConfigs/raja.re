acRegisterData { ON($dataType like "\*image*") {
  msiRegisterData ::: recover_msiRegisterData; 
  acExtractMetadataForImageForRaja; } }
# alternate registration
acRegisterData {ON($objPath like "/home/raja#sdsc/myImportantFiles*" && $dataSize > 10000000) {
  msiRegisterData ::: recover_msiRegisterData;
  msiQueue("msiReplicateData('hpss-sdsc') ::: recover_msiReplicateData;"); } }
# alternate registration
acRegisterData { ON($objPath like "/home/raja@sdsc/myImportantFiles*") {
  msiRegisterData :::recover_msiRegisterData;
  msiQueue("msiReplicateData('unix-sdsc') ::: recover_msiReplicateData;"); } }
# deletion policy
acDeleteData { ON($objPath like "/home/raja@sdsc/myProtectedFiles*") {
  cut; fail; } }
# resource selection policy
acGetResource { ON($dataSize > 10000000) { msiSetResource('hpss-sdsc'); } }
