myTestRule {
#Workflow operator to specify that no other versions of the rule will be tried
#Output from running the example is:
#  ERROR: rcExecMyRule error.  status = -1089000 CUT_ACTION_PROCESSED_ERR
#  Level 0: DEBUG:
    print;
}

print {
  or {
    writeLine("serverLog", "print 1");
    cut;
    fail;
  }
  or {
    writeLine("serverLog", "print 2");
    succeed;
  }
}

INPUT null
OUTPUT ruleExecOut

