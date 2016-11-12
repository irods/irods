#!/bin/bash -e


IRODSVERSION=$1
BUILDDIR=$2
ICMD=$3
MANDIR=$4

#grep is ggrep on solaris
if [ "$DETECTEDOS" == "Solaris" ] ; then
    GREPCMD="ggrep"
else
    GREPCMD="grep"
fi

HELP2MAN=`which help2man`
if [[ "$?" != "0" || `echo $HELP2MAN | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    else
        PREFLIGHTDOWNLOAD=$PREFLIGHTDOWNLOAD$'\n'"      :: download from: http://www.gnu.org/software/help2man/"
        PREFLIGHTDOWNLOAD=$PREFLIGHTDOWNLOAD$'\n'"      ::                http://mirrors.kernel.org/gnu/help2man/"
    fi
else
    H2MVERSION=`help2man --version | head -n1 | awk '{print $3}'`
fi

# prepare man pages for the icommand
cd $BUILDDIR
mkdir -p $MANDIR
if [ "$H2MVERSION" \< "1.37" ] ; then
    echo "NOTE :: Skipping man page generation -- help2man version needs to be >= 1.37"
    echo "     :: (or, add --version capability to all iCommands)"
    echo "     :: (installed here: help2man version $H2MVERSION)"
else
    if [ -f $MANDIR/$ICMD.1 ] ; then
        rm $MANDIR/$ICMD.1
    fi
    if [ -f $MANDIR/$ICMD.1.gz ] ; then
        rm $MANDIR/$ICMD.1.gz
    fi
    help2man -h -h -N -n "an iRODS iCommand" --version-string="iRODS-$IRODSVERSION" $BUILDDIR/$ICMD > $MANDIR/$ICMD.1
    gzip -9 $MANDIR/$ICMD.1
fi
