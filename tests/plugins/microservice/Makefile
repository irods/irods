GCC = g++ -std=c++0x
INC=-I ../../../iRODS/lib/core/include/ -I ../../../iRODS/lib/api/include/ -I../../../iRODS/lib/md5/include -I../../../iRODS/server/core/include/ -I../../../iRODS/server/icat/include/ -I../../../iRODS/server/re/include/ -I../../../iRODS/server/drivers/include/
all:
	${GCC} ${INC} -fPIC -shared -o libeirods_msvc_test.so eirods_msvc_test.cpp ../../../iRODS/lib/core/obj/libRodsAPIs.a
