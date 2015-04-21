GCC = g++ 
INC=-I ../../../iRODS/lib/core/include/ -I ../../../iRODS/lib/api/include/ -I../../../iRODS/server/core/include/ -I../../../iRODS/server/icat/include/ -I../../../iRODS/server/re/include/ -I../../../iRODS/server/drivers/include/
all:
	${GCC} ${INC} -fPIC -shared -o libirods_msvc_test.so irods_msvc_test.cpp ../../../iRODS/lib/core/obj/libRodsAPIs.a
