myTestRule {
#Input parameters are:
#  Number of seconds to sleep
#  Number of micro-seconds to sleep
#Output from running the example is:
#  Jun 01 2011 17:04:59
#  Jun 01 2011 17:05:09
    writeLine("stdout", timestr(time()));
    msiSleep(*Sec, *MicroSec);
    writeLine("stdout", timestr(time()));
}
INPUT *Sec="10", *MicroSec="0"
OUTPUT ruleExecOut

