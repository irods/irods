#!/usr/bin/perl
#
# This script helps convert an MCAT DB to an ICAT one.
#
# Currently, this is a preliminary version, but it does handle many
# cases.  For example, we were able to ingest an SRB file into the
# ICAT and get it via 'iget'.
#
# To run this script:
# 1) Move it to an empty directory.
# 2) Review and edit some the settings below.
# 3) For the first phase, be sure your version of the SRB client codes
#    has has the fixes committed for Spullmeta.c srbClientUtil.c by 
#    Wayne on Sept 18, 2008.
# 4) Set up your SRB environment for the MCAT you want to convert.
# 5) Run the script (and/or rerun).
#
# The script will say it has completed the SRB interaction.
# 
# 6) Set up your iRODS environment for your IRODS instance.
#    (You want, you can do this on another host, just move the files
#     after the Spullmeta phase).
# 7) Run the script again (it skips the SRB interaction).
#
# There are 3 phases to this script: 1) it runs Spullmeta to get
# information from the MCAT, 2) it generates iadmin, imeta, and psql
# commands and 3) it runs the commands.  For 1, it will skip that step
# if the Spull.log.* log files exist.  For 2 and 3 it will prompt you
# before doing or re-doing the steps.  Generally, it is OK to rerun
# this script.  You might want to review, and possibly edit, some of
# the command files before executing them.
#
# Note that if the text files get very large, the perl process may
# grow too large and hang your system.  If that happens, let us know
# and we'll develop a workaround to this perl limitation.
#
#
# You will need to change the following parameters for your
# particular MCAT and  ICATs and environment.
#
# Begin section to edit ---------------------------------------------------
# Conversion tables (cv_):
$cv_srbZone = "galvin";            # Your SRB zone name
$cv_irodsZone = "tempZone";   # Your iRODS zone name

# Special usernames to convert:
@cv_srb_usernames=("srbAdmin"); # username conversion table:srb form
@cv_irods_usernames=("rods");   # irods form for corresponding srb forms
@cv_srb_userdomains=("demo");   # Used for user.domain situations

# Special usernames to add
@cv_special_users=("vidarch.sils");  # Needed for sils instance
# if $useDomainWithUsername="0", above should be "vidarch".

# These below are used for ingesting users.
# Note that, by default, users of type 'staff' and 'sysadmin' are ingested.
# You might want to skip the sysadmin users, either by removing the "sysadmin"
# below or editing m2icat.cmds.iadmin after it is created.
@cv_srb_usertypes=("staff", "sysadmin");   # Users of these types are converted
@cv_irods_usertypes=("rodsuser", "rodsadmin");  # to these types.

# This flag indicates if you want user names to be of the form
# user#domain or just user.
$useDomainWithUsername = "1"; # 0 no, 1 yes

$SRB_BEGIN_DATE="1995-07-12"; # approximate date your SRB was installed;used to
                              # avoid some unneeded built-in items, etc.
$Spullmeta = "/scratch/slocal/srbtest/testc/SRB2_0_0rel/utilities/bin/Spullmeta";
                              # if in Path this could also be just "Spullmeta"
$iadmin = "iadmin";           # Change this to be full path if not in PATH.
$psql = "psql";               # And this too.
$imeta = "imeta";             # And this one.

$password_length = 6;         # The length of the random password strings
                              # used for users.  Note that you will need
                              # to send the strings (from the $iadminFile)
                              # to the users in some manner.  They can
                              # then change their password with 'ipasswd'.

$bypass_time_for_speed = 1;   # For debug/speed, don't convert time values
                              # but instead use the current time

$thisUser = 'rods';           # The irods user that this is being run as.

# End section to edit  ---------------------------------------------------

# You can, but don't need to, change these:
$logUser = "Spull.log.user";     # Spullmeta log file for users
$logColl = "Spull.log.coll";     # Spullmeta log file for collections
$logData = "Spull.log.data";     # Spullmeta log file for dataObjects
$logResc = "Spull.log.resc";     # Spullmeta log file for resources
$logLoc = "Spull.log.loc";       # Spullmeta log file for locations
$logMetaData = "Spull.log.meta.data"; # Spullmeta log file for user-defined
                                      # metadata for dataObjects
$logMetaColl = "Spull.log.meta.coll"; # Spullmeta log file for user-defined
                                      # metadata for collections
$sqlFile = "m2icat.cmds.sql"; # the output/input file: SQL (for psql)
$psqlLog = "psql.log";        # the log file when running psql
$iadminFile = "m2icat.cmds.iadmin"; # the output/input file for iadmin commands
$imetaFile = "m2icat.cmds.imeta";  # the output/input file for imeta commands
$imetaLog  = "imeta.log";     # the log file when running imeta
$iadminLog  = "iadmin.log";   # the log file when running iadmin
$showOne="0";            # A debug option, if "1";

#------------
@cv_srb_resources =();   # SRB resource(s) being converted (created on the fly)
@cv_srb_location_name=();
@cv_srb_location_address=();
@cv_irods_resources=();  # corresponding name of SRB resource(s) in iRODS.
@datatypeList=();        # filled and used dynamically
$nowTime="";
$metadataLastColl="";
$metadataLastDataname="";

$SRBMode=0;
if (!-e $logUser || -s $logUser == 0) {
    runCmd(0, "$Spullmeta -F GET_CHANGED_USER_INFO $SRB_BEGIN_DATE > $logUser");
    $SRBMode=1;
}

if (!-e $logResc || -s $logResc == 0) {
    runCmd(0, "$Spullmeta -F GET_CHANGED_PHYSICAL_RESOURCE_CORE_INFO $SRB_BEGIN_DATE > $logResc");
    $SRBMode=1;
}

if (!-e $logLoc  || -s $logLoc == 0) {
    runCmd(0, "$Spullmeta -F GET_LOCATION_INFO > $logLoc");
    $SRBMode=1;
}

if (!-e $logColl || -s $logColl == 0) {
    runCmd(0, "$Spullmeta -F GET_CHANGED_COLL_CORE_INFO $SRB_BEGIN_DATE > $logColl");
    $SRBMode=1;
}

if (!-e $logData || -s $logData == 0) {
    runCmd(0, "$Spullmeta -F GET_CHANGED_DATA_CORE_INFO $SRB_BEGIN_DATE > $logData");
    $SRBMode=1;
}

if (!-e $logMetaData) {
    runCmd(0, "$Spullmeta -F GET_CHANGED_DATA_UDEFMETA_INFO $SRB_BEGIN_DATE > $logMetaData");
    $SRBMode=1;
}

if (!-e $logMetaColl) {
    runCmd(1, "$Spullmeta -F GET_CHANGED_COLL_UDEFMETA_INFO $SRB_BEGIN_DATE > $logMetaColl");
    $SRBMode=1;
}

if ($SRBMode==1) {
    printf("\nSRB interaction completed.\n");
    printf("Run this script again to interact with iRODS.\n");
    printf("You will first need to set up your irods environment.\n");
    printf("If needed, you can move this script and the log files\n");
    printf("(from the SRB interaction) to another host and continue there.\n\n");
    exit();
}

my $doMainStep="yes";
if ( -e $sqlFile ) {
    printf("Enter y or yes if you want this script to regenerate the command\n");
    printf("files ($sqlFile, $iadminFile, and $imetaFile):");
    my $answer = <STDIN>;
    chomp( $answer );	# remove trailing return
    if ($answer eq "yes" || $answer eq "y") {
    }
    else {
	$doMainStep="no";
    }
}

if ($doMainStep eq "yes") {

    print("Generating command files\n");
    if ( open(  SQL_FILE, ">$sqlFile" ) == 0 ) {
	die("open failed on output file " . $sqlFile);
    }
    if ( open(  IADMIN_FILE, ">$iadminFile" ) == 0 ) {
	die("open failed on output file " . $iadminFile);
    }
    if ( open(  IMETA_FILE, ">$imetaFile" ) == 0 ) {
	die("open failed on output file " . $iadminFile);
    }

    processLogFile($logUser);

    foreach $user (@cv_special_users) {
	print( IADMIN_FILE "mkuser $user rodsuser\n");
	$newPassword = randomPassword();
	print( IADMIN_FILE "moduser $user password $newPassword\n");
    }


    processLogFile($logLoc);

    processLogFile($logResc);

    processLogFile($logColl);

    processLogFile($logData);

    processLogFile($logMetaData);

    processLogFile($logMetaColl);

    foreach $dataType (@datatypeList) {
	print( IADMIN_FILE "at data_type \'$dataType\'\n");
    }
    print( IADMIN_FILE "quit\n");

    print( IMETA_FILE "quit\n");

    close( IADMIN_FILE );
    close( SQL_FILE );
    close( IMETA_FILE );

    printf("Command files $sqlFile, $iadminFile, and $imetaFile written\n");
}

$thisOS=`uname -s`;
chomp($thisOS);

printf("Enter y or yes if you want to run iadmin now:");
my $answer = <STDIN>;
chomp( $answer );	# remove trailing return
if ($answer eq "yes" || $answer eq "y") {
    if ($thisOS eq "SunOS" or $thisOS eq "AIX" or $thisOS eq "HP-UX") {
	runCmd (1, "iadmin -V < $iadminFile > $iadminLog 2>&1" );
    }
    else {
	runCmd (1, "iadmin -V < $iadminFile >& $iadminLog" );
    }
}

printf("Enter y or yes if you want to run psql now:");
my $answer = <STDIN>;
chomp( $answer );	# remove trailing return
if ($answer eq "yes" || $answer eq "y") {
    if ($thisOS eq "SunOS" or $thisOS eq "AIX" or $thisOS eq "HP-UX") {
	runCmd (0, "$psql ICAT < $sqlFile > $psqlLog 2>&1");
    }
    else {
	runCmd (0, "$psql ICAT < $sqlFile >& $psqlLog");
    }
}

printf("Enter y or yes if you run imeta now:");
my $answer = <STDIN>;
chomp( $answer );	# remove trailing return
if ($answer eq "yes" || $answer eq "y") {
    if ($thisOS eq "SunOS" or $thisOS eq "AIX" or $thisOS eq "HP-UX") {
	runCmd (0, "$imeta < $imetaFile > $imetaLog 2>&1");
    }
    else {
	runCmd (0, "$imeta < $imetaFile >& $imetaLog");
    }
}

# run a command
# if option is 0 (normal), check the exit code and fail if non-0
# if 1, don't care
# if 2, should get a non-zero result, exit if not
sub runCmd {
    my($option, $cmd) = @_;
    print "running: $cmd \n";
    $cmdStdout=`$cmd`;
    $cmdStat=$?;
    if ($option == 0) {
	if ($cmdStat!=0) {
	    print "The following command failed:";
	    print "$cmd \n";
	    print "Exit code= $cmdStat \n";
	    die("command failed");
	}
    }
    if ($option == 2) {
	if ($cmdStat==0) {
	    print "The following command should have failed:";
	    print "$cmd \n";
	    print "Exit code= $cmdStat \n";
	    die("command failed to fail");
	}
    }
}

# Using an iadmin option, convert srb to irods time values;
# also keep a cache list to avoid running the subprocess so often.
sub convertTime($)
{
    my ($inTime) = @_;
    $lastOutTime;
    $lastOutTimeConverted;
    $maxLastOutTime=10;

    if ($bypass_time_for_speed ) {
	getNow();
	return($nowTime);
    }

#   printf("inTime =%s\n",$inTime);
    $outTime = substr($inTime,0,10) . "." . substr($inTime, 11, 2) .
	":" . substr($inTime, 14, 2) . 	":" . substr($inTime, 17, 2);
#   printf("outTime=%s\n",$outTime);
    for ($i=0; $i<$maxLastOutTime; $i++) {
	if ($lastOutTime[$i] eq $outTime) {
	    return($lastOutTimeConverted[$i]);
	}
    }

    runCmd(0, "iadmin ctime str " . $outTime);
    $i = index($cmdStdout, "time: ");
    $outTimeConvert = substr($cmdStdout, $i+6);
    chomp($outTimeConvert);

    $lastOutTime[$ixLastOutTime] = $outTime;
    $lastOutTimeConverted[$ixLastOutTime] = $outTimeConvert;
    $ixLastOutTime++;
    if ($ixLastOutTime >= $maxLastOutTime) {
	$ixLastOutTime=0;
    }

    return($outTimeConvert);
}

# Using an iadmin option, get the current time in irods format
sub getNow() {
    if ($nowTime eq "") {
	runCmd(0, "iadmin ctime now");
	$i = index($cmdStdout, "time: ");
	$nowTime = substr($cmdStdout, $i+6);
	chomp($nowTime);
    }
}

# Using the defined arrays at the top, possibly convert a user name
# Also, if not special, do the define conversion
sub convertUser($$)
{
    my ($inUser, $inDomain) = @_;
    $k=0;
    foreach $user (@cv_srb_usernames) {
	if ($user eq $inUser) {
#	    printf("user=$user inUser=$inUser\n");
	    return ($cv_irods_usernames[$k]);
	}
	$k++;
    }
    if ($useDomainWithUsername) {
#	$newUserName=$v_user_name . "#" . $v_user_domain;
	$newUserName=$inUser. "." . $inDomain;
    }
    else {
#	$newUserName=$v_user_name;
	$newUserName=$inUser;
    }
    return($newUserName);
}

# Using the defined arrays at the top, possibly convert collection name
sub convertCollection($)
{
    my ($inColl) = @_;
    my $outColl = $inColl;

    if (index($outColl, "/home/") == 0) {
	$outColl =~ s\/home/\/$cv_irodsZone/home/\g;
    }

    $outColl =~ s\/$cv_srbZone/\/$cv_irodsZone/\g; 
    $k=0;
    foreach $user (@cv_srb_usernames) {
	$tmp = "/" . $user . "." . $cv_srb_userdomains[$k];
	$tmp2 = "/" . $cv_irods_usernames[$k];
	$outColl =~ s\$tmp\$tmp2\g; 
	$k++;
    }
#   printf("inColl=$inColl outColl=$outColl\n");
    return($outColl);
}

# convertCollections version, for dataObjects
sub convertCollectionForData($$$)
{
    my ($inColl, $inUser, $inDomain) = @_;
    my $outColl = $inColl;
    my $tmp;
    my $tmp2;

     if (index($outColl, "/home/") == 0) {
       $outColl =~ s\/home/\/$cv_irodsZone/home/\g;
     }

    $outColl =~ s\/$cv_srbZone/\/$cv_irodsZone/\g; 
    $k=0;
    foreach $user (@cv_srb_usernames) {
	$tmp = "/" . $user . "." . $cv_srb_userdomains[$k];
	$tmp2 = "/" . $cv_irods_usernames[$k];
	$outColl =~ s\$tmp\$tmp2\g; 
	$k++;
    }
    $tmp = "/" . $inUser . "." . $inDomain;
    my $newUser = convertUser($inUser, $inDomain);
    $tmp2 = "/" . $newUser;
#   printf("tmp=$tmp tmp2=$tmp2\n");
    $outColl =~ s\$tmp\$tmp2\g; 

#   printf("inColl=$inColl outColl=$outColl\n");
    return($outColl);
}

# Add item to the datatype array unless it's already there.
sub AddToDatatypeList($) {
    my($item) = @_;
    if ($item eq "generic") {return;}
    foreach $listItem (@datatypeList) {
        if ($listItem eq $item) {
            return;
        }
    }
    push(@datatypeList, $item);
}


sub processLogFile($) {
# First line in the log file is the run parameters, 
# 2nd is the column-names, the rest are data items.
    my($logFile) = @_;
    my @udsmd, @mkdirList, @mkdirSortList;
    if ( open(  LOG_FILE, "<$logFile" ) == 0 ) {
	die("open failed on input file " . $logFile);
    }
    $lineNum = 0;
    $mode = "";

    foreach $line ( <LOG_FILE> ) {
	$lineNum++;
	if ($lineNum==1) {
	    @cmdArgs = split('\|',$line);
#	    print ("cmdArgs[0]:" . $cmdArgs[0] . " " . "\n");
	    if ($cmdArgs[0] eq "GET_CHANGED_USER_INFO") {
		$mode="USER";
	    }
	    if ($cmdArgs[0] eq "GET_CHANGED_COLL_CORE_INFO") {
		$mode="COLL";
	    }
	    if ($cmdArgs[0] eq "GET_CHANGED_DATA_CORE_INFO") {
		$mode="DATA";
	    }
	    if ($cmdArgs[0] eq "GET_CHANGED_PHYSICAL_RESOURCE_CORE_INFO") {
		$mode="RESC";
	    }
	    if ($cmdArgs[0] eq "GET_LOCATION_INFO\n") {
		$mode="LOCATION";
	    }
	    if ($cmdArgs[0] eq "GET_CHANGED_DATA_UDEFMETA_INFO") {
		$mode="METADATA_DATA";
	    }
	    if ($cmdArgs[0] eq "GET_CHANGED_COLL_UDEFMETA_INFO") {
		$mode="METADATA_COLL";
	    }
	}
#	printf("MODE: $mode, lineNum: $lineNum\n");
	if ($lineNum==2) {
	    @names = split('\|',$line);
#	    print ("names[0]:" . $names[0] . " " . "\n");
	    if ($mode eq "") {
		die ("Unrecognized type of Spullmeta log file: $logFile");
	    }
            if ($mode eq "DATA") {
                parseDataHdr($line);
            }
	}
	if ($lineNum>2) { # regular lines
	    @values = split('\|',$line);
	    $doDataInsert=0;
	    if ($mode eq "DATA") {
		$v_resc = $values[$g_resc_name_ix];
		$k=0;
		foreach $resc (@cv_srb_resources) {
		    if ($resc eq $v_resc) {
			$newResource = $cv_irods_resources[$k];
			$doDataInsert=1;
		    }
		    $k++;
		}
	    }
	    if ($doDataInsert) {
		$v_dataName = $values[$g_data_name_ix];
		$v_dataTypeName = $values[$g_data_type_name_ix];
		$v_phyPath = $values[$g_path_name_ix];
		$v_size = $values[$g_size_ix];
		$v_owner_domain = $values[$g_data_owner_domain_ix];
		$v_owner = convertUser($values[$g_data_owner_ix], $v_owner_domain);
		$v_create_time = convertTime($values[$g_data_create_timestamp_ix]);
		$v_access_time = convertTime($values[$g_data_last_access_timestamp_ix]);
		$v_replica = $values[$g_container_repl_enum_ix];
#		$v_collection = convertCollectionForData($values[27],
#							 $v_owner,
#							 $v_owner_domain);
#		$v_collection = convertCollection($values[27]);
		$v_collection = convertCollection($values[$g_collection_cont_name_ix]);
		if (checkDoCollection($values[27])==1) {
		    print( SQL_FILE "begin;\n"); # begin/commit to make these 2 like 1
		    print( SQL_FILE "insert into r_data_main (data_id, coll_id, data_name, data_repl_num, data_version, data_type_name, data_size, resc_name, data_path, data_owner_name, data_owner_zone, data_is_dirty, create_ts, modify_ts) values ((select nextval('R_ObjectID')), (select coll_id from r_coll_main where coll_name ='$v_collection'), '$v_dataName', '$v_replica', ' ', '$v_dataTypeName', '$v_size', '$newResource', '$v_phyPath', '$v_owner', '$cv_irodsZone', '1', '$v_create_time', '$v_access_time');\n");

		    getNow();
		    print( SQL_FILE "insert into r_objt_access ( object_id, user_id, access_type_id , create_ts,  modify_ts) values ( (select currval('R_ObjectID')), (select user_id from r_user_main where user_name = '$v_owner'), '1200', '$nowTime', '$nowTime');\n");

		    print( SQL_FILE "commit;\n");

		    AddToDatatypeList($v_dataTypeName);

		    if ($lineNum==3 && $showOne=="1") {
			$j = 0;
			foreach $value (@values) {
			    print ($names[$j] . " " . $value . "\n");
			    $j++;
			}
		    }
		}
	    }
	    if ($mode eq "USER") {
		$v_user_type = $values[0];
		$v_user_addr = $values[1];
		$v_user_name = $values[2];
		$v_user_domain=$values[3];
		my $k=0;
		foreach $userType (@cv_srb_usertypes) {
		    if ($v_user_type eq $userType) {
			$newUserName=convertUser($v_user_name, $v_user_domain);
			print( IADMIN_FILE "mkuser $newUserName $cv_irods_usertypes[$k]\n");
			print( IADMIN_FILE "moduser $newUserName info $v_user_addr\n");
			$newPassword = randomPassword();
			print( IADMIN_FILE "moduser $newUserName password $newPassword\n");
		    }
		    $k++;
		}
	    }
	    if ($mode eq "COLL") {
		if (checkDoCollection($values[0])==1) {
		   $v_coll_name=convertCollection($values[0]);
		   $v_parent_coll_name=convertCollection($values[1]);
		   $v_coll_owner_domain=$values[5];
		   $v_coll_owner=convertUser($values[2],
				      $v_coll_owner_domain);
		   $v_coll_zone=$values[6];
		   $skipIt=0;
		   if ($values[2] eq "srb")    { $skipIt=1; }
		   if ($values[2] eq "npaci")  { $skipIt=1; }
		   if ($values[2] eq "public") { $skipIt=1; }
		   if ($values[2] eq "sdsc")   { $skipIt=1; }
		   if ($values[2] eq "testuser")   { $skipIt=1; }
		   if ($values[2] eq "ticketuser")  { $skipIt=1; }
		   # make sure it's in the converting zone:
		   if ($v_coll_zone ne $cv_srbZone) { $skipIt=1;}
		   if ($skipIt==0) {
                       push(@mkdirList, "mkdir '$v_coll_name' $v_coll_owner\n");
		   }
		}
	    }
	    if ($mode eq "LOCATION") {
		$v_location_name=$values[2];
		$v_location_address=$values[3];
		$v_location_address =~ s/:NULL.NULL//;
		push(@cv_srb_location_name, $v_location_name);
		push(@cv_srb_location_address, $v_location_address);
	    }
	    if ($mode eq "RESC") {
		$v_resc_name=$values[0];
		$v_resc_type=$values[1];
		$v_resc_path=$values[2];
		$v_resc_physical_name=$values[3];
		$resc_hostaddress=$values[5];
		$k=0;
		foreach $location_name (@cv_srb_location_name) {
		    if ($location_name eq $values[5]) {
			$resc_hostaddress = $cv_srb_location_address[$k];
		    }
		    $k++;
		}
#		$v_resc_location=$values[5];
		if ($v_resc_name eq $v_resc_physical_name &&
		    ($v_resc_type eq "unix file system" || $v_resc_type eq "UNIX_NOCHK file system") ) {
		    $k = index($v_resc_path, "/?");
		    $newPath = substr($v_resc_path, 0, $k);
		    if ($v_resc_name ne "sdsc-fs") { # skip special built-in
			push(@cv_srb_resources, $v_resc_name);
			$newName = "SRB-" . $v_resc_name;
			push(@cv_irods_resources, $newName);
			print( IADMIN_FILE "mkresc '$newName' 'unix file system' 'archive' '$resc_hostaddress' $newPath\n");
		    }
		}
	    }
	    if ($mode eq "METADATA_DATA") {
		$v_dataobj_name=$values[0];
		$v_dataobj_collection=convertCollection($values[1]);
		$TYPE=$values[2];
		for ($i=0;$i<10;$i++) {
		    $udsmd[$i]=$values[$i+3];
		}
		$v_dataobj_udimd0=$values[13];
		$v_dataobj_udimd1=$values[14];
		$didOne=0;
		for ($i=0;$i<10;$i++) {
		    if ($udsmd[$i] ne "") {
			print( IMETA_FILE "adda -d $v_dataobj_collection/$v_dataobj_name $TYPE-UDSMD$i '$udsmd[$i]'\n") ;
			$didOne=1;
		    }
		}
		if ($v_dataobj_udimd0 ne "") {
		    print ( IMETA_FILE "adda -d $v_dataobj_collection/$v_dataobj_name $TYPE-UDIMD0 '$v_dataobj_udimd0'\n");
		    $didOne=1;
		}
		if ($v_dataobj_udimd1 ne "") {
		    print ( IMETA_FILE "adda -d $v_dataobj_collection/$v_dataobj_name $TYPE-UDIMD1 '$v_dataobj_udimd1'\n");
		    $didOne=1;
		}
		if ($didOne) {
		    # give our admin user access for setting metadata
#		    addAccess($v_dataobj_collection, $v_dataobj_name);
# Changed to use the adda command instead of add; adda is privileged.
		}
	    }

	    if ($mode eq "METADATA_COLL") {
		$v_coll_name=convertCollection($values[0]);
		for ($i=0;$i<10;$i++) {
		    $udsmd_coll[$i]=$values[$i+2];
		}
		$v_coll_udimd0=$values[12];
		$v_coll_udimd1=$values[13];
		for ($i=0;$i<10;$i++) {
		    if ($udsmd_coll[$i] ne "") {
			print( IMETA_FILE "adda -c $v_coll_name UDSMD_COLL$i '$udsmd_coll[$i]'\n") ;
		    }
		}
		if ($v_coll_udimd0 ne "") {
		    print ( IMETA_FILE "adda -c $v_coll_name UDIMD_COLL0 '$v_coll_udimd0'\n");
		}
		if ($v_coll_udimd1 ne "") {
		    print ( IMETA_FILE "adda -c $v_coll_name UDIMD_COLL1 '$v_coll_udimd1'\n");
		}
	    }
	}
    }
    @mkdirSortList = sort @mkdirList;
    print(IADMIN_FILE @mkdirList);
    close( LOG_FILE );
}

# Geneate a random password string
sub randomPassword() {
    my @chars = split(" ",
      "a b c d e f g h i j k l m n o p q r s t u v w x y z 
       A B C D E F G H I J K L M N O P Q R S T U V W X Y Z 
       0 1 2 3 4 5 6 7 8 9");
    srand;
    my $password="";
    for (my $i=0; $i < $password_length ;$i++) {
	$_rand = int(rand 62);
	$j=$_rand;
#	printf("$i=$i j=$j\n");
	$password .= $chars[$j];
    }
    return $password;
}

# check to see if a collection (based on the name) should be converted
sub checkDoCollection($) {
    my ($inCollName) = @_;
#   printf("incollname = $inCollName \n");
    if (index($inCollName, "/container")==0) {
	return(0); # don't convert those starting with/container
    }
    if (index($inCollName, "/home/")==0) {
	return(1); # do convert /home/* collections 
    }
    $testColl = "/" . $cv_srbZone . "/container";
    if (index($inCollName, "$testColl")==0) {
	return(0); # don't convert these
    }
    $testColl = "/" . $cv_srbZone . "/trash";
    if (index($inCollName, "$testColl")==0) {
	return(0); # don't convert these
    }
#   $testColl = "/" . $cv_srbZone . "/home/";
#
# Sometimes these will match existing /zone/home/user directories and
# sometimes not, so go ahead and return them as OK (caller might reject
# specific ones).
#
#    if (index($inCollName, "$testColl")==0) {  # if it starts with /zone/home/ 
#	if (rindex($inCollName, "/") < length($testColl)) {
#                                         # and there are no additional subdirs
#	    return(0);                    # skip it
#	}
#   }
    $testColl = "/" . $cv_srbZone . "/";
    if (index($inCollName, "$testColl")==0) {  # if it starts with /zone/
	if (rindex($inCollName, "/") < length($testColl)) {
                                          # and there are no additional subdirs
	    return(0);                    # skip it
	}
    }

    $testColl = "/" . $cv_srbZone . "/";
    if (index($inCollName, "$testColl")!=0) {  # if it doesn't start w /zone/
	    return(0);                    # skip it
    }
    return(1);
}

# Add SQL to give access to this user if the previous item didn't already
sub addAccess($$) {
    my ($inColl, $inDataname) = @_;
    if ($inColl eq $metadataLastColl &&
	$inDataname eq $metadataLastDataname) {
	return;
    }
    print( SQL_FILE "insert into r_objt_access (object_id, user_id, access_type_id)  values ((select data_id from r_data_main where data_name = '$inDataname' and coll_id = (select coll_id from r_coll_main where coll_name = '$inColl')),(select user_id from r_user_main where user_name = '$thisUser'), '1200');\n");
    $metadataLastColl=$inColl;
    $metadataLastDataname=$inDataname;
}

# This function checks the position (index) of the fields in the log
# file and sets global indexes (g_*).  Sometimes the order of the
# fields in the Spullmeta log files differ, so this function is needed.
sub parseDataHdr {
    my($line) = @_;
    $_=$line;
    @DO_LIST=split('\|', $_);
    $ix=0;

    $g_data_name_ix=-1;
    $g_resc_name_ix=-1;
    $g_data_typ_name_ix=-1;
    $g_path_name_ix=-1;
    $g_size_ix=-1;
    $g_data_owner_ix=-1;
    $g_data_owner_domain_ix=-1;
    $g_data_create_timestamp_ix=-1;
    $g_data_last_access_timestamp_ix=-1;
    $g_container_repl_enum_ix=-1;
    $g_collection_cont_name_ix=-1;

    foreach $DO_ITEM (@DO_LIST) {
        if (index($DO_ITEM, "DATA_NAME")>=0) {
            $g_data_name_ix=$ix;
        }
        if (index($DO_ITEM, "RSRC_NAME")>=0) {
            $g_resc_name_ix=$ix;
        }
        if (index($DO_ITEM, "PATH_NAME")>=0) {
            $g_path_name_ix=$ix;
        }
        if (index($DO_ITEM, "DATA_TYP_NAME")>=0) {
            $g_data_typ_name_ix=$ix;
        }
        if (index($DO_ITEM, "SIZE")>=0) {
            $g_size_ix=$ix;
        }
        if (index($DO_ITEM, "DATA_OWNER")>=0) {
            if (index($DO_ITEM, "DATA_OWNER_") < 0) {
                $g_data_owner_ix=$ix;
            }
        }
        if (index($DO_ITEM, "DATA_OWNER_DOMAIN")>=0) {
            $g_data_owner_domain_ix=$ix;
        }
        if (index($DO_ITEM, "DATA_CREATE_TIMESTAMP")>=0) {
            $g_data_create_timestamp_ix=$ix;
        }
        if (index($DO_ITEM, "DATA_LAST_ACCESS_TIMESTAMP")>=0) {
            $g_data_last_access_timestamp_ix=$ix;
        }
        if (index($DO_ITEM, "CONTAINER_REPL_ENUM")>=0) {
            $g_container_repl_enum_ix=$ix;
        }
        if (index($DO_ITEM, "COLLECTION_CONT_NAME")>=0) {
            $g_collection_cont_name_ix=$ix;
        }
        $ix++;
    }
    if ($g_data_name_ix<0) {
        die("missing data_name field in Spullmeta log");
    }
    if ($g_resc_name_ix<0) {
        die("missing resc_name field in Spullmeta log");
    }
    if ($g_path_name_ix<0) {
        die("missing path_name field in Spullmeta log");
    }
    if ($g_data_typ_name_ix<0) {
        die("missing data_typ_name field in Spullmeta log");
    }
    if ($g_size_ix<0) {
        die("missing size field in Spullmeta log");
    }
    if ($g_data_owner_ix<0) {
        die("missing data_owner field in Spullmeta log");
    }
    if ($g_data_owner_domain_ix<0) {
        die("missing data_owner_domain field in Spullmeta log");
    }
    if ($g_data_create_timestamp_ix<0) {
        die("missing data_create_timestamp field in Spullmeta log");
    }
    if ($g_data_last_access_timestamp_ix<0) {
        die("missing data_last_access_timestamp field in Spullmeta log");
    }
    if ($g_container_repl_enum_ix<0) {
        die("missing container_repl_enum field in Spullmeta log");
    }
    if ($g_collection_cont_name_ix<0) {
        die("missing collection_cont_name field in Spullmeta log");
    }
    $debug_print=0;
    if ($debug_print) {
        print "g_data_name_ix $g_data_name_ix\n";
        print "g_resc_name_ix $g_resc_name_ix\n";
        print "g_path_name_ix $g_path_name_ix\n";
        print "g_data_typ_name_ix $g_data_typ_name_ix\n";
        print "g_size_ix $g_size_ix\n";
        print "g_data_owner_ix $g_data_owner_ix\n";
        print "g_data_owner_domain_ix $g_data_owner_domain_ix\n";
        print "g_data_create_timestamp_ix $g_data_create_timestamp_ix\n";
        print "g_data_last_access_timestamp_ix $g_data_last_access_timestamp_ix\n";
        print "g_container_repl_enum_ix $g_container_repl_enum_ix\n";
        print "g_collection_cont_name_ix $g_collection_cont_name_ix\n";
    }
}
