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
# archive all client objects
ar qs $ar_file $(find iRODS/lib -name \*.o ! -name irods_pack_table.o ! -name irods_client_api_table.o ! -name irods_*_plugin.o ! -name irods_*_object.o )

# =-=-=-=-=-=-=-
# Generate index
ranlib $ar_file

# =-=-=-=-=-=-=-
# build shared object of all client objects
so_file="libirods_client.so"
g++ -fPIC -shared -rdynamic "-Wl,-E" -o $so_file $(find iRODS/lib -name \*.o ! -name irods_pack_table.o ! -name irods_client_api_table.o ! -name irods_*_plugin.o ! -name irods_*_object.o )



# =-=-=-=-=-=-=-
# Build Client Library
ar_file="libirods_client_api.a"

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
ar qS $ar_file $(find iRODS/lib -name irods_pack_table.o)
ar qS $ar_file $(find iRODS/lib -name irods_client_api_table.o)
ar qS $ar_file $(find iRODS/lib -name irods_*_object.o)
ar qS $ar_file $(find iRODS/lib -name irods_*_plugin.o)

# =-=-=-=-=-=-=-
# Generate index
ranlib $ar_file

# =-=-=-=-=-=-=-
# build shared object of all client objects
so_file="libirods_client_api.so"
g++ -fPIC -shared -rdynamic "-Wl,-E" -o $so_file  $(find iRODS/lib -name irods_pack_table.o) $(find iRODS/lib -name irods_client_api_table.o) $(find iRODS/lib -name irods_*_object.o) $(find iRODS/lib -name irods_network_plugin.o) $(find iRODS/lib -name irods_auth_plugin.o)



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
ar qS $ar_file $(find iRODS/server -name \*.o ! -name irods_server_api_table.o ! -name \*_manager.o )

# =-=-=-=-=-=-=-
# Generate index
ranlib $ar_file

# =-=-=-=-=-=-=-
# build shared object of all server objects
so_file="libirods_server.so"
g++ -fPIC -shared -rdynamic "-Wl,-E" -o $so_file $(find iRODS/server -name \*.o ! -name \*Server.o ! -name \*_with_no_re.o ! -name \*Agent.o ! -name test_\*.o ! -name PamAuthCheck.o ! -name \*_manager.o )

