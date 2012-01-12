#!/bin/csh -ex
## PLEASE RUN THIS IN icommands/test
unalias rm
cd ../bin
rm -f irtest.txt
rm -fr Vault8
rm -f dir1.tar
rm -f lfoo1 lfoo2
iinit
ienv
ils -l
ilsresc -l 
ilsresc
mkdir $PWD/Vault8
iadmin mkresc demoResc8 "unix file system" cache $HOST  $PWD/Vault8
ilsresc -l demoResc8
ls -l>irtest.txt
ls -l
iput irtest.txt
ils -l irtest.txt
ils -L irtest.txt
ichksum irtest.txt
ils -L irtest.txt
ichksum -K irtest.txt
ils -A irtest.txt
ichmod read demo irtest.txt
ils -A irtest.txt
ichmod null demo irtest.txt
ils -A irtest.txt
imkdir testdir
ils testdir
ils -L irtest.txt
imv irtest.txt   testdir
ils -L testdir
iphymv -R demoResc8 testdir/irtest.txt
ils -L testdir
ipwd
icd testdir
ipwd
icd
ipwd
irm -r testdir
ils
ils  /tempZone/trash/home/rods
ils  /tempZone/trash/home/rods/testdir
ils -L /tempZone/trash/home/rods/testdir
ireg -R demoResc8 $PWD/irtest.txt  /tempZone/home/rods/irtest.txt
ils -L irtest.txt
irepl -R demoResc irtest.txt
ils -L irtest.txt
itrim  -S demoResc irtest.txt
ils -L irtest.txt
itrim -N  2 -S demoResc irtest.txt
ils -L irtest.txt
itrim -N  1 -S demoResc irtest.txt
ils -L irtest.txt
iquest 
iquest "SELECT DATA_NAME, DATA_CHECKSUM WHERE DATA_RESC_NAME like 'demo%'"
iquest "SELECT DATA_NAME, DATA_CHECKSUM, DATA_RESC_NAME WHERE DATA_RESC_NAME like 'demo%'" 
iquest "SELECT DATA_NAME, DATA_CHECKSUM, DATA_RESC_NAME WHERE DATA_RESC_NAME like 'demo%8'"
ierror 1000
imiscsvrinfo
ienv
iuserinfo rods
iexecmd hello
irule -F showCore.ir
cat i*>lfoo1
cat lfoo1 lfoo1 lfoo1 lfoo1 lfoo1 lfoo1 lfoo1 lfoo1 lfoo1 lfoo1>lfoo2
ls -l lfoo1 lfoo2
iput -v lfoo1
iput -vR demoResc8  lfoo2
ils -l 
iget -v lfoo2 lfoo3
irm lfoo1 lfoo2
ils -lr /tempZone/trash/home/rods
irmtrash
ils -lr /tempZone/trash/home/rods
iput -vK lfoo1
ils -L
iget -vK lfoo1 lfoo4
md5sum lfoo4
rm lfoo3  lfoo4
ls -lR ../bin
iput -vr ../bin dir1
ils -lr dir1
irepl -vrRdemoResc8 dir1
ils -Lr dir1
iget -rv dir1 dir2
diff -r ../bin dir2 &
sleep 2
iget dir1/irtest.txt - | wc
cat irtest.txt >> dir2/irtest.txt
irsync -vr dir2 i:dir1
iget dir1/irtest.txt - | wc
rm -r dir2
icp -vr dir1 dir3
ils -l
ils -lr dir3
icd dir1
ils -l
imeta help
imeta add -d iquest color red
imeta ls -d iquest
imeta qu -d color = red
imeta add -d irepl length 20 inches
imeta ls -d irepl
imeta qu -d length ">" 15 inches
imeta qu -d length ">" 25 inches
icd
tar -chlf dir1.tar ../bin
imkdir tarfiles
iput dir1.tar tarfiles
ils -l tarfiles
imkdir /tempZone/home/rods/tarmnt
imcoll -m tar /tempZone/home/rods/tarfiles/dir1.tar /tempZone/home/rods/tarmnt
ils -L
icd /tempZone/home/rods/tarmnt/bin
ils
iget irtest.txt - 
icd
imcoll -p /tempZone/home/rods/tarmnt
imcoll -U /tempZone/home/rods/tarmnt
irm -rf /tempZone/home/rods/tarmnt
irm -rf tarfiles
irm irtest.txt
irm -rf dir3
irm -f lfoo1
irm -rf dir1
irmtrash
iadmin rmresc demoResc8
rm -f dir1.tar
rm -f irtest.txt
rm -fr ./Vault8
rm -f lfoo1 lfoo2
iexit
exit
