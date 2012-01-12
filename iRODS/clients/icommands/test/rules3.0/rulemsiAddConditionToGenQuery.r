myTestRule {
#Input parameters are:
#  Attribute name
#  Operator
#  Value
#Input/Output
#  General query structure
#Output from running the example is:
#  List of files in collection /tempZone/home/rods

  # initial condition for query corresponds to "COLL_NAME like '/tempZone/home/rods/%%'"
  msiMakeGenQuery(*Select,"COLL_NAME like '/tempZone/home/rods/%%'",*GenQInp);

  # adding condition to query "DATA_NAME like rule%%"
  msiAddConditionToGenQuery(*Attribute,*Operator,*Value,*GenQInp);
  msiExecGenQuery(*GenQInp,*GenQOut);
  foreach(*GenQOut)
  {
    msiGetValByKey(*GenQOut,"DATA_NAME",*DataFile);
    msiGetValByKey(*GenQOut,"COLL_NAME",*Coll);
    writeLine("stdout","*Coll/*DataFile");
  }
}
INPUT *Select="DATA_NAME, COLL_NAME", *Attribute="DATA_NAME", *Operator=" like ", *Value="rule%%"
OUTPUT ruleExecOut
