myTestRule {
#Input parameters are:
#  string
#  separator
#Output parameters are:
#  head
#  tail
#Output from running the example is:
#  string is foo.bar.baz
#  head is foo.bar and tail is baz
   writeLine("stdout","string is *string");
   msiSplitPathByKey(*string,*separator,*head,*tail);
   writeLine("stdout","tail is *tail and head is *head");
}
INPUT *string="foo.bar.baz",*separator="."
OUTPUT ruleExecOut
