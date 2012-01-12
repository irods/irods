#
# Perl

#
# Printing functions used by iRODS Perl scripts.
#
# Usage:
# 	require scripts/perl/utils_print.pl
#
# Dependencies:
# 	none
#
# These utility functions handle colored and formatted printing
# for iRODS perl scripts.
#

use IO::File;
use POSIX;

$version{"utils_print.pl"} = "March 2010";




#
# Design Notes: color printing
#	Color output can help highlight messages of interest (such as
#	red for errors).  The problem is that we don't know (a) the
#	output is going to a terminal, (b) if that terminal can handle
#	color, and (c) if the user's terminal background color is
#	light or dark.
#
#	We use POSIX to determine if STDOUT is a tty.  If not, color
#	output is disabled.  If it is a tty, and $TERM is a type known
#	to support color, color is enabled.  Today, this is
#	very often "xterm" or "xterm-color", which supports color.
#	Otherwise, colors are disabled.
#
#	Unfortunately, there is no standard way to know if the user's
#	terminal window is light or dark.
#
# Design Notes:  color scheme
# 	Since we don't know if the screen background is white or black
# 	(or something unusual), we have to pick text colors that will
# 	work with both.  While many xterms are configured for 256 colors,
# 	some still are not.  So we are limited to 8 colors:
#
# 		black, red, green, yellow, blue, pink, cyan, white
#
# 	Black and white cannot be used because backgrounds are often
# 	one or the other.
#
# 	Yellow is hard to read on a white screen.
#
#	Blue is hard to read on a black screen.
#
#	Red, pink, and green work reasonably well on black and white.
#	Cyan is weak.
#
#	For color info, see:
#		http://www.frexx.de/xterm-256-notes/
#		http://rtfm.etla.org/xterm/ctlseq.html
#





########################################################################
#
# Print configuration
#
# Escape sequences for common colors.
my $BLACK  = "\033[30m";
my $RED    = "\033[31m";
my $GREEN  = "\033[32m";
my $YELLOW = "\033[33m";
my $BLUE   = "\033[34m";
my $PINK   = "\033[35m";
my $CYAN   = "\033[36m";
my $WHITE  = "\033[37m";

# Escape sequences for bold colors.
my $BOLD_BLACK  = "\033[1;30m";
my $BOLD_RED    = "\033[1;31m";
my $BOLD_GREEN  = "\033[1;32m";
my $BOLD_YELLOW = "\033[1;33m";
my $BOLD_BLUE   = "\033[1;34m";
my $BOLD_PINK   = "\033[1;35m";
my $BOLD_CYAN   = "\033[1;36m";
my $BOLD_WHITE  = "\033[1;37m";

# Escape sequences for bold colors.
my $BRIGHT_BLACK  = "\033[90m";
my $BRIGHT_RED    = "\033[91m";
my $BRIGHT_GREEN  = "\033[92m";
my $BRIGHT_YELLOW = "\033[93m";
my $BRIGHT_BLUE   = "\033[94m";
my $BRIGHT_PINK   = "\033[95m";
my $BRIGHT_CYAN   = "\033[96m";
my $BRIGHT_WHITE  = "\033[97m";

# Bold-only, using the current color.
my $BOLD = "\033[1m";

# Reset returns color choices to whatever the user has as a default.
my $RESET = "\033[0m";

# Initially, there are no color choices.  Color printing must be
# enabled.
my $TITLE_COLOR    = "";
my $SUBTITLE_COLOR = "";
my $NOTICE_COLOR   = "";
my $STATUS_COLOR   = "";
my $ERROR_COLOR    = "";
my $QUESTION_COLOR = "";
my $NORMAL_COLOR   = "";





#
# @brief	Enable or disable colored printing.
#
# By default, all of the printXXX() functions print their messages in
# color, when the terminal supports color.  Errors are in red, etc.
# This can be disabled and enabled.
#
# @param	$onOff
# 	1 to enable color (default), 0 to disable
#
my $PRINT_COLOR = 1;

sub enablePrintColors($)
{
	my ($onOff) = @_;
	if ( $onOff )
	{
		# There is no clear way to be sure that all the colors
		# we pick are readable in a terminal with the user's
		# color scheme.  However, we can be reasonably sure that
		# the background isn't bright yellow, green, cyan, or red.
		# Nobody does that.  What we are unsure about is if the
		# background is white or black.  So, none of these
		# colors is black or white.
		#
		# We also use the BRIGHT (bold) versions so they are
		# more likely to stand out from normal text.
		$PRINT_COLOR = 1;
		$TITLE_COLOR    = $BOLD_GREEN;
		$SUBTITLE_COLOR = $BOLD_GREEN;
		$NOTICE_COLOR   = $RESET;
		$STATUS_COLOR   = $RESET;
		$ERROR_COLOR    = $BOLD_RED;
		$QUESTION_COLOR = $BOLD;
		$NORMAL_COLOR   = $RESET;	# black or white or
						# whatever the user
						# has as a default
	}
	else
	{
		$PRINT_COLOR = 0;
		$TITLE_COLOR    = "";
		$SUBTITLE_COLOR = "";
		$NOTICE_COLOR   = "";
		$STATUS_COLOR   = "";
		$ERROR_COLOR    = "";
		$QUESTION_COLOR = "";
		$NORMAL_COLOR   = "";
	}
}





#
# @brief	Enable color printing based upon the TERM type.
#
# The TERM environment variable indicates if the terminal supports
# color printing.  If it doesn't, we disable color printing.
#
sub enablePrintColorsIfSupported()
{
	# First, if stdout is not a tty (it's a file or pipe),
	# then don't use colors.  The escape sequences would
	# just make the file output hard to read.
	if ( !POSIX::isatty( STDOUT ) )
	{
		enablePrintColors( 0 );
		return;
	}

	# Second, if the terminal is one known to support color,
	# enable it.
	if ( $ENV{"TERM"} =~ /xterm/i ||	# xterm, xterm-color, xterm-256color
		$ENV{"TERM"} =~ /vt10[02]/i ||	# vt100, vt102
		$ENV{"TERM"} =~ /vt[23]20/i ||	# vt220, vt230
		$ENV{"TERM"} =~ /vtnt/i ||	# vtnt
		$ENV{"TERM"} =~ /ansi/i )	# ansi
	{
		enablePrintColors( 1 );
		return;
	}

	# Otherwise, disable colors.  The escape sequences will
	# look odd on non-color-aware terminals.
	enablePrintColors( 0 );
}

enablePrintColorsIfSupported( );





#
# @brief	Set or unset message verbosity.
#
# In verbose mode (default), all calls to the printXXX functions print
# their messages.  When verbosity is disabled, only error messages are
# printed.
#
# @param	$onOff
# 	1 to enable message output, 0 to disable
#
my $PRINT_VERBOSE = 1;

sub setPrintVerbose($)
{
	my ($verbose) = @_;
	if ( $verbose )
	{
		$PRINT_VERBOSE = 1;
	}
	else
	{
		$PRINT_VERBOSE = 0;
	}
}





#
# @brief	Get the message verbosity.
#
# Return 1 if verbosity is enabled (default).
#
# @return
# 	1 if verbose, 0 if quiet
#
sub isPrintVerbose()
{
	return $PRINT_VERBOSE;
}





#
# @brief	Set a master indent.
#
# A 'master indent' is prepended to every line output.  This can be
# used to indent a large block of messages without having to modify
# every print message.
#
# @param	$indent
# 	the indent string (usually spaces)
#
my $MASTER_INDENT = "";

sub setMasterIndent($)
{
	($MASTER_INDENT) = @_;
}





########################################################################
#
# Print messages
#

#
# @brief	Print a message with a color.
#
# @param	$color
# 	the color string
# @param	@message
# 	an array of message strings
#
sub printMessage
{
	my ($color,@message) = @_;
	return if ( $#message < 0 );

	# If the first message line is just white space, then
	# presume it is an indent for the rest of the message
	# lines.  But if there's nothing more in the message
	# array, then it isn't an indent.
	my $indent = $MASTER_INDENT;
	if ( $message[0] =~ /^[ \t]+$/ && $#message > 0 )
	{
		$indent .= $message[0];
		shift @message;
	}

	# Print the message line-by-line.
	my $entry;
	foreach $entry (@message)
	{
		# Add the color choice and indent at the start.
		$entry =~ s/^/$color$indent/g;

		# Add the indent for intermediate carriage returns.
		$entry =~ s/\n(?!$)/\n$indent/g;

		# Print and add the color reset to the end.
		print( "$entry$NORMAL_COLOR" );
	}
}





#
# @brief	Print a title message.
#
# @param	@message
# 	an array of message strings
#
sub printTitle
{
	return if !$PRINT_VERBOSE;
	printMessage( $TITLE_COLOR, @_ );
}





#
# @brief	Print a subtitle message.
#
# @param	@message
# 	an array of message strings
#
sub printSubtitle
{
	return if !$PRINT_VERBOSE;
	printMessage( $SUBTITLE_COLOR, @_ );
}





#
# @brief	Print a notice message.
#
# @param	@message
# 	an array of message strings
#
sub printNotice
{
	return if !$PRINT_VERBOSE;
	printMessage( $NOTICE_COLOR, @_ );
}





#
# @brief	Print a status message.
#
# @param	@message
# 	an array of message strings
#
sub printStatus
{
	return if !$PRINT_VERBOSE;
	printMessage( $STATUS_COLOR, "    ", @_ );
}





#
# @brief	Print a question message.
#
# @param	message		the message to print
#
sub printQuestion
{
	printMessage( $QUESTION_COLOR, @_ );
}





#
# @brief	Print an error message.
#
# @param	message		the message to print
#
sub printError
{
	printMessage( $ERROR_COLOR, @_ );
}





#
# @brief	Ask the user for yes/no and return the answer.
#
# Prompt the user for a yes or no answer and return 1 for yes, 0 for no.
#
# @param	$prompt
# 	the prompt to print
# @return
# 	0 for no, 1 for yes.
# @deprecated	Use promptYesNo() instead.
#
sub askYesNo($)
{
	my ($prompt) = @_;

	# Print the prompt
	if ( defined( $prompt ) )
	{
		print( "$NOTICE_COLOR$prompt$NORMAL_COLOR" );
	}

	# Get user input
	my $answer = <STDIN>;
	chomp( $answer );	# remove trailing return
	$answer =~ s/[ \t]*$//;	# remove trailing white space
	$answer =~ s/^[ \t]*//;	# remove leading white space

	# Is it a "yes"?
	if ( $answer =~ /y(es)?/i )
	{
		return 1;
	}
	return 0;
}





#
# @brief	Ask the user for a string answer and return it.
#
# Prompt the user for a text string and return it.  Keep prompt
# the user until they enter a non-empty string.
#
# @param	$prompt
# 	the prompt to print
# @param	$emptyError
# 	the message to print if the text is empty.
# @return
# 	the text.
# @deprecated	Use promptString() instead.
sub askString($$)
{
	my ($prompt,$emptyError) = @_;
	my $answer = undef;

	for ( $tryAgain = 0; $tryAgain < 20; $tryAgain++ )
	{
		printNotice( $prompt );
		$answer = <STDIN>;
		chomp( $answer );	# remove trailing return
		$answer =~ s/[ \t]*$//;	# remove trailing white space
		$answer =~ s/^[ \t]*//;	# remove leading white space
		if ( $answer ne "" )
		{
			return $answer;
		}
		printError( $emptyError );
	}
	printError(
		"\n",
		"Sorry, there have been too many invalid answers.\n",
		"Abort.\n" );
	exit( 1 );
}





########################################################################
#
# Log messages
#

#
# @brief	Open a log file.
#
# Open the general log file.
#
# @param	$logfile
# 	the name of the log file
#
my $LogOpen = 0;
sub openLog($)
{
	my ($logfile) = @_;
	if ( -e $logfile )
	{
		unlink( $logfile );
	}
	open( LOG, ">$logfile" );
	$LogOpen = 1;
}





#
# @brief	Close the log file.
#
# Close the general log file.
#
sub closeLog()
{
	if ( $LogOpen == 1 )
	{
		close( LOG );
		$LogOpen = 0;
	}
}




#
# @brief	Get the log file handle.
#
# @return	$log
# 	the log file handle
#
sub getLog()
{
	return undef if ( $LogOpen == 0 );
	return LOG;
}




#
# @brief	Print to the log file.
#
# Print a scalar or array message to the log file.
#
# @param	@message
# 	an array of message strings
#
sub printLog
{
	my (@message) = @_;
	return if ( $LogOpen == 0 );
	return if ( $#message < 0 );

	# If the first message line is just white space, then
	# presume it is an indent for the rest of the message
	# lines.  But if there's nothing more in the message
	# array, then it isn't an indent.
	my $indent = "";
	if ( $message[0] =~ /^[ \t]+$/ && $#message > 0 )
	{
		$indent = $message[0];
		shift @message;
	}

	# Print the message line-by-line.
	my $entry;
	foreach $entry (@message)
	{
		# Add the indent at the start.
		$entry =~ s/^/$indent/g;

		# Add the indent for intermediate carriage returns.
		$entry =~ s/\n(?!$)/\n$indent/g;

		# Print.
		print( LOG $entry );
	}
}



return( 1 );
