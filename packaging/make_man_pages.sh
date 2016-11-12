#!/bin/bash

IRODSVERSION=$1
BUILDDIR=$2
MANDIR="man"

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
    echo "Detected help2man [$HELP2MAN] v[$H2MVERSION]"
fi

# prepare man pages for the icommands
cd $BUILDDIR
rm -rf $MANDIR
mkdir -p $MANDIR
if [ "$H2MVERSION" \< "1.37" ] ; then
    echo "NOTE :: Skipping man page generation -- help2man version needs to be >= 1.37"
    echo "     :: (or, add --version capability to all iCommands)"
    echo "     :: (installed here: help2man version $H2MVERSION)"
else
#    IRODSMANVERSION=`$GREPCMD "find_package(IRODS" "../CMakeLists.txt" | awk '{print $2}'`
#    ICMDDIR="/home/rskarbez/build/icommands-rip"
    ICMDS=(
    iadmin
    ibun
    icd
    ichksum
    ichmod
    icp
    idbug
    ienv
    ierror
    iexecmd
    iexit
    ifsck
    iget
    igetwild
    igroupadmin
    ihelp
    iinit
    ilocate
    ils
    ilsresc
    imcoll
    imeta
    imiscsvrinfo
    imkdir
    imv
    ipasswd
    iphybun
    iphymv
    ips
    iput
    ipwd
    iqdel
    iqmod
    iqstat
    iquest
    iquota
    ireg
    irepl
    irm
    irmtrash
    irsync
    irule
    iscan
    isysmeta
    iticket
    itrim
    iuserinfo
    ixmsg
    izonereport
    )
    for ICMD in "${ICMDS[@]}"
    do
        help2man -h -h -N -n "an iRODS iCommand" --version-string="iRODS-$IRODSVERSION" $BUILDDIR/$ICMD > $MANDIR/$ICMD.1
    done
    for manfile in `ls $MANDIR`
    do
        gzip -9 $MANDIR/$manfile
    done
fi
