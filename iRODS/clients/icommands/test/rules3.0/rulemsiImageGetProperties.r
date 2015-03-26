myTestRule {
# Input parameters:
# inFile - input iRODS file (with complete path)
# inOptions - list of options to query for 
# NB: The inOptions parameter seems to have no effect; all 
# properties are returned, independent of the value of inOptions.
#
# Output parameter:
# outProperties - string containing returned properties list
#

  # Call microservice to get the image properties
  msiImageGetProperties(*inFile, *inOptions, *outProperties);

  # Write message and all properties to stdout
  writeLine("stdout", "Getting properties of *inFile");
  writeLine("stdout", "");
  msiPrintKeyValPair("stdout", *outProperties);

  # Write message to server log
  writeLine("serverLog", "Getting properties of *inFile");

  # Write out value of a specific property
  msiPropertiesGet(*outProperties, "image.Compression", *val);  
  writeLine("stdout","");
  writeLine("stdout", "Value of image.Compression: *val");
}
INPUT *inFile="/tempZone/home/rods/image/ncdc.png", *inOptions="null"
OUTPUT ruleExecOut 
