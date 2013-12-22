#!/usr/bin/perl
# Copyright UCSD and UNC.
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script that runs various icommands to test iCat
# ticket functions.
#
# It assumes the iinit has been run for the initial user, the two
# users (the initial one and user2) exist and the user2_Password is correct,
# and that the icommands are in the path.  Also, for strict-mode tests,
# which are run by default, you need to create the $CORE_RE_STRICT file
# with the strict rule set and also need to check the defines for your 
# setup ($CORE_RE_STRICT, $CORE_RE_ORIG, and $CORE_RE) and create those 
# files ($CORE_RE_STRICT with strict mode set).
# Also chck $Future_Date and $This_Host and $Other_Host.
# The user you run this from initially must be a rodsadmin, for example
# user 'rods'. User 'anonymous' must also exist.
#
# This is run by hand (i.e. is not part of 'irodsctl devtest').


use Net::Domain qw (hostname hostfqdn hostdomain);
use Net::Address::IP::Local;	# 4NT TEST


$G1="GROUP1";

#
# It will print "Success" at the end and have a 0 exit code, if
# everything works properly.
#
# Some of the icommands print errors but that's by design; those
# commands should fail and this script will fail if they don't.
#
# Current coverage:
# Basic means access fails without ticket, succeeds with it.
# 'use' checks that it updates and limit is enforced.
# 'write-bytes' and 'write-files' also check for updates and limits enforced.
#                    basic  expire  use   host  user group
#read file user        x      x      x     x     x    x
#read file anon        x      x      x     x     x    x
#read file strict      x      x      x     x     x    x
#read coll user        x      x      x     x     x    x
#read coll anon        x      x      x     x     x    x
#read coll strict      x      x      x     x     x    x
#                                                       write-bytes  write-files
#write file user       x      x      x     x     x    x      x          x
#write file anon       x      x      x     x     x    x      x          x
#write file strict     x      x      x     x     x    x      x          x
#write coll user       x      x      x     x     x    x      x          x
#write coll anon       x      x      x     x     x    x      x          x
#write coll strict     x      x      x     x     x    x      x          x


$tmpAuthFile="/tmp/testAuthFile.56789352";
$user2="user2";
$user2_Password="SUPERS3CR37";

$D1="testDir1";
$T1="ticket1";
$F1="TicketTestFile1";
$F2="TicketTestFile2";
$F3="TicketTestFile3";

$Future_Date="2040-12-12";	# Valid ticket
$Past_Date="1970-01-01";	# Expired ticket

#$This_Host=hostfqdn();
$This_Host=Net::Address::IP::Local->public;	# 4NT TEST

#$Other_Host="shiny.irods.renci.org";
$Other_Host="jargontest.irods.renci.org";

$G1="GROUP1";
$CORE_RE_STRICT="/var/lib/irods/iRODS/server/config/reConfigs/core.re.strict";
$CORE_RE_ORIG="/var/lib/irods/iRODS/server/config/reConfigs/core.re.orig";
$CORE_RE="/var/lib/irods/iRODS/server/config/reConfigs/core.re";

# run a command
# if option is 0 (normal), check the exit code and fail if non-0
# if 1, don't care (might succeed, might not)
# if 2, should get a non-zero result, exit if not
sub runCmd {
    my($option, $cmd, $stdoutVal) = @_;
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
    if ($stdoutVal != "") {
	if ($cmdStdout != $stdoutVal) {
	    print "stdout should have been: $stdoutVal\n";
	    print "but was: $cmdStdout";
	    die("incorrect stdout");
	}
    }
}

# set up iRODS test users
sub prepare {	
	printf("Creating test users...\n");
	runCmd(1, "iadmin mkuser anonymous rodsuser");
	runCmd(1, "iadmin mkuser $user2 rodsuser");
	runCmd(1, "iadmin moduser $user2 password $user2_Password");
}


# remove test users
sub teardown {
	printf("Removing test users...\n");
	runCmd(1, "iadmin rmgroup $G1");
	runCmd(1, "iadmin rmuser anonymous");
	runCmd(0, "iadmin rmuser $user2");
}

# switch to being the user user2.
sub becomeuser2 {
    $ENV{'irodsUserName'}=$user2; 
    $ENV{'irodsAuthFileName'}=$tmpAuthFile;
    if ($iinited == 0) {
	runCmd(0, "iinit $user2_Password");
	$iinited = 1;
    }
    printf("Became user2\n");
}

# switch to being user anonymous.
sub becomeAnon {
    $ENV{'irodsUserName'}='anonymous';
    delete $ENV{'irodsAuthFileName'};
    printf("Became Anonymous\n");
}

# Undo becomeuser2 to become the initial user again
sub becomeSelfAgain {
    delete $ENV{'irodsAuthFileName'};
    delete $ENV{'irodsUserName'};
    printf("Became self again.\n");
}

sub testGetBasicFail() {
    unlink($F1);
    becomeuser2();
    runCmd(2,"iget $D1/$F1");
    runCmd(2, "iticket delete $T1");
    becomeAnon();
    unlink($F1);
    runCmd(2,"iget $D1/$F1");
    runCmd(2, "iticket delete $T1");
    becomeSelfAgain();
}

sub testGet() {
    unlink($F1);
    becomeuser2();
    runCmd(0,"iget -t $T1 $D1/$F1");
    becomeAnon();
    unlink($F1);
    runCmd(0,"iget -t $T1 $D1/$F1");
    becomeSelfAgain();
}

sub testPut() {
    `ls -l >> $F1`;
    becomeuser2();
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1");
    becomeSelfAgain();
}

sub testGetShouldFail() {
    unlink($F1);
    becomeuser2();
    runCmd(2,"iget -t $T1 $D1/$F1");
    becomeAnon();
    unlink($F1);
    runCmd(2,"iget -t $T1 $D1/$F1");
    becomeSelfAgain();
}

sub testPutShouldFail() {
    `ls -l >> $F1`;
    becomeuser2();
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput -t $T1 $F1 $D1/$F1");
    becomeSelfAgain();
}

sub testGetAndUses() {   # CHECK PB WITH 1 USE
    unlink($F1);
#    runCmd(0, "iticket mod $T1 uses 1");
#    becomeuser2();
#    runCmd(2,"iget $D1/$F1");
#    runCmd(0,"iget -t $T1 $D1/$F1");
#    unlink($F1);
#    runCmd(2,"iget -t $T1 $D1/$F1");
#    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 uses 2");
    becomeAnon();
    unlink($F1);
    runCmd(2,"iget $D1/$F1");
    runCmd(0,"iget -t $T1 $D1/$F1");
    unlink($F1);
    runCmd(2,"iget -t $T1 $D1/$F1");
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 uses 100");
}

sub testPutAndUses() {
    `ls -l >> $F1`;
    runCmd(0, "iticket mod $T1 uses 3");
    becomeuser2();
    runCmd(2,"iput -f $F1 $D1/$F1");
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 1
    `ls -l >> $F1`;
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 2
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1"); # 3 should fail

    becomeSelfAgain();
#    runCmd(0, "iticket mod $T1 uses 5");
	runCmd(0, "iticket mod $T1 uses 6");	# did that previous failed access count???
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput $F1 $D1/$F1");
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 1
    `ls -l >> $F1`;
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 2
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1"); # 3 should fail
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 uses 100");
}

# test puts with the write-file limits
sub testPutAndWriteFile() {
    `ls -l >> $F1`;
    runCmd(0, "iticket mod $T1 write-file 2");
    becomeuser2();
    runCmd(2,"iput -f $F1 $D1/$F1");
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 1
    `ls -l >> $F1`;
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 2
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1"); # 3 should fail
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 write-file 4");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput $F1 $D1/$F1");
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 1
    `ls -l >> $F1`;
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1"); # 2
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1"); # 3 should fail
    becomeSelfAgain();
}

# test puts with the write-bytes limits
sub testPutAndWriteBytes() {
    `ls -l >> $F1`;
    runCmd(0, "iticket mod $T1 write-byte 2500");
    runCmd(0, "cp $F1 $F2");
    runCmd(0, "truncate -s 1000 $F2"); # make a 1000-byte file
    runCmd(0, "cp $F1 $F3");
    runCmd(0, "truncate -s 1020 $F3"); # make a somewhat larger one (same
                                       # size file isn't counted).
    becomeuser2();
    runCmd(2,"iput -f $F2 $D1/$F1");
    runCmd(0,"iput -ft $T1 $F2 $D1/$F1"); # 1
    `ls -l >> $F1`;
    runCmd(0,"iput -ft $T1 $F3 $D1/$F1"); # 2
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F2 $D1/$F1"); # 3 should fail
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 write-byte 4500");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput $F2 $D1/$F1");
    runCmd(0,"iput -ft $T1 $F2 $D1/$F1"); # 1
    `ls -l >> $F1`;
    runCmd(0,"iput -ft $T1 $F3 $D1/$F1"); # 2
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F2 $D1/$F1"); # 3 should fail
    becomeSelfAgain();
}

sub setStrict() {
    printf("Setting to strict mode\n");
    if ( -f $CORE_RE_STRICT ) {
	if ( ! -f $CORE_RE_ORIG) {
	    rename($CORE_RE, $CORE_RE_ORIG);
	}
	runCmd(0, "cp -f $CORE_RE_STRICT $CORE_RE");
    }
    else {
	printf("Warning, cannot set strict, no $CORE_RE_STRICT file\n");
    }
    runCmd(1, "imiscsvrinfo");  # slight delay, may segfault cp'ing new core.re
}

sub unSetStrict() {
    printf("Setting to regular unstrict mode\n");
    if ( -f $CORE_RE_ORIG ) {
	runCmd(0, "cp -f $CORE_RE_ORIG $CORE_RE");
    }
    else {
	printf("Warning, cannot set strict, no $CORE_RE_ORIG file\n");
    }
    runCmd(1, "imiscsvrinfo");  # slight delay, may segfault cp'ing new core.re
}

sub doWriteTests() {
    runCmd(1, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create write $D1 $T1");
    }
    else {
	runCmd(0, "iticket create write $D1/$F1 $T1");
    }
    runCmd(0, "iticket mod $T1 write-file 100"); # default is just 10 so adjust

# Basic test
    testPut();

    #setStrict();
    testPut();
    #unSetStrict();

# Group test
    runCmd(1, "iadmin mkgroup $G1");
    runCmd(1, "iadmin rfg $G1 $user2");
    runCmd(0, "iadmin atg $G1 $user2");
    runCmd(0, "iticket mod $T1 add group $G1");
    `ls -l >> $F1`;
    becomeuser2();
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1");
    
    #setStrict();
    `ls -l >> $F1`;
    becomeuser2();
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1");
    #unSetStrict();
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 remove group $G1");

# User test
    runCmd(0, "iticket mod $T1 add user $user2");
    `ls -l >> $F1`;
    becomeuser2();
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1");

    #setStrict();
    `ls -l >> $F1`;
    becomeuser2();
    runCmd(0,"iput -ft $T1 $F1 $D1/$F1");
    becomeAnon();
    `ls -l >> $F1`;
    runCmd(2,"iput -ft $T1 $F1 $D1/$F1");
    #unSetStrict();
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 remove user $user2");

# Host test
    runCmd(0, "iticket mod $T1 add host $Other_Host");
    testPutShouldFail();
    runCmd(0, "iticket mod $T1 add host $This_Host");
    testPut();
    runCmd(0, "iticket mod $T1 remove host $Other_Host");
    testPut();
    runCmd(0, "iticket mod $T1 add host $Other_Host");

    #setStrict();
    runCmd(0, "iticket mod $T1 expire $Past_Date");
    testPutShouldFail();
    runCmd(0, "iticket mod $T1 expire $Future_Date");
    testPut();
    runCmd(0, "iticket mod $T1 expire 0");
    #unSetStrict();

# Expire test
    runCmd(0, "iticket mod $T1 expire $Past_Date");
    testPutShouldFail();
    runCmd(0, "iticket mod $T1 expire $Future_Date");
    testPut();
    runCmd(0, "iticket mod $T1 expire 0");
    #setStrict();
    runCmd(0, "iticket mod $T1 expire $Past_Date");
    testPutShouldFail();
    runCmd(0, "iticket mod $T1 expire $Future_Date");
    testPut();
    runCmd(0, "iticket mod $T1 expire 0");
    #unSetStrict();

# use count tests
    testPutAndUses();
    runCmd(0, "iticket ls $T1");
    printf("stdout:$cmdStdout");

    #setStrict();
    runCmd(0, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create write $D1 $T1");
    }
    else {
	runCmd(0, "iticket create write $D1/$F1 $T1");
    }
    runCmd(0, "iticket mod $T1 write-file 100"); # default is just 10 so adjust

    testPutAndUses();
    #unSetStrict();

# write-file tests
    runCmd(0, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create write $D1 $T1");
    }
    else {
	runCmd(0, "iticket create write $D1/$F1 $T1");
    }

    testPutAndWriteFile();
    runCmd(0, "iticket ls $T1");
    printf("stdout:$cmdStdout");

    #setStrict();
    runCmd(0, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create write $D1 $T1");
    }
    else {
	runCmd(0, "iticket create write $D1/$F1 $T1");
    }

    testPutAndWriteFile();
    #unSetStrict();

# write-byte tests
    runCmd(0, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create write $D1 $T1");
    }
    else {
	runCmd(0, "iticket create write $D1/$F1 $T1");
    }
    runCmd(0, "iticket mod $T1 write-file 100"); # default is just 10 so adjust

    testPutAndWriteBytes();
    runCmd(0, "iticket ls $T1");
    printf("stdout:$cmdStdout");

    #setStrict();
    runCmd(0, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create write $D1 $T1");
    }
    else {
	runCmd(0, "iticket create write $D1/$F1 $T1");
    }

    testPutAndWriteBytes();
    #unSetStrict();

} # end for doWriteTests

sub doReadTests() {
    runCmd(1, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create read $D1 $T1");
    }
    else {
	runCmd(0, "iticket create read $D1/$F1 $T1");
    }

# Basic test
    testGet();

    #setStrict();
    testGet();
    #unSetStrict();


# Group test
    runCmd(1, "iadmin mkgroup $G1");
    runCmd(1, "iadmin rfg $G1 $user2");
    runCmd(0, "iadmin atg $G1 $user2");
    runCmd(0, "iticket mod $T1 add group $G1");
    unlink($F1);
    becomeuser2();
    runCmd(0,"iget -t $T1 $D1/$F1");
    becomeAnon();
    unlink($F1);
    runCmd(2,"iget -t $T1 $D1/$F1");
    
    #setStrict();
    unlink($F1);
    becomeuser2();
    runCmd(0,"iget -t $T1 $D1/$F1");
    becomeAnon();
    unlink($F1);
    runCmd(2,"iget -t $T1 $D1/$F1");
    #unSetStrict();
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 remove group $G1");

# User test
    runCmd(0, "iticket mod $T1 add user $user2");
    unlink($F1);
    becomeuser2();
    runCmd(0,"iget -t $T1 $D1/$F1");
    becomeAnon();
    unlink($F1);
    runCmd(2,"iget -t $T1 $D1/$F1");

    #setStrict();
    unlink($F1);
    becomeuser2();
    runCmd(0,"iget -t $T1 $D1/$F1");
    becomeAnon();
    unlink($F1);
    runCmd(2,"iget -t $T1 $D1/$F1");
    #unSetStrict();
    becomeSelfAgain();
    runCmd(0, "iticket mod $T1 remove user $user2");

# Host test
    runCmd(0, "iticket mod $T1 add host $Other_Host");
    testGetShouldFail();
    runCmd(0, "iticket mod $T1 add host $This_Host");
    testGet();
    runCmd(0, "iticket mod $T1 remove host $Other_Host");
    testGet();
    runCmd(0, "iticket mod $T1 add host $Other_Host");

    #setStrict();
    runCmd(0, "iticket mod $T1 expire $Past_Date");
    testGetShouldFail();
    runCmd(0, "iticket mod $T1 expire $Future_Date");
    testGet();
    runCmd(0, "iticket mod $T1 expire 0");
    #unSetStrict();

# Expire test
    runCmd(0, "iticket mod $T1 expire $Past_Date");
    testGetShouldFail();
    runCmd(0, "iticket mod $T1 expire $Future_Date");
    testGet();
    runCmd(0, "iticket mod $T1 expire 0");
    #setStrict();
    runCmd(0, "iticket mod $T1 expire $Past_Date");
    testGetShouldFail();
    runCmd(0, "iticket mod $T1 expire $Future_Date");
    testGet();
    runCmd(0, "iticket mod $T1 expire 0");
    #unSetStrict();

# use count tests
	printf("starting count tests\n");
    testGetAndUses();
    runCmd(0, "iticket ls $T1");
    printf("stdout:$cmdStdout");

    #setStrict();
    runCmd(0, "iticket delete $T1");
    if ($doCollectionTicket==1) {
	runCmd(0, "iticket create read $D1 $T1");
    }
    else {
	runCmd(0, "iticket create read $D1/$F1 $T1");
    }
    testGetAndUses();
    #unSetStrict();
}  # end of doReadTests


printf("FQDN: $This_Host\n");
# Create test accounts
prepare();

#unlink($tmpPwFile);
unlink($tmpAuthFile);
$iinited=0;
$doEnvTest=1;
if ($doEnvTest==1) {
    becomeuser2();
    runCmd(0, "ienv");
    printf("stdout:$cmdStdout");
    becomeSelfAgain();
    runCmd(0, "ienv");
    printf("stdout:$cmdStdout");
    becomeAnon();
    runCmd(0, "ienv");
    printf("stdout:$cmdStdout");
    becomeSelfAgain();
}

`ls -l > $F1`;
runCmd(1, "irm -rf $D1");
runCmd(0, "imkdir $D1");
runCmd(0, "iput $F1 $D1/$F1");
runCmd(0, "ils -l $D1");
printf("stdout:$cmdStdout");
becomeAnon();
runCmd(2, "ils -l $D1");
printf("stdout:$cmdStdout");
becomeSelfAgain();
runCmd(1, "iticket delete $T1");

# Test that the user-set works and gets fail without the ticket
testGetBasicFail();

# You can comment out any of the do*Tests() calls below to run a briefer set
$doCollectionTicket=0;
doReadTests();
$doCollectionTicket=1;
doReadTests();

$doCollectionTicket=0;
doWriteTests();
$doCollectionTicket=1;
doWriteTests();

# done
runCmd(0, "iticket delete $T1");
runCmd(0, "irm -rf $D1");
unlink($tmpAuthFile);

# remove test accounts
teardown();

printf("Success\n");
