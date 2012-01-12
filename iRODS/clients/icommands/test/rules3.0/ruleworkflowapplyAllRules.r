myTestRule {
#Input parameters are:
#  Action to perform
#  Flag for whether to save REI structure, 1 is yes
#  Flag for whether to apply recursively, 1 is yes
#Output from executing the example is:
#  print 1
#  print 2
    applyAllRules(print, *SaveREI, *All);
}
print{
    or {
  	writeLine("stdout", "print 1");
    }
    or {
        writeLine("stdout", "print 2");
    }
}
INPUT *All="1", *SaveREI="0"
OUTPUT ruleExecOut

