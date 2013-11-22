myTestRule {
   msiGetFormattedSystemTime(*myTime,"human","%d-%d-%d %ldh:%ldm:%lds");
   writeLine("stdout", "Workflow Executed Successfully at *myTime");
}
INPUT 