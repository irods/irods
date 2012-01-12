myTestRule {
#Input parameter is:
#  Buffer where string will be written
#  GenQueryInp string
#Output from running the example is:
# Selected Column 501 With Option 1
# Selected Column 407 With Option 4
# Condition Column 501 like '/tempZone/home/rods/%%'

  msiMakeGenQuery(*Select,"COLL_NAME like '/tempZone/home/rods/%%'",*GenQInp);

  # add select on sum(DATA_SIZE)
  msiAddSelectFieldToGenQuery(*SelectAdd,*Function,*GenQInp);
  msiPrintGenQueryInp("stdout",*GenQInp);
}
INPUT *Select="COLL_NAME", *SelectAdd="DATA_SIZE",*Function="SUM"
OUTPUT *GenQInp, ruleExecOut
