myTestRule {
# Input parameters:
# srcFile  - iRODS image file that will be converted
# destFile - iRODS image file that will be created and hold converted image
#
# Uses ImageMagick.
#
# Conversion is guided by file extensions in file names.
#
#
# Call the microservice that converts the image from the type specified in
# *srcFile into the type specified in *destFile
  msiImageConvert(*srcFile,"null",*destFile,"null");

  # Write message to server log
  writeLine("serverLog", "Converting *srcFile to *destFile");
  
  # Write message to stdout
  writeLine("stdout", "Converting *srcFile to *destFile");
}
INPUT *srcFile="/tempZone/home/rods/image/ncdc.png", *destFile="/tempZone/home/rods/image/ncdc.gif"
OUTPUT ruleExecOut
