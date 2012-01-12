myTestRule {
#Input parameters are:
#  Select field
#  Function to apply to attribue
#Input/Output parameter:
#  GenQuery structure
#Output from running the example is:
#  List of sizes of collections in /tempZone/home/rods

  # initial select is on COLL_NAME
  msiMakeGenQuery(*Select,"COLL_NAME like '/tempZone/home/rods/%%'",*GenQInp);

  # add select on sum(DATA_SIZE)
  msiAddSelectFieldToGenQuery(*SelectAdd,*Function,*GenQInp);
  msiExecGenQuery(*GenQInp,*GenQOut);
  foreach(*GenQOut)
  {
    msiGetValByKey(*GenQOut,"DATA_SIZE",*Size);
    msiGetValByKey(*GenQOut,"COLL_NAME",*Coll);
    writeLine("stdout","For collection *Coll, the size of the files is *Size");
  }
}
INPUT *Select="COLL_NAME", *SelectAdd="DATA_SIZE", *Function="SUM"
OUTPUT ruleExecOut
