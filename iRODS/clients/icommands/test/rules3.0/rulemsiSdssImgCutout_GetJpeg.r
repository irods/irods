myTestRule {
#Input parameters are:
#  Right Ascension
#  Declination
#  Scaling factor
#  Image width
#  Image height
#  Optional parameter for SDSS web service
#Output parameter is:
#  Image buffer
  msiSdssImgCutout_GetJpeg(*RA, *DEC, *Scale, *Width, *Height, *Opt, *OutImg);
}
 INPUT *RA=$"185.72", *DEC=$"15.82", *Scale=$"0.396127", *Width=$"64", *Height=$"64", *Opt=$"GPST" 
OUTPUT ruleExecOut
