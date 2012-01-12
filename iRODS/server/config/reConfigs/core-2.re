acPostProcForPut {ON($objPath like "/tempZone/home/rods/nvo/*") {delay("<PLUSET>1m</PLUSET>") {msiSysReplDataObj("nvoReplResc","null");}}}
acPostProcForPut {ON($objPath like "/tempZone/home/rods/tg/*") {msiSysReplDataObj("tgReplResc","null");} }
