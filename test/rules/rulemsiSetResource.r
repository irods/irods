acRegisterData {
  ON($objPath like "/home/collections.nvo/2mass/fits-images/*") {
    acCheckDataType("fits image");
    msiSetResource("testResc");
    msiRegisterData;
  }
}