#!/bin/bash
#===============================================================================
#
#          FILE:  add-clients.sh
# 
#         USAGE:  ./add-clients.sh 
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
apath=$PWD"/clients/icommands/bin"
export PATH=$apath:${PATH}
echo "Path now is: "$PATH

