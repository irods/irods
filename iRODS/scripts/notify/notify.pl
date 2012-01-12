#!/usr/bin/perl
# Simple script to monitor iRODS collections and send email when files
# are added or removed.  The script is run periodically and it queries
# (via iquest) and records the number of files and number of bytes
# stored in the monitored collections, and when they change, emails
# the specified accounts.  Configuration is in the $notifyConfig file in
# which each line is of the form:
# Collection Name,Last Name,First Name,Organization,Email Address,Phone Number
# This script uses the Collection Name and Email Address items.
# Collection Name must be the full path.
#
# The same collection can appear multiple times to send multiple
# emails.  The script will handle this efficiently, processing each
# unique collection once per iteration and when there are changes
# emailing the notification to one or more accounts.
#
# This script should be rerun at the period at which you want to be
# notified.  The trade-off is getting multiple notifications versus
# some delays.  For example, if changes typically take 10 minutes or
# so, you might run this once an hour so that you'll usually get just
# one notification for a change set and within an hour of the update.
#
# The records are kept in the $notifyData file, which can be manually
# truncated when desired.
#
# The 'icommands' are assumed to be in the path ('iquest' is used) and
# that an 'iinit' was done so it can function.  A few normal
# linux/unix commands are assumed to be available too (grep, echo,
# date, mail, tail).
#
# You can use different files if you want by changing these,
# they are full path names so this can be easily run via cron:
$notifyConfig="/home/schroeder/cron/notify.config";
$notifyData="/home/schroeder/cron/data.txt";

#For running from cron, path for iquest:
$iquest="/home/schroeder/svn/trunk/iRODS/clients/icommands/bin/iquest";

#
$debug=0;
$debugMail=0;
#
$collectionCount=0;
@collections;
@emailAddrs;
$size;
$count;
$change;
$changeBrief;

#
# Add a Collection to the list to do.  If it is not unique, instead of
# adding it, just add in the email address for it.
sub addCollection($$) {
    my ($collection, $emailAddress) = @_;
    if ($collectionCount>0) {
	for ($i=0;$i<$collectionCount;$i++) {
	    if (@collections[$i] eq $collection) {
		@emailAddrs[$i] = @emailAddrs[$i] . "," . $emailAddress;
		return;
	    }
	}
    }
    @collections[$collectionCount]=$collection;
    @emailAddrs[$collectionCount]=$emailAddress;
    $collectionCount++;
}

sub printCollections() {
    for ($i=0;$i<$collectionCount;$i++) {
	print "c:" . @collections[$i] . "\n";
	print "e:" . @emailAddrs[$i] . "\n";
    }
}

# Query to get the number of files and bytes for a collection and
# update the record file with the values.
sub updateCollectionRecord($) {
    my ($collection) = @_;

    my $command = "$iquest \"%s\" \"select count(DATA_NAME) where COLL_NAME like \'$collection%\'\" ";

    if ($debug==1) { print $command . "\n"; }
    my $output = `$command`;
    my $cmdStat=$?;
    if ($debug==1) { print "out:" . $output . ":\n"; }
    if ($cmdStat != 0) {
	print "command failed, exiting \n";
	exit(1);
    }

    chomp($output);
    $count=$output;
    if (length($count)==0) {
	print "No result\n";
	return;
    }

    my $command2 = "$iquest \"%s\" \"select sum(DATA_SIZE) where COLL_NAME like \'$collection%\'\" ";
    my $cmdStat=$?;
    if ($debug==1) { print $command2 . "\n"; }

    my $output2 = `$command2`;
    my $cmdStat=$?;
    if ($debug==1) { print "out2:" . $output2 . ":\n"; }
    if ($cmdStat != 0) {
	print "command failed, exiting \n";
	exit(1);
    }

    chomp($output2);
    $size=$output2;
    if (length($size)==0) {
	$size = "0";
    }

    $date = `date`;
    chomp($date);
    $echo = $date . "," . $collection . "," . $count . "," . $size ;
    if ($debug==1) { printf("echo:$echo:\n"); }
    `echo $echo >> $notifyData`;
}

sub getLastCollectionValues($) {
    my ($collection) = @_;
    my $grep = "," . $collection . ",";
    my $command = "grep $grep $notifyData | tail -1";
    if ($debug==1) { print $command . "\n"; }
    my $output = `$command`;
    if ($debug==1) { print "out:" . $output . ":\n"; }

    chomp($output);
    my @values = split(',', $output);
    $count = @values[2];
    $size = @values[3];
}

sub sendNotices($$) {
    my ($collection, $emailAddresses) = @_;

    $i = rindex($collection, "/");
    $shortCollName = substr($collection, $i+1);
#   print "coll:" . $collection . "\n";
#   print "short Coll:" . $shortCollName . "\n";

    $body = "iRODS collection change notification for $collection, $change";
    $subj = "iRODS change notice for $shortCollName, $changeBrief";

    my @values = split(',', $emailAddresses);
    foreach my $email (@values) {
	if ($debugMail == 1) {
	    print "would have emailed: $email\n";
	    printf "body:$body\n";
	    printf "subj:$subj\n";
	}
	else {
	    `echo $body | mail -s \"$subj\" $email`;
	}
    }
}

sub checkCollection($$) {
    my ($collection, $emailAddresses) = @_;
    getLastCollectionValues($collection);
    if ($debug==1) {
	print "count: $count \n";
	print "size: $size \n";
    }
    my $oldCount=$count;
    my $oldSize=$size;
    updateCollectionRecord($collection);

    if (length($oldSize)==0) {return;}

    $change="";
    $changeBrief="";
    if ($count < $oldCount) {
	$delta = $oldCount-$count;
	if ($delta == 1) {
	    $change = $change . "1 file removed";
	    $changeBrief=$changeBrief . "file removed";
	}
	else {
	    $change = $change . "$delta files removed";
	    $changeBrief=$changeBrief . "files removed";
	}
    }
    if ($count > $oldCount) {
	$delta=$count-$oldCount;
	if ($delta == 1) {
	    $change = $change . "1 file added";
	    $changeBrief=$changeBrief . "file added";
	}
	else {
	    $change = $change . "$delta files added";
	    $changeBrief=$changeBrief . "files added";
	}
    }
    if ($size > $oldSize) {
	$delta = $size - $oldSize;
	if (length($change) > 0) { $change = $change . ", "}
	$change = $change . "total size increased by $delta bytes";
	$changeBrief=$changeBrief . ", total size increased";
    }
    if ($size < $oldSize) {
	$delta = $oldSize - $size;
	if (length($change) > 0) { $change = $change . ", "}
	$change = $change . "total size decreased by $delta bytes";
	$changeBrief=$changeBrief . ", total size decreased";
    }
    if ($debug==2) { print "change: $change \n"; }

    if (length($change) > 0) {
	sendNotices($collection, $emailAddresses);
    }
}

$date = `date`;
print "Running at $date";

open (MYFILE, $notifyConfig);

while (<MYFILE>) {
    chomp;
    my @values = split(',', $_);
    addCollection(@values[0],@values[4]);
}
close (MYFILE); 

if ($debug==1) {
    printCollections();
}

for ($i=0;$i<$collectionCount;$i++) {
    checkCollection(@collections[$i],@emailAddrs[$i]);
}
