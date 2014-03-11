#!/bin/bash
# Script for irods i-commands auto-completion with bash
# This script is GPL, blah blah blah...
# Bruno Bzeznik <Bruno.Bzeznik@imag.fr> 10/2011
#  
# Simply source this script as follows:
#     . irods_completion.bash 
# and enjoy <tab> key
# Feel free to improve!
#

# Irods command to auto-complete
command_list="ibun icd ichksum ichmod icp iget ils imeta imkdir imv iphybun iphymv irm irmtrash irsync itrim iput"

# Completion function that gets the files list from irods
_ils() {
  local cur prev dirname basename base list
  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  # Set irods current directory (weird!!)
  export irodsCwd=$(ipwd)

  # Generate the list of irods files
  if [[ $cur == "" ]] ; then
    dirname=""
    basename=""
  elif [[ $cur == */ ]] ; then
    dirname=$cur
    basename=""
  else
    dirname="$(dirname ${cur})/"
    if [[ $dirname == "." ]] ; then dirname="" ; fi
    basename=$(basename ${cur})
  fi
  list=`ils ${dirname}| perl -ne "/^  / || next; s|^  ([^C])|\1|; s|^  C-.*/(.*)|\1/|; print;"`

  # Count the number of arguments that are not options
  # (that do not begin by a dash)
  # TODO
  # to be used in place of $COMP_CWORD into the following tests

  # Case of "iput", first arg is a local file
  if [ $1 = "iput" -a $COMP_CWORD -eq 1 ]; then
    COMPREPLY=( $(compgen -o default ${cur}) )

  # Case of "iget", second arg is a local file
  elif [ $1 = "iget" -a $COMP_CWORD -eq 2 ]; then
    COMPREPLY=( $(compgen -o default ${cur}) )

  # Case of "irsync", manage i: prefix
  elif [ $1 = "irsync" ]; then
    if [[ $cur == i:* ]]; then
      base=${cur:2}
      COMPREPLY=( $(compgen -W "$list \ " ${base} ) )
    else
      COMPREPLY=( $(compgen -P i: -W "$list \ " ${cur} ) )
      COMPREPLY+=( $(compgen -o default ${cur}) )
    fi

  # General case 
  else
    COMPREPLY=( $(compgen -P "$dirname" -W "$list" ${basename}) )
  fi
}

# Complete the specified commands
complete -o nospace -F _ils $command_list


