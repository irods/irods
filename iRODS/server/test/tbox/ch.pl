#!/usr/bin/perl

# Handy little script for monitoring a build procedure.

($arg1, $arg2, $arg3)=@ARGV;

$sleepTime = 30;
if ($arg1 eq "-f") {
    $sleepTime = 15;
}
if ($arg1 eq "-t") {
    $out1 = `grep total tinderclient.log | grep exiting | grep -v echo`;
    printf($out1);
    sleep $sleepTime;
}
if ($arg1 eq "-r") {
    $out1 = `tail -100000 tinderbox.out | grep total | grep exiting | grep -v echo`;
    printf($out1);
    exit(0);
}
if ($arg1 eq "-R") {
    $out1 = `tail -1000000 tinderbox.out | grep total | grep exiting | grep -v echo`;
    printf($out1);
    exit(0);
}

while (2 > 1)
{
    $out1 = `grep total tinderclient.log | grep exiting | grep -v echo`;
    printf($out1);
    if ($arg1 ne "-t") {
	$out2 = `ps -el | grep sleep`;
	printf($out2);
	$out = `tail ti*log`;
	printf($out);
	$time=`date`;
	print $time;
    }
    sleep $sleepTime;
}
