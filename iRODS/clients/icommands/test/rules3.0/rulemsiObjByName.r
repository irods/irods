myTestRule {
#Input parameter is:
#  Astronomical object name
#Output parameters are:
#  Right Ascension
#  Declination
#  Type of object
  msiObjByName(*objName,*RA,*DEC,*TYPE);
  writeLine("stdout","Right ascension is *RA, declination is *DEC, type is *Type");
}
INPUT *objName=$"m100"
OUTPUT ruleExecOut

