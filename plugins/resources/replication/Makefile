TARGET = libreplication.so

SRCS = librepl.cpp \
       irods_replicator.cpp \
       irods_create_write_replicator.cpp \
       irods_object_oper.cpp \
       irods_repl_rebalance.cpp \
       irods_repl_retry.cpp

HEADERS = irods_create_write_replicator.hpp \
          irods_object_oper.hpp \
          irods_oper_replicator.hpp \
          irods_replicator.hpp \
          irods_repl_retry.hpp \
          irods_repl_types.hpp \
          irods_repl_rebalance.hpp

EXTRALIBS = ../../../iRODS/lib/core/obj/irods_virtual_path.o

include ../Makefile.base
