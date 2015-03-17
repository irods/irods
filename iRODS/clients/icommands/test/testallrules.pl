#!/usr/bin/perl -w
#
# script to test the basic functionnalities of icommands.
# icommands location has to be put in the PATH env variable or their PATH will be asked
# at the beginning of the execution of this script.
#
# usage: ./testiCommands.pl
# If an argument is given, the debug mode is activated
#
# Copyright (c), CCIN2P3
# For more information please refer to files in the COPYRIGHT directory.

use strict;
use Cwd;
use Sys::Hostname;
use File::stat;
use JSON;

#-- Initialization

my $debug;
my $entry;
my @failureList;
my $i;
my $input;
my $irodsdefresource;
my $irodshost;
my $irodshome;
my $irodszone;
my $line;
my @list;
my $misc;
my $nfailure;
my $nsuccess;
my $rc;
my @returnref;
my @successlist;
my @summarylist;
my @tmp_tab;
my $username;
my @words;

my $unixuser     = `whoami`;
chomp $unixuser;
my $dir_w        = cwd();
my $myssize;
my $host         = hostname();
if ( $host =~ '.' ) {
    @words = split( /\./, $host );
    $host  = $words[0];
}
my $irodsfile;
my $irodsEnvFile = $ENV{'IRODS_ENVIRONMENT_FILE'};
if ($irodsEnvFile) {
    $irodsfile = $irodsEnvFile;
} else {
    $irodsfile    = "$ENV{HOME}/.irods/irods_environment.json";
}
my $ntests       = 0;
my $progname     = $0;

my $outputfile   = "testallrules_" . $host . ".log";

#-- Find current working directory and make consistency path

$outputfile   = $dir_w . '/' . $outputfile;

if ( $progname !~ '/' ) {
    $progname = $dir_w . '/' . $progname;
} else {
    if ( substr( $progname, 0, 2 ) eq './' ) {
        @words    = split( /\//, $progname );
        $i        = $#words;
        $progname = $dir_w . '/' . $words[$i];
    }
    if ( substr( $progname, 0, 1 ) ne '/' ) {
        $progname = $dir_w . '/' . $progname;
    }
}

#-- Take debug level

$debug = shift;
if ( ! $debug ) {
    $debug = 0;
} else {
    $debug = 1;
}

#-- Print debug

if ( $debug ) {
    print( "\n" );
    print( "MAIN: irodsfile        = $irodsfile\n" );
    print( "MAIN: cwd              = $dir_w\n" );
    print( "MAIN: outputfile       = $outputfile\n" );
    print( "MAIN: progname         = $progname\n" );
    print( "\n" );
}
# read and parse the irods json environment file
# and extract useful values
my $json_text = do {
   open(my $json_fh, "<:encoding(UTF-8)", $irodsfile)
	 or die("Can't open \$filename\": $!\n");
	    local $/;
	       <$json_fh>
};

my $json = JSON->new;
my $data = $json->decode($json_text);

$username   = $data->{ "irods_user_name" };
$irodshome = $data->{ "irods_home" };
$irodszone = $data->{ "irods_zone" };
$irodshost = $data->{ "irods_host" };
$irodsdefresource = $data->{ "irods_default_resource" };



#-- Dump content of $irodsfile to @list

#@list = dumpFileContent( $irodsfile );

#-- Loop on content of @list
# The below parsing works in the current environment 
# but there are two shortcomings:
#   1) single quotes are removed, but if there were to be embedded ones,
#      they would be removed too.
#   2) if the name and value are separated by =, the line will not split right.
#foreach $line ( @list ) {
#    chomp( $line );
#    if ( ! $line ) { next; }
#    if ( $line =~ /irodsUserName/ ) {
#        ( $misc, $username ) = split( / /, $line );
#        $username =~ s/\'//g; #remove all ' chars, if any
#        next;
#    }
#    if ( $line =~ /irodsHome/ ) {
#        ( $misc, $irodshome ) = split( / /, $line );
#        $irodshome =~ s/\'//g; #remove all ' chars, if any
#        next;
#    }
#    if ( $line =~ /irodsZone/ ) {
#        ( $misc, $irodszone ) = split( / /, $line );
#        $irodszone =~ s/\'//g; #remove all ' chars, if any
#        next;
#    }
#    if ( $line =~ /irodsHost/ ) {
#        ( $misc, $irodshost ) = split( / /, $line );
#        $irodshost =~ s/\'//g; #remove all ' chars, if any
#        next;
#    }
#    if ( $line =~ /irodsDefResource/ ) {
#        ( $misc, $irodsdefresource ) = split( / /, $line );
#        $irodsdefresource =~ s/\'//g; #remove all ' chars, if any
#    }
#}

#-- Print debug

if ( $debug ) {
    print( "MAIN: username         = $username\n" );
    print( "MAIN: irodshome        = $irodshome\n" );
    print( "MAIN: irodszone        = $irodszone\n" );
    print( "MAIN: irodshost        = $irodshost\n" );
    print( "MAIN: irodsdefresource = $irodsdefresource\n" );
}

#-- Environment setup and print to stdout

print( "\nThe results of the test will be written in the file: $outputfile\n\n" );
print( "Warning: you need to be a rodsadmin in order to pass successfully all the tests," );
print( " as some admin commands are being tested." );
print( " If icommands location has not been set into the PATH env variable," );
print( " please give it now, else press return to proceed.\n" );
print( "icommands location path: " );
chomp( $input = <STDIN> );
if ( $input ) { $ENV{'PATH'} .= ":$input"; }
print "Please, enter the password of the iRODS user used for the test: ";
chomp( $input = <STDIN> );
if ( ! $input ) {
    print( "\nYou should give valid pwd.\n\n");
    exit;
} else {
    print( "\n" );
}



#-- Test the icommands and eventually compared the result to what is expected.

runCmd( "iinit $input" );

# prep and cleanup
runCmd( "iadmin mkuser devtestuser rodsuser","","","","iadmin rmuser devtestuser" );
runCmd( "iadmin mkresc testResc 'unix file system' $irodshost:/tmp/$unixuser/testResc I_AM_A_CONTEXT_STRING", "", "", "", "iadmin rmresc testResc" );
runCmd( "iadmin mkresc testallrulesResc 'unix file system' $irodshost:/tmp/$unixuser/testallrulesResc I_AM_ALSO_A_CONTEXT_STRING", "", "", "", "iadmin rmresc testallrulesResc" );
runCmd( "iadmin atrg testallrulesgroup testResc", "", "", "", "iadmin rfrg testallrulesgroup testResc");
runCmd( "imkdir sub1", "", "", "", "irm -rf sub1" );
runCmd( "imkdir forphymv", "", "", "", "irm -rf forphymv" );
runCmd( "imkdir ruletest", "", "", "", "irm -rf ruletest" );
runCmd( "imkdir test", "", "", "", "irm -rf test" );
runCmd( "imkdir test/phypathreg" );
runCmd( "imkdir ruletest/subforrmcoll" );
runCmd( "iput $progname test/foo1");
runCmd( "icp test/foo1 sub1/mdcopysource");
runCmd( "icp test/foo1 sub1/mdcopydest");
runCmd( "icp test/foo1 sub1/dcmetadatatarget");
runCmd( "icp test/foo1 sub1/foo1");
runCmd( "icp test/foo1 sub1/foo2");
runCmd( "icp test/foo1 sub1/foo3");
runCmd( "icp test/foo1 forphymv/phymvfile");
runCmd( "icp test/foo1 sub1/objunlink1");
runCmd( "irm sub1/objunlink1"); # put it in the trash
runCmd( "icp test/foo1 sub1/objunlink2");
runCmd( "irepl -RtestResc sub1/objunlink2");
runCmd( "icp test/foo1 sub1/freebuffer");
runCmd( "icp test/foo1 sub1/automove");
runCmd( "icp test/foo1 test/versiontest.txt");
runCmd( "icp test/foo1 test/metadata-target.txt");
runCmd( "icp test/foo1 test/ERAtestfile.txt");
runCmd( "ichmod read devtestuser test/ERAtestfile.txt");
runCmd( "imeta add -d test/ERAtestfile.txt Fun 99 Balloons");
runCmd( "imkdir sub1/SaveVersions" );
runCmd( "iput $dir_w/misc/devtestuser-account-ACL.txt test");
runCmd( "iput $dir_w/misc/load-metadata.txt test");
runCmd( "iput $dir_w/misc/load-usermods.txt test");
runCmd( "iput $dir_w/misc/sample.email test");
runCmd( "iput $dir_w/misc/email.tag test");
runCmd( "iput $dir_w/misc/sample.email test/sample2.email");
runCmd( "iput $dir_w/misc/email.tag test/email2.tag");

runCmd( "ls > foo1" );

# get listing of example rules in ./rules3.0
my @rules;
my $rulefile;
my $ssb=0;
@rules = <rules3.0/*>;

# loop through all
foreach $rulefile (@rules) 
{

    #if ($rulefile =~ /rulemsiDataObjTrim/) { print "---- splitting the deck -- $rulefile\n"; last; }
    #if ($rulefile =~ /rulemsiGetDataObjPSmeta/) { print "----- Ending for debugging -- $rulefile\n"; last; }

    # skipping for now, not sure why it's throwing a stacktrace at the moment
    if ($rulefile =~ /rulemsiPropertiesToString/) { print "----- skipping b/c of stacktrace -- $rulefile\n"; next; }

    # skip rules that touch our core re
    if ($rulefile =~ /rulemsiAdmAppendToTopOfCoreIRB/) { print "----- skipping RE -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiAdmAppendToTopOfCoreRE/) { print "----- skipping RE -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiAdmChangeCoreIRB/) { print "----- skipping RE -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiAdmChangeCoreRE/) { print "----- skipping RE -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiGetRulesFromDBIntoStruct/) { print "----- skipping RE -- $rulefile\n"; next; }

    # skip rules that fail by design
    if ($rulefile =~ /GoodFailure/) { print "----- skipping failbydesign -- $rulefile\n"; next; }

    # skip if an action (run in the core.re), not enough input/output for irule
    if ($rulefile =~ /rulemsiAclPolicy/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiAddUserToGroup/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiCheckHostAccessControl/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiCheckOwner/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiCheckPermission/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiCommit/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiCreateCollByAdmin/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiCreateUser/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiDeleteCollByAdmin/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiDeleteDisallowed/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiDeleteUser/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiExtractNaraMetadata/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiOprDisallowed/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiRegisterData/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiRenameCollection/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiRenameLocalZone/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiRollback/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetBulkPutPostProcPolicy/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetDataObjAvoidResc/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetDataObjPreferredResc/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetDataTypeFromExt/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetDefaultResc/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetGraftPathScheme/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetMultiReplPerResc/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetNoDirectRescInp/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetNumThreads/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetPublicUserOpr/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetRandomScheme/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetRescQuotaPolicy/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetRescSortScheme/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetReServerNumProc/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSetResource/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSortDataObj/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiStageDataObj/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSysChksumDataObj/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSysMetaModify/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSysReplDataObj/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiNoChkFilePathPerm/) { print "----- skipping input/output -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiNoTrashCan/) { print "----- skipping input/output -- $rulefile\n"; next; }

    # skip rules we are not yet supporting

    if ($rulefile =~ /rulemsiobj/) { print "----- skipping msiobj -- $rulefile\n"; next; }


    if ($rulefile =~ /rulemsiFlagInfectedObjs.r/) { print "----- skipping ERA -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiGetAuditTrailInfoByActionID.r/) { print "----- skipping ERA -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiGetAuditTrailInfoByKeywords.r/) { print "----- skipping ERA -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiGetAuditTrailInfoByObjectID.r/) { print "----- skipping ERA -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiGetAuditTrailInfoByTimeStamp.r/) { print "----- skipping ERA -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiGetAuditTrailInfoByUserID.r/) { print "----- skipping ERA -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiMergeDataCopies.r/) { print "----- skipping ERA -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiGetFormattedSystemTime/) { print "----- skipping guinot -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiFtpGet.r/) { print "----- skipping FTP -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiTwitterPost.r/) { print "----- skipping FTP -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiConvertCurrency.r/) { print "----- skipping webservices -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiGetQuote.r/) { print "----- skipping webservices -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiIp2location.r/) { print "----- skipping webservices -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiObjByName.r/) { print "----- skipping webservices -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSdssImgCutout_GetJpeg.r/) { print "----- skipping webservices -- $rulefile\n"; next; }

    if ($rulefile =~ /ruleintegrity/) { print "----- skipping integrityChecks -- $rulefile\n"; next; }

    if ($rulefile =~ /z3950/) { print "----- skipping Z3950 -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiImage/) { print "----- skipping image -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiLoadMetadataFromXml/) { print "----- skipping XML -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiXmlDocSchemaValidate/) { print "----- skipping XML -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiXsltApply/) { print "----- skipping XML -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiCreateXmsgInp/) { print "----- skipping XMsg -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiRcvXmsg/) { print "----- skipping XMsg -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiSendXmsg/) { print "----- skipping XMsg -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiXmsgCreateStream/) { print "----- skipping XMsg -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiXmsgServerConnect/) { print "----- skipping XMsg -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiXmsgServerDisConnect/) { print "----- skipping XMsg -- $rulefile\n"; next; }
    if ($rulefile =~ /rulereadXMsg/) { print "----- skipping XMsg -- $rulefile\n"; next; }
    if ($rulefile =~ /rulewriteXMsg/) { print "----- skipping XMsg -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiRda/) { print "----- skipping RDA -- $rulefile\n"; next; }

    if ($rulefile =~ /rulemsiCollRepl/) { print "----- skipping deprecated -- $rulefile\n"; next; }
    if ($rulefile =~ /rulemsiDataObjGetWithOptions/) { print "----- skipping deprecated -- $rulefile\n"; next; }


    # run it
    runCmd( "irule -F $rulefile" );
}

#runCmd( "irule -F rules3.0/rulegenerateBagIt.r" );



# cleanup
if ( 1 ) {
    if ($ssb != 1 ) { runCmd( "irm -rf system_backups" ); }
    runCmd( "ichmod own rods ruletest/automove" );
    runCmd( "ichmod own rods sub1/foo3" );
    runCmd( "imcoll -U /tempZone/home/rods/test/phypathreg" );
    runCmd( "irm -rf bagit" );
    runCmd( "irm -f bagit.tar" );
    runCmd( "irm -f rules" );
    runCmd( "irm -rf sub2" );
    runCmd( "iadmin modresc testResc status up" );
    runCmd( "irm -rf /tempZone/bundle/home/rods" );
    runCmd( "rm foo1" );








#-- Execute rollback commands

    if ( $debug ) { print( "\nMAIN ########### Roll back ################\n" ); }
    for ( $i = $#returnref; $i >= 0; $i-- ) {
        undef( @tmp_tab );
        $line     = $returnref[$i];
        @tmp_tab = @{$line};    
        runCmd( $tmp_tab[0], $tmp_tab[1], $tmp_tab[2], $tmp_tab[3] );
    }

#-- Execute last commands before leaving

    if ( $debug ) { print( "\nMAIN ########### Last ################\n" ); }

    runCmd( "iadmin lg", "negtest", "LIST", "testallrulesgroup" );
    runCmd( "iadmin lg", "negtest", "LIST", "resgroup" );
    runCmd( "iadmin lr", "negtest", "LIST", "testresource" );
    runCmd( "irmtrash" );
    runCmd( "iexit full" );
    `/bin/rm -rf /tmp/foo`;# remove the vault for the testresource; needed in case
    # another unix login runs this test on this host
}

#-- print the result of the test into testSurvey.log

$nsuccess = @successlist;
$nfailure = @failureList;

open( FILE, "> $outputfile" ) or die "unable to open $outputfile for writing\n";
print( FILE "============================================\n" );
print( FILE "number of successfully tested commands = $nsuccess\n" );
print( FILE "number of failed tested commands       = $nfailure\n" );
print( FILE "============================================\n\n" );
print( FILE "Summary of the consecutive commands which have been tested:\n\n" );

$i = 1;
foreach $line ( @summarylist ) {
    print( FILE "$i - $line\n" );
    $i++;
}
print( FILE "\n\nList of successful tests:\n\n" );
foreach $line ( @successlist ) { print( FILE "$line" ); }
print( FILE "\n\nList of failed tests:\n\n" );
foreach $line ( @failureList ) { print( FILE "$line" ); }
print( FILE "\n" );
close( FILE );
exit;

##########################################################################################################################
# runCmd needs at least 8 arguments: 
#   1- command name + arguments
#   2- specify if it is a negative test by providing the "negtest" value, ie it is successful if the test fails (optional).
#   3- output line of interest (optional), if equal to "LIST" then match test the entire list of answers provided in 4-.
#   4- expected list of results separeted by ',' (optional: must be given if second argument provided else it will fail).
#       5- command name to go back to first situation
#       6- same as 2 but for 5
#       7- same as 3 but for 5
#       8- same as 4 but for 5

sub runCmd {
    my ( $cmd, $testtype, $stringToCheck, $expResult, $r_cmd, $r_testtype, $r_stringToCheck, $r_expResult ) = @_;

    my $rc = 0;
    my @returnList;
    my @words;
    my $line;

    my $answer     = "";
    my @list       = "";
    my $numinlist  = 0;
    my $numsuccess = 0;
    my $tempFile   = "/tmp/iCommand.log";
    my $negtest    = 0;
    my $result     = 1;             # used only in the case where the answer of the command has to be compared to an expected answer.

#-- Check inputs

    if ( ! $cmd ) {
        print( "No command given to runCmd; Exit\n" );
        exit;
    }
    if ( ! $testtype ) {
        $testtype = 0;
        $negtest  = 0;
    } else {
        if ( $testtype eq "negtest" ) {
            $negtest = 1;
        } else {
            $negtest = 0;
        }       
    }
    if ( ! $stringToCheck   ) { $stringToCheck = ""; }
    if ( ! $expResult       ) { $expResult = ""; }
    if ( ! $r_cmd           ) { $r_cmd = ""; }
    if ( ! $r_testtype      ) { $r_testtype = ""; }
    if ( ! $r_stringToCheck ) { $r_stringToCheck = ""; }
    if ( ! $r_expResult     ) { $r_expResult = ""; }

#-- Update counter

    $ntests++;

#-- Print debug
    
    if ( $debug ) { print( "\n" ); }
    printf( "%3d - cmd executed: $cmd\n", $ntests );
    use File::Basename;
    my $thescriptname = basename($0);
    chomp(my $therodslog = `ls -t /var/lib/irods/iRODS/server/log/rodsLog* | head -n1`);
    open THERODSLOG, ">>$therodslog" or die "could not open [$therodslog]";
    print THERODSLOG " --- $thescriptname $ntests [$cmd] --- \n";
    close THERODSLOG;

    if ( $debug ) { print( "DEBUG: input to runCMd: $cmd, $testtype, $stringToCheck, $expResult.\n" ); }

#-- Push return command in list

    undef( @returnList );
    if ( $r_cmd ){
        $returnList[0] = $r_cmd;
        $returnList[1] = $r_testtype;
        $returnList[2] = $r_stringToCheck;
        $returnList[3] = $r_expResult;
        push( @returnref, \@returnList );
        
        if ( $debug ) { print( "DEBUG: roll back:       $returnList[0], $returnList[1], $returnList[2], $returnList[3].\n" ); }
    } else {
        if ( $debug ) { print( "DEBUG: roll back:       no.\n" ); }             
    }
    
#-- Push icommand in @summarylist

    push( @summarylist, "$cmd" );

#-- Execute icommand

    $rc = system( "$cmd > $tempFile" );

#-- check that the list of answers is part of the result of the command.

    if (( $rc == 0 ) and $stringToCheck ) {
        @words     = split( ",", $expResult );
        $numinlist = @words;
        @list      = dumpFileContent( $tempFile );
        
        if ( $debug ) {
            print( "DEBUG: numinlist = $numinlist\n" );
            print( "DEBUG: list =\n@list\n" );
        }
        
#---- If LIST is given as 3rd element: compare output of icommand to list in 4th argument
        
        if ( $stringToCheck eq "LIST" ) {
            foreach $line ( @list ) {
                chomp( $line );
                $line =~ s/^\s+//;
                $line =~ s/\s+$//;
                $answer .= "$line ";
            }
            chomp( $answer );
            $answer =~ s/^\s+//;
            $answer =~ s/\s+$//;

            if ( $debug ) { print( "DEBUG: answer    = $answer\n" ); }

            foreach $entry ( @words ) {
                if ( $answer =~ /$entry/ ) { $numsuccess++; }
            }
            
            if ( $numsuccess == $numinlist ) {
                $result = 1;
            } else {
                $result = 0;
            }
        } else {
            if ( $debug ) { print( "DEBUG: stringToCheck = $stringToCheck\n" ); }
            foreach $line ( @list ) {
                chomp( $line );
                if ( $debug ) { print( "DEBUG: line = $line\n" ); }
                if ( $line =~ /$stringToCheck/ ) {
                    ( $misc, $answer ) = split( /$stringToCheck/, $line );
                    $answer =~ s/^\s+//;            # remove blanks
                    $answer =~ s/\s+$//;            # remove blanks
                    last;
                }
            }
            
            if ( $answer eq $words[0] ) {
                $result = 1;
            } else {
                $result = 0;
            }
        }
    }
    
    if ( $rc == 0 and ( $result ^ $negtest ) ) {
        push( @successlist, "$ntests - $cmd  ====> OK\n" );
        $result = 1;
    } else {
        push( @failureList, "$ntests - $cmd  ====> error code = $rc\n" );
        $result = 0;
    }

    if ( $debug ) { print( "DEBUG: result    = $result (1 is OK).\n" ); }                   
    
    unlink( $tempFile );
    return();
}
##########################################################################################################################
# dumpFileContent: open a file in order to dump its content to an array

sub dumpFileContent {
    my $file = shift;
    my @filecontent;
    my $line;

    open( DUMP, $file ) or die "Unable to open the file $file in read mode.\n";
    foreach $line ( <DUMP> ) {
        $line =~ s/^\s+//;              # remove blanks
        $line =~ s/\s+$//;              # remove blanks
        push( @filecontent, $line );
    }
    close( DUMP );
    return( @filecontent );
}
