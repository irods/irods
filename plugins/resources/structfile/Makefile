TARGET = libstructfile.so

SRCS = libstructfile.cpp

HEADERS = 

EXTRALIBS = $(LIBARCHIVE_DIR)/libarchive/libarchive.a \
            $(BOOST_DIR)/stage/lib/libboost_filesystem.a \
            $(BOOST_DIR)/stage/lib/libboost_system.a \
            -L /usr/lib -lz -lbz2

include ../Makefile.base
