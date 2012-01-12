/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/**************************************
INCLUDE files for :regular expression comparator
**************************************/
/* +++Date last modified: 05-Jul-1997 */
/*
EPSHeader
File: match.h
Author: J. Kercheval
Created: Sat, 01/05/1991 22:27:18
*/
/*
EPSRevision History
J. Kercheval Wed, 02/20/1991 22:28:37 Released to Public Domain
J. Kercheval Sun, 03/10/1991 18:02:56 add is_valid_pattern
J. Kercheval Sun, 03/10/1991 18:25:48 add error_type in is_valid_pattern
J. Kercheval Sun, 03/10/1991 18:47:47 error return from matche()
J. Kercheval Tue, 03/12/1991 22:24:49 Released as V1.1 to Public Domain
*/
#ifndef MATCH__H
#define MATCH__H
/*
Wildcard Pattern Matching
*/
#ifndef BOOLEAN
# define BOOLEAN int
# define TRUE 1
# define FALSE 0
#endif
/* match defines */
#define MATCH_PATTERN 6/* bad pattern */
#define MATCH_LITERAL 5/* match failure on literal match */
#define MATCH_RANGE 4/* match failure on [..] construct */
#define MATCH_ABORT 3/* premature end of text string */
#define MATCH_END 2/* premature end of pattern string */
#define MATCH_VALID 1/* valid match */
/* pattern defines */
#define PATTERN_VALID 0/* valid pattern */
#define PATTERN_ESC -1/* literal escape at end of pattern */
#define PATTERN_RANGE -2/* malformed range in [..] construct */
#define PATTERN_CLOSE -3/* no end bracket in [..] construct */
#define PATTERN_EMPTY -4/* [..] construct is empty */
/*----------------------------------------------------------------------------
*
* Match the pattern PATTERN against the string TEXT;
*
*match() returns TRUE if pattern matches, FALSE otherwise.
*matche() returns MATCH_VALID if pattern matches, or an errorcode
*as follows otherwise:
*
*MATCH_PATTERN - bad pattern
*MATCH_LITERAL - match failure on literal mismatch
*MATCH_RANGE- match failure on [..] construct
*MATCH_ABORT- premature end of text string
*MATCH_END - premature end of pattern string
*MATCH_VALID- valid match
*
*
* A match means the entire string TEXT is used up in matching.
*
* In the pattern string:
*`*' matches any sequence of characters (zero or more)
*`?' matches any character
*[SET] matches any character in the specified set,
*[!SET] or [^SET] matches any character not in the specified set.
*
* A set is composed of characters or ranges; a range looks like
* character hyphen character (as in 0-9 or A-Z). [0-9a-zA-Z_] is the
* minimal set of characters allowed in the [..] pattern construct.
* Other characters are allowed (ie. 8 bit characters) if your system
* will support them.
*
* To suppress the special syntactic significance of any of `[]*?!^-\',
* and match the character exactly, precede it with a `\'.
*
----------------------------------------------------------------------------*/
BOOLEAN match (char *pattern, char *text);
int matche(register char *pattern, register char *text);
/*----------------------------------------------------------------------------
*
* Return TRUE if PATTERN has any special wildcard characters
*
----------------------------------------------------------------------------*/
BOOLEAN is_pattern (char *pattern);
/*----------------------------------------------------------------------------
*
* Return TRUE if PATTERN has is a well formed regular expression according
* to the above syntax
*
* error_type is a return code based on the type of pattern error. Zero is
* returned in error_type if the pattern is a valid one. error_type return
* values are as follows:
*
*PATTERN_VALID - pattern is well formed
*PATTERN_ESC- pattern has invalid escape ('\' at end of pattern)
*PATTERN_RANGE - [..] construct has a no end range in a '-' pair (ie [a-])
*PATTERN_CLOSE - [..] construct has no end bracket (ie [abc-g )
*PATTERN_EMPTY - [..] construct is empty (ie [])
*
----------------------------------------------------------------------------*/
BOOLEAN is_valid_pattern (char *pattern, int *error_type);
#endif /* MATCH__H */

