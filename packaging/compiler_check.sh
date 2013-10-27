#!/bin/bash

ver=`g++ --version | grep g++ | cut -d' ' -f4`

major=`echo $ver | cut -d'.' -f1`
middle=`echo $ver | cut -d'.' -f2`
minor=`echo $ver | cut -d'.' -f3`

#echo $major
major_good="NO"
if [ $major -ge $1 ]; then 
    major_good="YES"; 
fi
#echo $major_good

#echo $middle
middle_good="NO"
if [ $middle -ge $2 ]; then 
    middle_good="YES"; 
fi
#echo $middle_good

if [ "$major_good"  == "YES" ] &&
   [ "$middle_good" == "YES" ]; 
   then
       echo "-std=c++0x"
fi
