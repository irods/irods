myTestRule {
#Placeholder for creating a rule for a new microservice
  msiDoSomething("", *keyValOut);
  writeKeyValPairs("stdout", *keyValOut, " : ");
}
INPUT null
OUTPUT ruleExecOut
