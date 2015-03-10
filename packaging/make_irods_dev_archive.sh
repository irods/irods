#!/bin/bash -e

SCRIPT_DIR=${BASH_SOURCE%/*}
OS=`${SCRIPT_DIR}/find_os.sh`

if [ "${OS}" = "MacOSX" ]; then
    LD_ADD = ",-undefined,dynamic_lookup"
fi


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
CLIENT_VERSION_MAJOR=1
CLIENT_VERSION_MINOR=0
CLIENT_VERSION_RELEASE=0

CLIENT_SONAME="libirods_client.so.${CLIENT_VERSION_MAJOR}"
CLIENT_REAL_NAME="${CLIENT_SONAME}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_RELEASE}"

g++ -fPIC -shared -rdynamic "-Wl,-soname,${CLIENT_SONAME}${LD_ADD}" -o ${CLIENT_REAL_NAME} $(find iRODS/lib -name \*.o ! -name irods_pack_table.o ! -name irods_client_api_table.o ! -name irods_*_plugin.o ! -name irods_*_object.o )



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
# build shared object of all client api objects

CLIENT_API_VERSION_MAJOR=1
CLIENT_API_VERSION_MINOR=0
CLIENT_API_VERSION_RELEASE=0

CLIENT_API_SONAME="libirods_client_api.so.${CLIENT_API_VERSION_MAJOR}"
CLIENT_API_REAL_NAME="${CLIENT_API_SONAME}.${CLIENT_API_VERSION_MINOR}.${CLIENT_API_VERSION_RELEASE}"

g++ -fPIC -shared -rdynamic "-Wl,-soname,${CLIENT_API_SONAME}${LD_ADD}" -o ${CLIENT_API_REAL_NAME} $(find iRODS/lib -name irods_pack_table.o) $(find iRODS/lib -name irods_client_api_table.o) $(find iRODS/lib -name irods_*_object.o) $(find iRODS/lib -name irods_network_plugin.o) $(find iRODS/lib -name irods_auth_plugin.o)



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
SERVER_VERSION_MAJOR=1
SERVER_VERSION_MINOR=0
SERVER_VERSION_RELEASE=0

SERVER_SONAME="libirods_server.so.${SERVER_VERSION_MAJOR}"
SERVER_REAL_NAME="${SERVER_SONAME}.${SERVER_VERSION_MINOR}.${SERVER_VERSION_RELEASE}"

g++ -fPIC -shared -rdynamic "-Wl,-soname,${SERVER_SONAME}${LD_ADD}" -o ${SERVER_REAL_NAME} $(find iRODS/server -name \*.o ! -name \*Server.o ! -name \*_with_no_re.o ! -name \*Agent.o ! -name test_\*.o ! -name PamAuthCheck.o ! -name \*_manager.o )
