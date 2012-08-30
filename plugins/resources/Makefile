GCC = g++ 
INC=-I ../../iRODS/lib/core/include/ -I ../../iRODS/lib/api/include/ -I../../iRODS/lib/md5/include -I../../iRODS/server/core/include/ -I../../iRODS/server/icat/include/ -I../../iRODS/server/re/include/ -I../../iRODS/server/drivers/include/
all:
	${GCC} ${INC} -Dlinux_platform -fPIC -shared -o libunixfilesystem.so  libunixfilesystem.cpp ../../iRODS/lib/core/obj/libRodsAPIs.a
