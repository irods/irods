#!/usr/local/bin/perl

# Handy little script for monitoring a build procedure.

($arg1, $arg2, $arg3)=@ARGV;

$sleepTime = 30;
if ($arg1 eq "-f") {
    $sleepTime = 15;
}

while (2 > 1)
{
    $out = `ps`;
    printf($out);
    $time=`date`;
    print $time;
    sleep $sleepTime;
}
