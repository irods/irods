###############################################################
# --- Part of TPAP Accession workflow demo ---
#
# *scanResObj should be the output of 'clamscan -ri VAULT_DIR' 
# where VAULT_DIR is the vault path of resource *rescName
###############################################################
flagInfectedObjs {
	msiFlagInfectedObjs(*scanResObj, *rescName, *status);
	writePosInt('stdout',*status);
	writeLine('stdout','');
}
INPUT *scanResObj='/pho27/home/rods/scan_output.txt',*rescName='mydrive'
OUTPUT ruleExecOut