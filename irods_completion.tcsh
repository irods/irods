#!/bin/tcsh

################################################################################
## iRODS tcsh completion for standard icommands and associated scripts
## Originally written by Syam Gadde for SRB.
## Adapted for the csh and tcsh shells by Jean-Yves Nief 29/03/10.
##
## To enable your tcsh shell to support iRODS <tab> completion for icommands
## simply source this file in your shell as follows:
##
##    source irods_completion.tcsh
##
## You can add the above line to your .tcshrc or .cshrc file to enable iRODS
## <tab> completion for all of your tcsh logins.
##
################################################################################

## list commands which should use iRODS lookups for completions
set command_list="ibun icd ichksum ichmod icp iget ils imeta imkdir imv iphybun iphymv iput irm irmtrash irsync itrim"

## Enable ALL listed commands to use iRODS completion
foreach command ($command_list)
  complete $command 'c%*/%`set q=$:-0; echo $q | perl -pe '"'"'s|^/[^/]+$|/|; s|(.)/[^/]*$|\1|;'"'"' | xargs ils | perl -ne "/^  / || next; s|^  \([^C]\)|\1|; s|^  C-.*/||; print;"`%%'
end
