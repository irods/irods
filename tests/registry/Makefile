include ../../iRODS/config/config.mk
TOPDIR=../..
GCC = g++ 
INC=-I$(TOPDIR)/iRODS/lib/core/include/ -I$(TOPDIR)/iRODS/lib/api/include/ -I$(TOPDIR)/iRODS/server/core/include/ -I$(TOPDIR)/iRODS/server/icat/include/ -I$(TOPDIR)/iRODS/server/re/include/ -I$(TOPDIR)/iRODS/server/drivers/include/ -I$(TOPDIR)/iRODS/lib/hasher/include

BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
             $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
             $(BOOST_DIR)/stage/lib/libboost_regex.a \
             $(BOOST_DIR)/stage/lib/libboost_thread.a
LIB=-lssl -lcrypto -ldl -lpthread $(TOPDIR)/iRODS/lib/core/obj/libRodsAPIs.a $(BOOST_LIBS)
all:
	${GCC} -g ${INC} -o hasher_test HasherTest.cpp ${LIB}
