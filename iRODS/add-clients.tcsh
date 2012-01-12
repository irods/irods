#!/bin/tcsh
#===============================================================================
#
#          FILE:  add-clients.tcsh
# 
#         USAGE:  ./add-clients.tcsh 
# 
#   DESCRIPTION:  Script to add icommand client tools to the path
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR:  Adil Hasan
#       COMPANY:  University of Liverpool
#       VERSION:  1.0
#       CREATED:  09/03/09 17:36:10 GMT
#      REVISION:  ---
#===============================================================================
echo "Adding icommands to PATH"
set apath = $PWD"/clients/icommands/bin"
set path = ($apath $path)
echo "Path now is: "$path

