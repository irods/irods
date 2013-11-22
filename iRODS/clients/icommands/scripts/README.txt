This directory contains some additional scripts that can be used to
supplement the functionality of the i-commands.  These are bash
scripts that run the i-commands.  To make them available, you can move
them into the clients/bin directory with the other i-commands,
changing the name if you wish.

Currently, these are some ticket-related scripts: iTicket, iGet, and
iDel, which work-around some ticket limitations when working with
directory trees.  The idea is to generate a ticket for each
subdirectory and use them instead of the top-level collection ticket
when needed.  See the scripts and their help text for more.  Also see
the irod-chat thread 'Problem with iticket for iget subdirectories.'
Thanks to Giacomo Mariani of the SuperComputing Applications and
Innovation Department CINECA, Bologna, Italy, for these.
