#
# Perl

#
# File functions used by iRODS Perl scripts.
#
# Usage:
# 	require scripts/perl/utils_file.pl
#
# Dependencies:
# 	None
#
# This file provides utility functions to handle reading, writing,
# and simply editing files for the iRODS Perl scripts.

use File::Spec;
use File::Copy;

$version{"utils_file.pl"} = "September 2011";





#
# Design notes:  In-place editing
#	These functions go through special effort to modify files
#	in-place, whenever possible.  The alternative would be to
#	create a new file, delete the old file, then put the new
#	file back where the old file was.  There are several problems
#	with that delete-and-swap approach:
#		* The new file may not have the old file's permissions.
#		* The new file may not have the old file's ownership.
#		* Hard links to the old file don't go to the new file.
#		* On a Mac, extra per-file info (such as color, icon,
#		  or comments) gets lost.
#





########################################################################
#
# File paths
#

#
# @brief	Create a path to a temp file.
#
# Construct a path to a temp file in the host's "temporary files"
# directory, which varies depending upon the OS.  The file has a name
# of the form:
#
# 	irods_NAME_$$.tmp
#
# where NAME is the name argument given to this function and $$ is the
# unique process ID.  The "irods_" in front helps identify the source
# of the files if they don't get deleted and somebody browses the
# directory.  The ".tmp" on the end is a semi-standard filename
# extension for temp files.
#
# @param	$name
# 	the NAME part of the constructed temporary file name
# @return
# 	the full path to the temporary file
#
sub createTempFilePath($)
{
	my ($name) = @_;

	# File::Spec->tmpdir() returns an OS-specific temp directory.
	# On UNIX-based systems, this is often /tmp, /usr/tmp, or /var.
	return File::Spec->catfile(
		File::Spec->tmpdir( ),
		"irods_" . $name . "_$$.tmp" );
}





########################################################################
#
# File writing
#

#
# @brief	Write text to a new file.
#
# Replace the file's content with the given text.
#
# @param	$file
# 	the name of the file
# @param	$text
# 	the text to write
#
sub printToFile
{
	my ($file, @text) = @_;

	# Do not delete the old file first.  Instead, when using
	# open() below, use ">" to write to the file, truncating it
	# if the file already exists.  This insures that the file
	# is changed in-place, retaining its existing permissions,
	# ownership, and other attributes.
	open( FILE, ">$file" );
	my $entry;
	foreach $entry (@text)
	{
		print( FILE $entry );
	}
	close( FILE );
}





#
# @brief	Append text to a new file.
#
# Add the given text to the end of the file's content.
#
# @param	$file
# 	the name of the file
# @param	$text
# 	the text to write
#
sub appendToFile
{
	my ($file, @text) = @_;

	# Append to the file, keeping its existing content.
	open( FILE, ">>$file" );
	my $entry;
	foreach $entry (@text)
	{
		print( FILE $entry );
	}
	close( FILE );
}





#
# @brief	Replace variable values in a file.
#
# Replace lines that set variables with new lines and new values.
# Several syntax types are handled:
# 	perl	$variable = "value";
# 	make	variable = value
# 	shell	variable="value"
# 	config	variable value
#
# @param	$filename	
# 	the file to edit
# @param	$syntax
# 	the file type ("make", "perl", "shell', "config")
# @param	$appendIfNotReplaced
# 	if true, append variables not replaced
# @param	%values
# 	an array of keys to match and values
# @return
#	a 2-tuple containing a numeric status code and a text
#	error message.  Status codes are:
# 		0 = failure
# 		1 = success, file changed
# 		2 = success, file unchanged
#
sub replaceVariablesInFile
{
	my ($filename, $syntax, $appendIfNotReplaced, %values) = @_;
	my $output = "";

	# Create a temporary file path.  Delete an old temp file if
	# it already exists.
	my $tmpPath = createTempFilePath( "varreplace" );
	if ( -e $tmpPath )
	{
		unlink( $tmpPath );
	}

	# Copy the source file to the temp file.  We'll be
	# reading that copy and overwriting the original.  This
	# insures that the original file is modified in-place,
	# keeping its existing permissions, ownership, etc.
	if ( copy( $filename, $tmpPath ) != 1 )
	{
		$output = "Cannot copy file:  $filename\n";
		return (0, $output);
	}

	# Open copied file for reading.
	if ( ! defined( open( FileIn, "<$tmpPath" ) ) )
	{
		$output = "Cannot read file:  $tmpPath\n";
		return (0, $output);
	}

	# Open the original file for writing.  Truncate the
	# file's contents.  We'll put the contents back below.
	if ( ! defined( open( FileOut, ">$filename" ) ) )
	{
		$output = "Cannot write file:  $filename\n";
		return (0, $output);
	}

	# Look for lines that start with the key.  The syntax
	# of lines, and the values written to replace them,
	# depends upon the $syntax argument.
	my %replacedValues = ();
	my $fileChanged = 0;
	if ( $syntax =~ /perl/i )
	{
		# Use Perl syntax
		#
		# This is used by iRODS configuration files
		# read by Perl scripts.  Values should be
		# in quotes.  Empty values are OK.
		#
		# Use single quotes so that Perl doesn't
		# process the value when it is read back in.
		# This insures that @, $, and other special
		# characters that might be in the value
		# remain in the value.
		while( $line = <FileIn> )
		{
			foreach $name (keys %values)
			{
				if ( $line =~ /^\$$name[ \t]*=/ )
				{
					# $NAME = 'value';
					my $value = $values{$name};
					$line = "\$$name = \'$value\';\n";
					$replacedValues{$name} = 1;
					$fileChanged = 1;
					last;
				}
			}
			print( FileOut $line );
		}
	}
	elsif ( $syntax =~ /shell/i )
	{
		# Use Shell syntax
		#
		# This is used by iRODS shell scripts.  Values
		# should be in quotes.  Empty values are OK.
		while( $line = <FileIn> )
		{
			foreach $name (keys %values)
			{
				if ( $line =~ /^$name=/ )
				{
					# NAME="value"
					my $value = $values{$name};
					$line = "$name=\"$value\"\n";
					$replacedValues{$name} = 1;
					$fileChanged = 1;
					last;
				}
			}
			print( FileOut $line );
		}
	}
	elsif ( $syntax =~ /config/i )
	{
		# Use config syntax
		#
		# This is used by the iRODS config files, such as
		# server.config.  Values should not be in quotes.
		# If a value is empty, skip the line.
		while( $line = <FileIn> )
		{
			foreach $name (keys %values)
			{
				if ( $line =~ /^#?$name/ )
				{
					# If value is not empty:
					# 	NAME value
					# else
					# 	#NAME
					my $value = $values{$name};
					if ( $value eq "" )
					{
						$line = "#$name\n";
					}
					else
					{
						$line = "$name $value\n";
					}
					$replacedValues{$name} = 1;
					$fileChanged = 1;
					last;
				}
			}
			print( FileOut $line );
		}
	}
	elsif ( $syntax =~ /simple/i )
	{
		# Use simple syntax
		#
		# Just do it; simply replace the string
		while( $line = <FileIn> )
		{
			foreach $name (keys %values)
			{
			        my $value = $values{$name};
				$line =~ s/$name/$value/g; 
				$fileChanged = 1;
			}
			print( FileOut $line );
		}	    
	}
	else
	{
		# Use Makefile syntax
		#
		# This is used by iRODS Makefiles.  Values should
		# not be in quotes.  If a value is empty, comment
		# out the line.  This insures that empty values
		# are not defined, which makes 'if NAME' work.
		while( $line = <FileIn> )
		{
			foreach $name (keys %values)
			{
				# Don't replace more than once.
				next if ( $replacedValues{$name} == 1 );

				if ( $line =~ /^#?[ \t]*$name[ \t]*=/ )
				{
					# If value is not empty:
					# 	NAME=value
					# else
					# 	#NAME=
					my $value = $values{$name};
					if ( $value eq "" )
					{
						$line = "#$name=\n";
					}
					else
					{
						$line = "$name=$value\n";
					}
					$replacedValues{$name} = 1;
					$fileChanged = 1;
					last;
				}
			}
			print( FileOut $line );
		}
	}
	close( FileIn );


	# If we are to append unreplaced values, do so now.
	if ( $appendIfNotReplaced )
	{
		foreach $name (keys %values)
		{
			# Was this value replaced?
			if ( defined( $replacedValues{$name} ) )
			{
				next;
			}

			# No.  Append it.
			my $value = $values{$name};
			$fileChanged = 1;
			if ( $syntax =~ /perl/i )
			{
				# $NAME = 'value';
				print( FileOut "\$$name = \'$value\';\n" );
			}
			elsif ( $syntax =~ /shell/i )
			{
				# NAME="value"
				print( FileOut "$name=\"$value\"\n" );
			}
			elsif ( $syntax =~ /config/i )
			{
				# If value is not empty:
				# 	NAME value
				# else
				# 	#NAME
				if ( $value eq "" )
				{
					print( FileOut "#$name\n" );
				}
				else
				{
					print( FileOut "$name $value\n" );
				}
			}
			else
			{
				# If value is not empty:
				# 	NAME=value
				# else
				# 	#NAME=
				if ( $value eq "" )
				{
					print( FileOut "#$name=\n" );
				}
				else
				{
					print( FileOut "$name=$value\n" );
				}
			}
		}

		# For Perl config files, the last line of the file sets
		# the return type for a 'require'-ed file.  Since we don't
		# know if the last variable output above evaluates to '1',
		# we need to add an extra variable that does.
		if ( $syntax =~ /perl/i )
		{
			print( FileOut "\n\$return_result = 1;\n" );
		}
	}
	close( FileOut );


	# The original file has now been overwritten with new lines.
	# The copy of the file is no longer needed and can be deleted.
	unlink( $tmpPath );

	return (1,undef) if ( $fileChanged );
	return (2,undef);
}






#
# @brief	Compare the contents of two files
#
# Two files are read into memory and compared.  If they differ in
# any way, 1 is returned.  Otherwise 0.
#
# @param	$filename1
# 	the first file
# @param	$filename2
# 	the second file
# @return
# 	1 if differ, 0 if same
#
sub diff($$)
{
	my ($filename1,$filename2) = @_;

	# Read in each file
	if ( open( FILE1, "<$filename1" ) == 0 )
	{
		# Can't open first file.  Must differ.
		return true;
	}
	my @first = <FILE1>;
	close( FILE1 );

	if ( open( FILE2, "<$filename2" ) == 0 )
	{
		# Can't open first file.  Must differ.
		return true;
	}
	my @second = <FILE2>;
	close( FILE2 );

	return 1 if ( $#first != $#second );
	my $n = $#first;
	for ( $i = 0; $i < $n; $i++ )
	{
		return 1 if ( $first[$i] ne $second[$i] );
	}
	return 0;
}





#
# @brief	Get a variable value from a name-value pair in a file
#
# Name-value pair files, such as "info.txt" for modules, each
# line gives a property name and a value:
#
# 	name: value...
#
# This function reads such a file, finds a line matching the given
# keyword, and returns the value.
#
# @param	$filename
# 	the file with name-value pairs
# @param	$keyword
# 	the name
# @return
#	the value
#
sub getPropertyValue($$$)
{
	my ($filename, $keyword, $case) = @_;

	# Read the file
	if ( open(  FILE, "<$filename" ) == 0 )
	{
		return undef;
	}

	# Find the desired line
	foreach $line ( <FILE> )
	{
		if ( (($val) = ($line =~ /^$keyword[ \t]*:[ \t]*(.*)$/i)) )
		{
			close( FILE );
			return $val;
		}
	}
	close( FILE );
	return undef;
}





#
# @brief	Copy template file if a file doesn't exist.
#
# Many iRODS configuration files have templates that provide a
# starting point.  If the configuration file doesn't exist yet,
# this function copies the template into place.  It handles
# several "standard" conventions in use now or in the past for
# the names of iRODS templates.
#
# @param	$configPath
# 	the configuration file path
# @return
#	a numeric code indicating if an error occurred
# 		0 = fail (no template, no permissions)
# 		1 = config file already exists
# 		2 = template copied
#
sub copyTemplateIfNeeded($)
{
	my ($configPath) = @_;

	# If the configuration file already exists, return.  There
	# is no need to copy a template.
	if ( -e $configPath )
	{
		return 1;
	}

	# Extract the directory path from the config file path
	my ($volume,$dirPath,$filename) = File::Spec->splitpath( $configPath );

	# Try several template file names.  Find one that exists
	# and copy it.
	my @templateNames = ( ".template", ".in", ".example" );
	foreach $template ( @templateNames )
	{
		my $templatePath = File::Spec->catpath( $volume, $dirPath,
			$filename . $template );
		if ( -e $templatePath )
		{
			if ( copy( $templatePath, $configPath ) != 1 )
			{
				# Couldn't copy?  Permissions?
				return 0;
			}
			return 2;
		}
	}

	# Couldn't find template?
	return 0;
}

#
# @brief	Copy template file if a file doesn't exist or if the
#               template is newer (from a cvs update, for example).
#
# Like copyTemplateIfNeeded but will also copy the template if it
# is newer than the config file.  Also, this version just checks
# for the one template file as the rest are unneeded.
#
# @param	$configPath
# 	the configuration file path
# @return
#	a numeric code indicating if an error occurred
# 		0 = fail (no template, no permissions)
# 		1 = config file already exists and is current
# 		2 = template copied
#
sub copyTemplateIfNeededOrNewer($)
{
	my ($configPath) = @_;

	# Extract the directory path from the config file path
	my ($volume,$dirPath,$filename) = File::Spec->splitpath( $configPath );

	my $template = ".template";
	my $templatePath = File::Spec->catpath( $volume, $dirPath,
						$filename . $template );

	if (!-e $templatePath )	{
	    return 0; 	    # Couldn't find template
	}

	# Get template modify time
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
	    $atime,$mtimeT,$ctime,$blksize,$blocks) = stat($templatePath); 

	# If the configuration file already exists, check if it is older
	if ( -e $configPath ) {
	    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
		$atime,$mtimeC,$ctime,$blksize,$blocks) = stat($configPath); 
	    if ($mtimeT < $mtimeC) {
		return 1;  # no need to copy, template is older
	    }
	}

	if ( copy( $templatePath, $configPath ) != 1 )   {
	    # Couldn't copy?  Permissions?
	    return 0;
	}
	return 2;
}





return( 1 );
