#ACTION {ON(CONDITION) {CALL_FUNCTION ::: ROLLBACK_FUNCTION; } } 
#
register_data {ON($objPath like "/home/collections.nvo/2mass/fits-images/*") {
  cut; check_data_type("fits image"); get_resource("nvo-image-resource");
  registerData ::: recover_registerData;
  addACLForDataToUser("2massusers.nvo","write") ::: recover_addACLForDataToUser;
  extractMetadataForFitsImage ::: recover_extractMetadataForFitsImage; } }
register_data {ON($objPath like "/home/collections.nvo/2mass/*") {
  get_resource("2mass-other-resource");
  registerData ::: recover_registerData;
  addACLForDataToUser("2massusers.nvo","write") ::: recover_addACLForDataToUser; } }
register_data {ON($dataType like "\*image*") {
  get_resource("null");
  registerData  ::: recover_registerData;
  extract_metadata_for_image; } }
register_data {get_resource("null"); registerData ::: recover_registerData; }
get_resource {ON($rescName != "null") { } }
get_resource {ON($dataSize > "10000000") {get_resource("hpss-sdsc"); } }
get_resource {getClosestResourceToClient; }
get_resource {getDefaultResourceForData; }
extract_metadata_for_image {ON($dataType == "fits image") {
  extractMetadataForFitsImage ::: recover_extractMetadataForFitsImage; } }
extract_metadata_for_image {ON($dataType == "dicom image") {
  extractMetadataForDicomData ::: recover_extractMetadataForDicomData; } }
