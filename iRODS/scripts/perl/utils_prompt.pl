#
# Perl

#
# Prompting functions used by iRODS Perl scripts.
#
# Usage:
# 	require scripts/perl/utils_prompt.pl
#
# Dependencies:
# 	none
#
# These utility functions handle prompting.  They require the
# print functions in utils_print.pl.
#

$version{"utils_prompt.pl"} = "March 2010";
my $TRY_TOO_MANY = 20;





# @brief	Ask a yes or no question
#
# Prompt for a yes or no answer using the given prompt string.
# Use the default value on an empty answer.  Return 1 on yes, 0 on no.
#
# @param	$prompt		the prompt string
# @param	$default	the default value string
# @return			the answer (0 = no, 1 = yes)
#
sub promptYesNo( $$ )
{
	my ($prompt,$default) = @_;

	my $tryAgain;
	for ( $tryAgain = 0; $tryAgain < $TRY_TOO_MANY; $tryAgain++ )
	{
		# Prompt
		printQuestion( "    $prompt [$default]? " );

		# Get answer
		my $answer = <STDIN>;
		chomp( $answer );	# remove trailing return
		$answer =~ s/[ \t]*$//;	# remove trailing white space
		$answer =~ s/^[ \t]*//;	# remove leading white space

		# Return default on empty answer
		if ( $answer eq "" )
		{
			$answer = $default;
		}

		# Check if answer is valid
		return 1 if ( $answer =~ /y(es)?/i );
		return 0 if ( $answer =~ /n(o)?/i );

		# The answer is invalid.
		printError( "    Please answer either yes or no.\n\n" );
	}
	printError(
		"\n",
		"Sorry, there have been too many invalid answers.\n",
		"Abort.\n" );
	exit( 1 );
}





# @brief	Ask for a string
#
# Prompt for a string answer using the given prompt string.  If a default
# value is given, use it on an empty answer.
#
# @param	$prompt		the prompt string
# @param	$default	the default
# @return			the answer string
#
sub promptString( $$ )
{
	my ($prompt,$default) = @_;

	my $promptString = "    $prompt";
	if ( !defined( $default ) )
	{
		$default = "";
	}
	elsif ( $default ne "" )
	{
		$promptString .= " [$default]";
	}
	$promptString .= "? ";


	my $tryAgain;
	for ( $tryAgain = 0; $tryAgain < $TRY_TOO_MANY; $tryAgain++ )
	{
		# Prompt
		printQuestion( $promptString );

		# Get answer
		my $answer = <STDIN>;
		chomp( $answer );	# remove trailing return
		$answer =~ s/[ \t]*$//;	# remove trailing white space
		$answer =~ s/^[ \t]*//;	# remove leading white space

		# Check if answer is valid
		if ( $answer eq "" )
		{
			$answer = $default;
		}
		return $answer if ( $answer ne "" );

		# The answer is invalid.
		printError(
			"    Sorry, but the value cannot be left empty.\n\n" );
	}
	printError(
		"\n",
		"Sorry, there have been too many invalid answers.\n",
		"Abort.\n" );
	exit( 1 );
}





# @brief	Ask for a host name
#
# Prompt for a host name answer using the given prompt string.  If a default
# value is given, use it on an empty answer.
#
# The answer must be a legal host name containing only the letters a-Z and
# A-Z, the digits 0-9, a dash, or a period.
#
# @param	$prompt		the prompt string
# @param	$default	the default
# @return			the answer string
#
sub promptHostName( $$ )
{
	my ($prompt,$default) = @_;

	my $tryAgain;
	for ( $tryAgain = 0; $tryAgain < $TRY_TOO_MANY; $tryAgain++ )
	{
		my $answer = promptString( $prompt, $default );
		return $answer if ( $answer !~ /[^a-zA-Z0-9\-\.]+/ );
		printError(
			"    Sorry, but the host name can only include the characters\n",
			"    a-Z A-Z 0-9, dash, or period.\n" );
	}
	printError(
		"\n",
		"Sorry, there have been too many invalid answers.\n",
		"Abort.\n" );
	exit( 1 );
}





# @brief	Ask for an integer
#
# Prompt for an integer answer using the given prompt string.  If a default
# value is given, use it on an empty answer.
#
# The answer must be a legal integer containing only the digits 0-9.
#
# @param	$prompt		the prompt string
# @param	$default	the default
# @return			the answer string
#
sub promptInteger( $$ )
{
	my ($prompt,$default) = @_;

	my $tryAgain;
	for ( $tryAgain = 0; $tryAgain < $TRY_TOO_MANY; $tryAgain++ )
	{
		my $answer = promptString( $prompt, $default );
		return $answer if ( $answer !~ /[^0-9]+/ );
		printError(
			"    Sorry, but the value must be a single positive integer.\n" );
	}
	printError(
		"\n",
		"Sorry, there have been too many invalid answers.\n",
		"Abort.\n" );
	exit( 1 );
}





# @brief	Ask for a non-empty identifier string
#
# Prompt for a non-empty string answer using the given prompt string.
# Return the answer if non-empty.  Otherwise print the given error
# message string and ask the user again.
#
# The answer must be a legal identifier-ish string containing only
# the letters a-z and A-Z, the digits 0-9, a dash, period, or
# underscore.
#
# @param	$prompt		the prompt string
# @param	$default	the default
# @return			the answer string
#
sub promptIdentifier( $$$ )
{
	my ($prompt,$default) = @_;

	my $tryAgain;
	for ( $tryAgain = 0; $tryAgain < $TRY_TOO_MANY; $tryAgain++ )
	{
		my $answer = promptString( $prompt, $default );
		return $answer if ( $answer !~ /[^a-zA-Z0-9_\-\.]+/ );
		printError(
			"    Sorry, but the value can only include the characters\n",
			"    a-Z A-Z 0-9, dash, period, and underscore.\n" );
	}
	printError(
		"\n",
		"Sorry, there have been too many invalid answers.\n",
		"Abort.\n" );
	exit( 1 );
}







return( 1 );
