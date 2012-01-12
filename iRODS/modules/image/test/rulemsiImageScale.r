ScaleImage
{
# Input parameters
#	srcFile - input iRODS image file
#	srcProp - properties string associated with srcFile - OPTIONAL
#		  (to be used to indicate image format if needed)
#	xscale - scale factor in x
#	yscale - scale factor in y
#	destFile - destination iRODS image file
#	destProp - properties string associated with destFile (created if needed) - OPTIONAL
#		   (only file format and/or compression flags allowed)
#
#	
#	msiImageScale(*srcFile, "null", *xscale, *yscale, *destFile, *destProp);
  	msiImageScale(*srcFile, "null", *xscale, *yscale, *destFile, "null");
}
#INPUT *srcfile="/tempZone/home/rods/image/ncdc.png", *xscale=".5", *yscale=".5", *destFile="ncdc-half", *destProp="<image.Format>gif</image.Format>" 
INPUT *srcFile="/tempZone/home/rods/image/ncdc.png", *xscale=".5", *yscale=".5", *destFile="ncdc-half.png"
OUTPUT ruleExecOut
