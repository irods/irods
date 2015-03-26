myTestRule {
#Input parameters are:
#  IP address
#  License string from http://ws.fraudlabs.com/
   msiIp2location(*IpAddr,*License,*Loc);
   writeLine("stdout","Location is *Loc");
}
INPUT *IpAddr="132.249.32.95", *License="02-G34B-H86A"
OUTPUT ruleExecOut
