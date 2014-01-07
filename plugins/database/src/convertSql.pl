#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# Script to convert the .sql files between Oracle and Postgres form.
#

# PostgresQL and Oracle forms for the table creation, etc, performed
# by the *.sql files in this directory, are mostly the same, so rather
# than maintain two nearly identical copies, this script converts
# icatSysTables.sql and icatCoreTables.sql back and forth.  The only
# difference is that for Postgres we use 'bigint's and for Oracle
# 'integer's.  When Postgresql's smaller integer is OK (rather than
# bigint), the .sql files have use upper case INTEGER which is kept
# the same in both versions.

# arg1 is the db type, optional arg2 is the directory where the
# scripts are (i.e. if not the cwd).

($arg1, $arg2)=@ARGV;

if ($arg2) {
    chdir($arg2);
}

if (!$arg1) {
    print "Usage postgres|oracle|mysql\n";
    die("Invalid (null) argument");
}

if ($arg1 eq "oracle" || $arg1 eq "postgres" || $arg1 eq "mysql") {
}
else {
    print "Usage postgres|oracle|mysql\n";
    die("Invalid argument");
}

print ("$arg1\n");

runCmd("cpp -D$arg1 icatSysTables.sql.pp | grep -v '^#' > icatSysTables.sql");
print "Preprocess icatSysTables.sql.pp to icatSysTables.sql\n";

runCmd("cpp -D$arg1 icatCoreTables.sql.pp | grep -v '^#' > icatCoreTables.sql");
print "Preprocess icatCoreTables.sql.pp to icatCoreTables.sql\n";

exit(0);

# run a command and print a message and exit if it fails.
sub runCmd {
    my($cmd) = @_;
    print "running: $cmd \n";
    $cmdStdout=`$cmd`;
    $cmdStat=$?;
    if ($cmdStat!=0) {
	print "The following command failed:";
	print "$cmd \n";
	print "Exit code= $cmdStat \n";
	die("command failed");
    }
}

