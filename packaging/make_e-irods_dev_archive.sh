#!/bin/bash

ar_file="libe-irods.a"

rm $ar_file

touch dummy

ar cr $ar_file dummy
ar d  $ar_file dummy

rm dummy


for ff in `find iRODS/lib -name "*.o"` 
do
    ar r $ar_file $ff
done

for ff in `find iRODS/server/core -name "*.o"` 
do
    ar r $ar_file $ff
done

for ff in `find iRODS/server/re -name "*.o"` 
do
    ar r $ar_file $ff
done

for ff in `find iRODS/server/api -name "*.o"` 
do
    ar r $ar_file $ff
done



