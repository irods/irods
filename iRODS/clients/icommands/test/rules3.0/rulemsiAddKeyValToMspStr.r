myTestRule {
#Input parameters are:
#  Attribute name
#  Attribute value
#Output from running the example is:
#  The string now contains 
#  destRescName=demoResc
   msiAddKeyValToMspStr(*AttrName,*AttrValue,*KeyValStr);
   writeLine("stdout","The string now contains");
   writeLine("stdout","*KeyValStr");
}
INPUT *AttrName="destRescName", *AttrValue="demoResc"
OUTPUT ruleExecOut
