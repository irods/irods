#!/bin/bash -e

SUFFIX=md

DETECTEDDIR=$( cd $(dirname $0) ; pwd -P )

TARGETDIR=$DETECTEDDIR/icommands
mkdir -p $TARGETDIR

TARGETFILE=$TARGETDIR/user.$SUFFIX
echo > $TARGETFILE
echo "# iCommands - User" >> $TARGETFILE

ihelp -a | grep "iRODS Version " | awk '{print $6}' | while read x ; do

  if [ "$x" = "iadmin" ] ; then
    # iadmin has subcommands
    TARGETFILE=$TARGETDIR/administrator.$SUFFIX
    echo > $TARGETFILE
    echo "# iCommands - Administrator" >> $TARGETFILE
    echo "# $x" >> $TARGETFILE
    echo "<pre>" >> $TARGETFILE
    ihelp $x | sed \$d >> $TARGETFILE
    echo "</pre>" >> $TARGETFILE
    echo >> $TARGETFILE
    ihelp $x | grep "^ " | grep -v "^     " | awk '{print $1}' | while read y ; do
      echo "## $y" >> $TARGETFILE
      echo "<pre>" >> $TARGETFILE
      $x h $y >> $TARGETFILE
      echo "</pre>" >> $TARGETFILE
      echo >> $TARGETFILE
    done
  elif [ "$x" = "imeta" ] ; then
    # imeta has subcommands
    TARGETFILE=$TARGETDIR/metadata.$SUFFIX
    echo > $TARGETFILE
    echo "# iCommands - Metadata" >> $TARGETFILE
    echo "# $x" >> $TARGETFILE
    echo "<pre>" >> $TARGETFILE
    ihelp $x | sed \$d >> $TARGETFILE
    echo "</pre>" >> $TARGETFILE
    echo >> $TARGETFILE
    ihelp $x | grep "^ " | grep -v "^ -" | grep -v " $" | grep -v "^     " | awk '{print $1}' | while read y ; do
      echo "## $y" >> $TARGETFILE
      echo "<pre>" >> $TARGETFILE
      $x h $y >> $TARGETFILE
      echo "</pre>" >> $TARGETFILE
      echo >> $TARGETFILE
    done
  elif [ "$x" = "igroupadmin" ] ; then
    # igroupadmin has subcommands
    TARGETFILE=$TARGETDIR/groupadmin.$SUFFIX
    echo > $TARGETFILE
    echo "# iCommands - Group Admin" >> $TARGETFILE
    echo "# $x" >> $TARGETFILE
    echo "<pre>" >> $TARGETFILE
    ihelp $x | sed \$d >> $TARGETFILE
    echo "</pre>" >> $TARGETFILE
    echo >> $TARGETFILE
    ihelp $x | grep "^ " | grep -v " $" | awk '{print $1}' | while read y ; do
      echo "## $y" >> $TARGETFILE
      echo "<pre>" >> $TARGETFILE
      $x h $y >> $TARGETFILE
      echo "</pre>" >> $TARGETFILE
      echo >> $TARGETFILE
    done
  elif [ "$x" = "iticket" ] ; then
    # iticket has subcommands
    TARGETFILE=$TARGETDIR/tickets.$SUFFIX
    echo > $TARGETFILE
    echo "# iCommands - Tickets" >> $TARGETFILE
    echo "# $x" >> $TARGETFILE
    echo "<pre>" >> $TARGETFILE
    ihelp $x | sed \$d >> $TARGETFILE
    echo "</pre>" >> $TARGETFILE
    echo >> $TARGETFILE
    ihelp $x | grep "^ " | grep -v "^ -" | grep -v " $" | grep -v "^     " | awk '{print $1}' | uniq | while read y ; do
      echo "## $y" >> $TARGETFILE
      echo "<pre>" >> $TARGETFILE
      $x h $y >> $TARGETFILE
      echo "</pre>" >> $TARGETFILE
      echo >> $TARGETFILE
    done
  else
    # all other icommands
    TARGETFILE=$TARGETDIR/user.$SUFFIX
    echo "# $x" >> $TARGETFILE
    echo "<pre>" >> $TARGETFILE
    ihelp $x | sed \$d >> $TARGETFILE
    echo "</pre>" >> $TARGETFILE
    echo >> $TARGETFILE
  fi

done
