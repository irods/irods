#!/bin/bash -e

IRODS_HOME_DIR=$1
OS_IRODS_ACCT=$2
SERVER_TYPE=$3

IRODS_HOME=$IRODS_HOME_DIR/iRODS

# =-=-=-=-=-=-=-
# detect whether this is an upgrade
UPGRADE_FLAG_FILE=$IRODS_HOME_DIR/upgrade.tmp
if [ -f "$UPGRADE_FLAG_FILE" ] ; then
    UPGRADE_FLAG="true"
else
    UPGRADE_FLAG="false"
fi

# =-=-=-=-=-=-=-
# debugging
#echo "UPGRADE_FLAG=[$UPGRADE_FLAG]"
#echo "IRODS_HOME_DIR=[$IRODS_HOME_DIR]"
#echo "OS_IRODS_ACCT=[$OS_IRODS_ACCT]"
#echo "SERVER_TYPE=[$SERVER_TYPE]"

# =-=-=-=-=-=-=-
# detect operating system
DETECTEDOS=`$IRODS_HOME_DIR/packaging/find_os.sh`

# =-=-=-=-=-=-=-
# clean up any stray iRODS files in /tmp which will cause problems
if [ -f /tmp/irodsServer.* ] ; then
    rm /tmp/irodsServer.*
fi

# =-=-=-=-=-=-=-
# explode tarball of necessary coverage files if it exists
if [ -f "$IRODS_HOME_DIR/gcovfiles.tgz" ] ; then
    cd $IRODS_HOME_DIR
    tar xzf gcovfiles.tgz
fi

# =-=-=-=-=-=-=-
# set permissions on the installed files
chown -R $OS_IRODS_ACCT:$OS_IRODS_ACCT $IRODS_HOME

# =-=-=-=-=-=-=-
# setup runlevels and aliases (use os-specific tools)
if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    update-rc.d irods defaults
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    /sbin/chkconfig --add irods
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    /sbin/chkconfig --add irods
fi

# =-=-=-=-=-=-=-
# symlink the icommands

#ln -s    /usr/bin/chgCoreToCore1.ir       ${IRODS_HOME}/clients/icommands/bin/chgCoreToCore1.ir 
#ln -s    /usr/bin/chgCoreToCore2.ir       ${IRODS_HOME}/clients/icommands/bin/chgCoreToCore2.ir 
#ln -s    /usr/bin/chgCoreToOrig.ir        ${IRODS_HOME}/clients/icommands/bin/chgCoreToOrig.ir  
#ln -s    /usr/bin/delUnusedAVUs.ir        ${IRODS_HOME}/clients/icommands/bin/delUnusedAVUs.ir  
#ln -s    /usr/bin/runQuota.ir             ${IRODS_HOME}/clients/icommands/bin/runQuota.ir       
#ln -s    /usr/bin/runQuota.r              ${IRODS_HOME}/clients/icommands/bin/runQuota.r        
#ln -s    /usr/bin/showCore.ir             ${IRODS_HOME}/clients/icommands/bin/showCore.ir       

ln -fs    /usr/bin/genOSAuth               ${IRODS_HOME}/clients/icommands/bin/genOSAuth         
ln -fs    /usr/bin/iadmin                  ${IRODS_HOME}/clients/icommands/bin/iadmin            
ln -fs    /usr/bin/ibun                    ${IRODS_HOME}/clients/icommands/bin/ibun              
ln -fs    /usr/bin/icd                     ${IRODS_HOME}/clients/icommands/bin/icd               
ln -fs    /usr/bin/ichksum                 ${IRODS_HOME}/clients/icommands/bin/ichksum           
ln -fs    /usr/bin/ichmod                  ${IRODS_HOME}/clients/icommands/bin/ichmod            
ln -fs    /usr/bin/icp                     ${IRODS_HOME}/clients/icommands/bin/icp               
ln -fs    /usr/bin/idbug                   ${IRODS_HOME}/clients/icommands/bin/idbug             
ln -fs    /usr/bin/ienv                    ${IRODS_HOME}/clients/icommands/bin/ienv              
ln -fs    /usr/bin/ierror                  ${IRODS_HOME}/clients/icommands/bin/ierror            
ln -fs    /usr/bin/iexecmd                 ${IRODS_HOME}/clients/icommands/bin/iexecmd           
ln -fs    /usr/bin/iexit                   ${IRODS_HOME}/clients/icommands/bin/iexit             
ln -fs    /usr/bin/ifsck                   ${IRODS_HOME}/clients/icommands/bin/ifsck             
ln -fs    /usr/bin/iget                    ${IRODS_HOME}/clients/icommands/bin/iget              
ln -fs    /usr/bin/igetwild                ${IRODS_HOME}/clients/icommands/bin/igetwild          
ln -fs    /usr/bin/igroupadmin             ${IRODS_HOME}/clients/icommands/bin/igroupadmin       
ln -fs    /usr/bin/ihelp                   ${IRODS_HOME}/clients/icommands/bin/ihelp             
ln -fs    /usr/bin/iinit                   ${IRODS_HOME}/clients/icommands/bin/iinit             
ln -fs    /usr/bin/ilocate                 ${IRODS_HOME}/clients/icommands/bin/ilocate           
ln -fs    /usr/bin/ils                     ${IRODS_HOME}/clients/icommands/bin/ils               
ln -fs    /usr/bin/ilsresc                 ${IRODS_HOME}/clients/icommands/bin/ilsresc           
ln -fs    /usr/bin/imcoll                  ${IRODS_HOME}/clients/icommands/bin/imcoll            
ln -fs    /usr/bin/imeta                   ${IRODS_HOME}/clients/icommands/bin/imeta             
ln -fs    /usr/bin/imiscsvrinfo            ${IRODS_HOME}/clients/icommands/bin/imiscsvrinfo      
ln -fs    /usr/bin/imkdir                  ${IRODS_HOME}/clients/icommands/bin/imkdir            
ln -fs    /usr/bin/imv                     ${IRODS_HOME}/clients/icommands/bin/imv               
ln -fs    /usr/bin/ipasswd                 ${IRODS_HOME}/clients/icommands/bin/ipasswd           
ln -fs    /usr/bin/iphybun                 ${IRODS_HOME}/clients/icommands/bin/iphybun           
ln -fs    /usr/bin/iphymv                  ${IRODS_HOME}/clients/icommands/bin/iphymv            
ln -fs    /usr/bin/ips                     ${IRODS_HOME}/clients/icommands/bin/ips               
ln -fs    /usr/bin/iput                    ${IRODS_HOME}/clients/icommands/bin/iput              
ln -fs    /usr/bin/ipwd                    ${IRODS_HOME}/clients/icommands/bin/ipwd              
ln -fs    /usr/bin/iqdel                   ${IRODS_HOME}/clients/icommands/bin/iqdel             
ln -fs    /usr/bin/iqmod                   ${IRODS_HOME}/clients/icommands/bin/iqmod             
ln -fs    /usr/bin/iqstat                  ${IRODS_HOME}/clients/icommands/bin/iqstat            
ln -fs    /usr/bin/iquest                  ${IRODS_HOME}/clients/icommands/bin/iquest            
ln -fs    /usr/bin/iquota                  ${IRODS_HOME}/clients/icommands/bin/iquota            
ln -fs    /usr/bin/ireg                    ${IRODS_HOME}/clients/icommands/bin/ireg              
ln -fs    /usr/bin/irepl                   ${IRODS_HOME}/clients/icommands/bin/irepl             
ln -fs    /usr/bin/irm                     ${IRODS_HOME}/clients/icommands/bin/irm               
ln -fs    /usr/bin/irmtrash                ${IRODS_HOME}/clients/icommands/bin/irmtrash          
ln -fs    /usr/bin/irsync                  ${IRODS_HOME}/clients/icommands/bin/irsync            
ln -fs    /usr/bin/irule                   ${IRODS_HOME}/clients/icommands/bin/irule             
ln -fs    /usr/bin/iscan                   ${IRODS_HOME}/clients/icommands/bin/iscan             
ln -fs    /usr/bin/isysmeta                ${IRODS_HOME}/clients/icommands/bin/isysmeta          
ln -fs    /usr/bin/iticket                 ${IRODS_HOME}/clients/icommands/bin/iticket           
ln -fs    /usr/bin/itrim                   ${IRODS_HOME}/clients/icommands/bin/itrim             
ln -fs    /usr/bin/iuserinfo               ${IRODS_HOME}/clients/icommands/bin/iuserinfo         
ln -fs    /usr/bin/ixmsg                   ${IRODS_HOME}/clients/icommands/bin/ixmsg             



# =-=-=-=-=-=-=-
# really make sure everything is owned by the irods service account
chown -R $OS_IRODS_ACCT:$OS_IRODS_ACCT $IRODS_HOME_DIR

# =-=-=-=-=-=-=-
# ensure the config directory is owned by irods
chown -R  $OS_IRODS_ACCT:$OS_IRODS_ACCT /etc/irods

# =-=-=-=-=-=-=-
# set permissions on iRODS authentication mechanisms
if [ "$DETECTEDOS" == "MacOSX" ] ; then
    chown root:wheel $IRODS_HOME/server/bin/PamAuthCheck
else
    chown root:root $IRODS_HOME/server/bin/PamAuthCheck
fi
chmod 4755 $IRODS_HOME/server/bin/PamAuthCheck
chmod 4755 /usr/bin/genOSAuth

# =-=-=-=-=-=-=-
# remove the password from the service account
if [ "$DETECTEDOS" != "MacOSX" ] ; then
    passwd -d $OS_IRODS_ACCT > /dev/null
fi

# =-=-=-=-=-=-=-
# display helpful information
if [ "$UPGRADE_FLAG" == "false" ] ; then
    # =-=-=-=-=-=-=-
    if [ "$SERVER_TYPE" == "icat" ] ; then
        # tell user about their irodsenv
#        cat $IRODS_HOME_DIR/packaging/user_irodsenv.txt
        # give user some guidance regarding database selection and installation
        cat $IRODS_HOME_DIR/packaging/user_icat.txt
    elif [ "$SERVER_TYPE" == "resource" ] ; then
        # give user some guidance regarding resource configuration
        cat $IRODS_HOME_DIR/packaging/user_resource.txt
    fi
fi

# =-=-=-=-=-=-=-
# exit with success
exit 0

