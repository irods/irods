include ../../iRODS/config/platform.mk
include ../../iRODS/config/config.mk

IRODSDIR=${IRODS_BUILD_DIR}/iRODS

SRCDIR = src

SRCS= \
    $(SRCDIR)/helloworld.cpp \
    $(SRCDIR)/rsSetRoundRobinContext.cpp

MAKEFLAGS += --no-print-directory

CFLAGS += -Wno-write-strings

CFLAGS += -L${IRODS_BUILD_DIR}

EXTRAINCS += -I${IRODSDIR}/lib/core/include/ -I${IRODSDIR}/lib/api/include/ -I${IRODSDIR}/lib/hasher/include \
						-I${IRODSDIR}/server/core/include/ -I${IRODSDIR}/server/icat/include/ -I${IRODSDIR}/server/re/include/ \
						-I${IRODSDIR}/server/drivers/include/ -I$(LIBARCHIVE_DIR)/libarchive/ -I$(BOOST_DIR) \
						-I/usr/include/openssl

#BOOST_LIBS = \
#    -lboost_filesystem \
#    -lboost_system \
#    -lboost_regex \
#    -lboost_thread

#EXTRALIBS += $(BOOST_LIBS) \
#						-lirods_client \
#						-lirods_client_api \
#						-lirods_server

.PHONY: default clean

default: api-base

clean: clean-base

include ../Makefile.base
