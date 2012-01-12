#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This script joins together all the lines on stdin into one long line
# on stdout.
#

# Read the file
chomp( @data = <STDIN> );

# Find the desired line and print it out
$result="";
foreach $line ( @data )
{
	$result = "$result $line";
}
print $result;
exit( 0 );
