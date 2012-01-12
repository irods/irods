#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# Script to list the running irods server and agent(s)
#

# Modify this, if needed, to the irodsServer you wish to monitor
$SERVER_STR="irodsServer";
$AGENT_STR="irodsAgent";
$RE_SERVER_STR="irodsReServer";
$XMS_SERVER_STR="irodsXmsgServer";

($arg1)=@ARGV;

$hostOS=`uname -s`;
chomp($hostOS);
if ("$hostOS" eq "Darwin" ) {
    $psOptions="-axlw";
    $pidField="2";
}
else {
    $psOptions="-elf";
    $pidField="4";
}
if ("$hostOS" eq "SunOS" ) {
    $psOptions="-el";
    $SERVER_STR="irodsSer";
    $AGENT_STR="irodsAge";
    $RE_SERVER_STR="irodsReS";
    $XMS_SERVER_STR="irodsXms";
}

if ($arg1) {
    $psOut = `ps $psOptions | egrep "irods[S|A|Re]" | grep -v grep`;
    print "ps of any irods servers and agents:\n" . $psOut;
    exit(0);
}


# Find the server
$server=`ps $psOptions | egrep "$SERVER_STR" | egrep -v grep`;
chomp($server);

if ($server) {
    print "Server:\n" . $server . "\n";
    print "list\n";
    $serverPid=`echo "$server" | awk '{ print \$$pidField }'`;
    chomp($serverPid);

#print "serverPid: " . $serverPid . "\n";

# Find all irodsAgents that are children of this irodsServer
    $agents=`ps $psOptions | egrep " $serverPid" | egrep $AGENT_STR | egrep -v "$SERVER_STR" | egrep -v grep`;
    print "Agents of this Server: \n" . $agents ;
}
else {
    print "No irodsServer running\n";
}

# Find any irodsReServers
$REs=`ps $psOptions | egrep $RE_SERVER_STR | egrep -v grep`;
chomp($REs);

if ($REs) {
    print "RuleExecution Server: \n" . $REs . "\n";
}
else {
    print "No RuleExecution Server running\n";
}

# Find any XmsgServers
$XMSs=`ps $psOptions | egrep $XMS_SERVER_STR | egrep -v grep`;
chomp($XMSs);

if ($XMSs) {
    print "XMessage Server: \n" . $XMSs . "\n";
}
else {
    print "No XMessage Server running\n";
}

