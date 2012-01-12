#!/bin/sh -e
i=1
resc1=demoResc
resc2=nvoReplResc
../bin/iinit
../bin/imkdir repeatiPutTest
../bin/icd repeatiPutTest
rm -f ../test/repeatTest.txt
../bin/irm -f repeatIputTest.txt
touch ../test/repeatTest.txt

../bin/iput -f -R $resc1 ../test/repeatTest.txt repeatIputTest.txt
../bin/irepl -R $resc2  repeatIputTest.txt
while [ $i -lt $1 ]
do

   ../bin/iput -f -R $resc1 ../test/repeatTest.txt repeatIputTest.txt
   ../bin/ichksum -a repeatIputTest.txt
   ../bin/ils -L repeatIputTest.txt
   i=`expr $i +  1`
   sleep 5

   ../bin/iput -f -R $resc2 ../test/repeatTest.txt repeatIputTest.txt
   ../bin/ichksum -a repeatIputTest.txt
   ../bin/ils -L repeatIputTest.txt
   cat ../test/addRepeatTest.txt >> ../test/repeatTest.txt
   i=`expr $i +  1`
   sleep 5
done
echo "Done"
exit 0
