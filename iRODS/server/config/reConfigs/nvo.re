acRegisterData {ON($objPath like "/home/collections.nvo/2mass/fits-images/*") {
  cut;
  acCheckDataType("fits image");
  msiSetResource("nvo-image-resource");
  msiRegisterData ::: recover_msiRegisterData;
  msiAddACLForDataToUser("2massusers.nvo","write") ::: recover_msiAddACLForDataToUser;
  msiExtractMetadataForFitsImage ::: recover_msiExtractMetadataForFitsImage; } }
acRegisterData {ON($objPath like "/home/collections.nvo/2mass/*") {
  acGetResource;
  msiRegisterData ::: recover_msiRegisterData;
  msiAddACLForDataToUser("2massusers.nvo","write") ::: recover_msiAddACLForDataToUser; } }
acDeleteData {ON($objPath like "/home/collections.nvo/2mass/*") {
  msiCheckPermission("curate");
  msiDeleteData ::: recover_msiDeleteData; } }
acGetResource {ON($rescName == "null") {msiGetClosestResourceToClient; } }
