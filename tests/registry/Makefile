TOPDIR=../..
GCC = g++ 
INC=-I$(TOPDIR)/iRODS/lib/core/include/ -I$(TOPDIR)/iRODS/lib/api/include/ -I$(TOPDIR)/iRODS/lib/md5/include -I$(TOPDIR)/iRODS/server/core/include/ -I$(TOPDIR)/iRODS/server/icat/include/ -I$(TOPDIR)/iRODS/server/re/include/ -I$(TOPDIR)/iRODS/server/drivers/include/ -I$(TOPDIR)/iRODS/lib/hasher/include
LIB=-lssl $(TOPDIR)/iRODS/lib/core/obj/libRodsAPIs.a
all:
	${GCC} -g ${INC} -o hasher_test HasherTest.cpp ${LIB}
