ConvertImage
{
# Input parameters:
#	srcFile     - iRODS image file that will be converted
#	srcOptions  - srcFile property options - OPTIONAL
#		      (only image format values are allowed if this parameter is used)
#	destFile    - iRODS image file that will be created to hold converted image
#	destOptions - destFile properties string giving parameters for conversion - OPTIONAL
#		      (only file format and compression flags are allowed)
#
# Uses ImageMagick.
#
# Set properties for destOptions... in this case, the image.Format.
#
# NB: in this example, the file extension in the destFile is omitted
# to illustrate the usage of destOptions.
#
# Get properties for srcFile for perusal
	msiImageGetProperties(*srcFile, "null", *Prop);
	writeLine("stdout", "Original file properties:");
	msiPrintKeyValPair("stdout", *Prop);
#	writeLine("stdout", *Prop);
# Get image format just for checking
	msiPropertiesGet(*Prop, "image.Format", *formatVal);
	writeLine("stdout", "");
	writeLine("stdout", "Original file format: *formatVal");
# Convert
  	msiImageConvert(*srcFile,"null",*destFile,*destOptions);
# Write a message to the server log
  	writeLine("serverLog", "Converting *srcFile to *destFile");
# Write a message to stdout
#  	writeLine("stdout", "Converting *srcFile to *destFile");
	writeLine("stdout", "");
#
# Write out properties of new image file
	msiImageGetProperties(*destFile, "null", *Prop);
#	writeLine("stdout", *Prop);
	writeLine("stdout", "Converted file properties:");
	msiPrintKeyValPair("stdout", *Prop);
	msiPropertiesGet(*Prop, "image.Format", *formatVal);
	writeLine("stdout", "");
	writeLine("stdout", "Format of converted file: *formatVal");
}
INPUT *srcFile="/tempZone/home/rods/image/ncdc.png", *destFile="/tempZone/home/rods/image/ncdc", *destOptions="<image.Format>gif</image.Format>"
OUTPUT ruleExecOut
