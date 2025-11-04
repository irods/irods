myTestRule {
#Administrator command to cause update to iCAT quota tables
  delay("<PLUSET>30s</PLUSET><EF>24h</EF>") {
    msiQuota;
    writeLine("serverLog","Updated quota check");
  }
}
INPUT null
OUTPUT ruleExecOut
