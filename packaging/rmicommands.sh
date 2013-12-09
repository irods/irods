#!/bin/sh
############################################################
#
#  uninstaller for irods-icommands package
#
############################################################
set -e
SCRIPTNAME=`basename $0`
if [ "$(id -u)" != "0" ]; then
    echo "ERROR :: $SCRIPTNAME must be run as root" 1>&2
    exit 1
fi

# icommands
rm -f /usr/bin/ixmsg
rm -f /usr/bin/iuserinfo
rm -f /usr/bin/itrim
rm -f /usr/bin/isysmeta
rm -f /usr/bin/iscan
rm -f /usr/bin/irule
rm -f /usr/bin/irsync
rm -f /usr/bin/irmtrash
rm -f /usr/bin/irm
rm -f /usr/bin/irepl
rm -f /usr/bin/ireg
rm -f /usr/bin/iquota
rm -f /usr/bin/iquest
rm -f /usr/bin/iqstat
rm -f /usr/bin/iqmod
rm -f /usr/bin/iqdel
rm -f /usr/bin/ipwd
rm -f /usr/bin/iput
rm -f /usr/bin/ips
rm -f /usr/bin/iphymv
rm -f /usr/bin/iphybun
rm -f /usr/bin/ipasswd
rm -f /usr/bin/imv
rm -f /usr/bin/imkdir
rm -f /usr/bin/imiscsvrinfo
rm -f /usr/bin/imeta
rm -f /usr/bin/imcoll
rm -f /usr/bin/ilsresc
rm -f /usr/bin/ils
rm -f /usr/bin/ilocate
rm -f /usr/bin/iinit
rm -f /usr/bin/ihelp
rm -f /usr/bin/igroupadmin
rm -f /usr/bin/igetwild
rm -f /usr/bin/iget
rm -f /usr/bin/ifsck
rm -f /usr/bin/iexit
rm -f /usr/bin/iexecmd
rm -f /usr/bin/ierror
rm -f /usr/bin/ienv
rm -f /usr/bin/idbug
rm -f /usr/bin/idbo
rm -f /usr/bin/icp
rm -f /usr/bin/ichmod
rm -f /usr/bin/ichksum
rm -f /usr/bin/icd
rm -f /usr/bin/ibun
rm -f /usr/bin/iadmin

# manpages
rm -f /usr/share/man/man1/ixmsg.1.gz
rm -f /usr/share/man/man1/iuserinfo.1.gz
rm -f /usr/share/man/man1/itrim.1.gz
rm -f /usr/share/man/man1/isysmeta.1.gz
rm -f /usr/share/man/man1/iscan.1.gz
rm -f /usr/share/man/man1/irule.1.gz
rm -f /usr/share/man/man1/irsync.1.gz
rm -f /usr/share/man/man1/irmtrash.1.gz
rm -f /usr/share/man/man1/irm.1.gz
rm -f /usr/share/man/man1/irepl.1.gz
rm -f /usr/share/man/man1/ireg.1.gz
rm -f /usr/share/man/man1/iquota.1.gz
rm -f /usr/share/man/man1/iquest.1.gz
rm -f /usr/share/man/man1/iqstat.1.gz
rm -f /usr/share/man/man1/iqmod.1.gz
rm -f /usr/share/man/man1/iqdel.1.gz
rm -f /usr/share/man/man1/ipwd.1.gz
rm -f /usr/share/man/man1/iput.1.gz
rm -f /usr/share/man/man1/ips.1.gz
rm -f /usr/share/man/man1/iphymv.1.gz
rm -f /usr/share/man/man1/iphybun.1.gz
rm -f /usr/share/man/man1/ipasswd.1.gz
rm -f /usr/share/man/man1/imv.1.gz
rm -f /usr/share/man/man1/imkdir.1.gz
rm -f /usr/share/man/man1/imiscsvrinfo.1.gz
rm -f /usr/share/man/man1/imeta.1.gz
rm -f /usr/share/man/man1/imcoll.1.gz
rm -f /usr/share/man/man1/ilsresc.1.gz
rm -f /usr/share/man/man1/ils.1.gz
rm -f /usr/share/man/man1/ilocate.1.gz
rm -f /usr/share/man/man1/iinit.1.gz
rm -f /usr/share/man/man1/ihelp.1.gz
rm -f /usr/share/man/man1/igroupadmin.1.gz
rm -f /usr/share/man/man1/igetwild.1.gz
rm -f /usr/share/man/man1/iget.1.gz
rm -f /usr/share/man/man1/ifsck.1.gz
rm -f /usr/share/man/man1/iexit.1.gz
rm -f /usr/share/man/man1/iexecmd.1.gz
rm -f /usr/share/man/man1/ierror.1.gz
rm -f /usr/share/man/man1/ienv.1.gz
rm -f /usr/share/man/man1/idbug.1.gz
rm -f /usr/share/man/man1/idbo.1.gz
rm -f /usr/share/man/man1/icp.1.gz
rm -f /usr/share/man/man1/ichmod.1.gz
rm -f /usr/share/man/man1/ichksum.1.gz
rm -f /usr/share/man/man1/icd.1.gz
rm -f /usr/share/man/man1/ibun.1.gz
rm -f /usr/share/man/man1/iadmin.1.gz
