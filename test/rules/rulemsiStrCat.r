myTestRule {
#Input parameters are:
#  dest
#  source
#Output parameter is:
#  dest
#Output from running the example is:
#  dest is foo
#  source is bar
#  dest is now foobar
   writeLine("stdout","dest is *dest");
   writeLine("stdout","source is *source");
   msiStrCat(*dest,*source);
   writeLine("stdout","dest is now *dest");
}
INPUT *dest="foo",*source="bar"
OUTPUT ruleExecOut
