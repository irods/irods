myTestRule {
# Input parameters:
#  srcFile     - iRODS image file that will be converted
#  srcOptions  - Optional srcFile property values
#               (only image format values are allowed if this parameter is used)
#  destFile    - iRODS image file that will be created to hold converted image
#  destOptions - Optional destFile properties string giving parameters for conversion
#               (only file format and compression flags are allowed)
#
# Uses ImageMagick.
#
# Set properties for destOptions... in this case, the compression
#

  # Get properties for srcFile - just for perusing
  msiImageGetProperties(*srcFile, "null", *Prop);
  writeLine("stdout", "Original file properties:");
  msiPrintKeyValPair("stdout", *Prop);

  # Get image format just for checking
  msiPropertiesGet(*Prop, "image.Compression", *compVal);
  writeLine("stdout", "");
  writeLine("stdout", "Original compression: *compVal");

  # Convert
  msiImageConvert(*srcFile,"null",*destFile,*destOptions);

  # Write a message to the server log
  writeLine("serverLog", "Converting *srcFile to *destFile");

  # Write a message to stdout
  writeLine("stdout", "");

  # Write out properties of new image file
  msiImageGetProperties(*destFile, "null", *Prop);
  writeLine("stdout", "Converted file properties:");
  msiPrintKeyValPair("stdout", *Prop);

  msiPropertiesGet(*Prop, "image.Compression", *compVal);
  writeLine("stdout", "");
  writeLine("stdout", "Compression of converted file: *compVal");
}
INPUT *srcFile="/tempZone/home/rods/image/ncdc.png", *destFile="/tempZone/home/rods/image/ncdc-recomp.png", *destOptions="<image.Compression>lossless</image.Compression>"
OUTPUT ruleExecOut
