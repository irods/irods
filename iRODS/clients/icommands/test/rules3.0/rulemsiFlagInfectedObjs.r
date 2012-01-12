myTestRule {
#Input parameter is:
#  Standard output from clamscan
#  Resource where clamscan was run
#Output parameter is:
#  Status
#Execute the clamscan (Clam AntiVirus) utility "clamscan -ri VAULT_DIR"
#  Note that the *VaultPath is the physical path for *Resource on *Host
#  clamscan looks at the physical files in the iRODS vault
  msiExecCmd("scanvault.py",*VaultPath,*Host,"null","null",*CmdOut);

  #Extract result of the scan of the files in the vault on the specified host
  msiGetStdoutInExecCmdOut(*CmdOut,*StdoutStr);
  msiGetSystemTime(*Time, "human");

  #Write result to an iRODS file
  msiDataObjCreate(*OutputObj ++ "." ++ *Time ++ ".txt","renci-vault1",*D_FD);
  msiDataObjWrite(*D_FD,*StdoutStr,*W_LEN);
  msiDataObjClose(*D_FD,*Status);
  writePosInt("stdout",*Status); writeline("stdout","");

  #Execute the routine to extract information from the output file
  msiFlagInfectedObjs(*OutputObj, *Resource, *Status);
}
INPUT *VaultPath="/home/rodsdev/loadingVault/", *Host="yellow.ils.unc.edu",*OutputObj="/tempZone/home/rods/loading/SCAN_RESULT", *Resource = "loadingResc"
OUTPUT ruleExecOut
