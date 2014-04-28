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
ar qS $ar_file $(find iRODS/lib -name \*.o ! -name irods_pack_table.o ! -name irods_client_api_table.o ! -name irods_*_plugin.o ! -name irods_*_object.o )

# =-=-=-=-=-=-=-
# Generate index
ranlib $ar_file

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
ar qS $ar_file $(find iRODS/server -name \*.o ! -name \*_manager.o ! -name irods_server_api_table.o )

# =-=-=-=-=-=-=-
# Generate index
ranlib $ar_file

