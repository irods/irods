#!/bin/bash -e

ar_file="libeirods.a"

rm -f $ar_file

touch dummy

ar cr $ar_file dummy
ar d  $ar_file dummy

rm dummy

ar r $ar_file iRODS/lib/core/obj/eirods_plugin_base.o
ar r $ar_file iRODS/lib/core/obj/eirods_kvp_string_parser.o


#for ff in `find iRODS/ -name "*.o"`
#do
#    ar r $ar_file $ff
#done

#for ff in `find iRODS/lib -name "*.o"`
#do
#    ar r $ar_file $ff
#done
#
#for ff in `find iRODS/server/core -name "*.o"`
#do
#    ar r $ar_file $ff
#done
#
#for ff in `find iRODS/server/re -name "*.o"`
#do
#    ar r $ar_file $ff
#done
#
#for ff in `find iRODS/server/api -name "*.o"`
#do
#    ar r $ar_file $ff
#done
#
#for ff in `find iRODS/server/icat -name "*.o"`
#do
#    ar r $ar_file $ff
#done
#



