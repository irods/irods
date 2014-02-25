#!/bin/bash -e

# =-=-=-=-=-=-=-
# Build Client Library
ar_file="libirods_client.a"

# =-=-=-=-=-=-=-
# clean up any old files
rm -f $ar_file

# =-=-=-=-=-=-=-
# init the ar file
touch dummy
ar cqS $ar_file dummy
ar d  $ar_file dummy
rm dummy

# =-=-=-=-=-=-=-
# Archive all Client Objects 
ar qS $ar_file $(find iRODS/lib -name \*.o )

# =-=-=-=-=-=-=-
# Generate index
ranlib $ar_file





# =-=-=-=-=-=-=-
# Build Server Library
ar_file="libirods_server.a"

# =-=-=-=-=-=-=-
# clean up any old files
rm -f $ar_file

# =-=-=-=-=-=-=-
# init the ar file
touch dummy
ar cqS $ar_file dummy
ar d  $ar_file dummy
rm dummy

# =-=-=-=-=-=-=-
# Archive all Server Objects, except the managers
ar qS $ar_file $(find iRODS/server -name \*.o ! -name \*_manager.o )

# =-=-=-=-=-=-=-
# Generate index
ranlib $ar_file

















#ar_file="libirods.a"

#rm -f $ar_file

#touch dummy

#ar cqS $ar_file dummy
#ar d  $ar_file dummy

#rm dummy

#ar r $ar_file iRODS/lib/core/obj/irods_plugin_base.o
#ar r $ar_file iRODS/lib/core/obj/irods_kvp_string_parser.o
#ar r $ar_file iRODS/server/re/obj/irods_ms_plugin.o
#ar r $ar_file iRODS/lib/core/obj/irods_resource_plugin.o
#ar r $ar_file iRODS/lib/core/obj/irods_network_plugin.o
#ar r $ar_file iRODS/lib/core/obj/irods_auth_plugin.o

#for ff in `find iRODS/ -name "*.o"`
#do
#    ar qS $ar_file $ff
#done

# Archive all objects except for managers
#ar qS $ar_file $(find iRODS/ -name \*.o ! -name \*_manager.o )
#ar qS $ar_file iRODS/lib/core/obj/irods_auth_manager.o
#ar qS $ar_file iRODS/lib/core/obj/irods_network_manager.o

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

# Generate index
#ranlib $ar_file


