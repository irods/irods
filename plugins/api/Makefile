include ../../iRODS/config/platform.mk
include ../../iRODS/config/config.mk
include ../../packaging/irods_externals_locations.mk

IRODSDIR=$(IRODS_BUILD_DIR)/iRODS

SRCDIR = src

SRCS= \
    $(SRCDIR)/helloworld.cpp

MAKEFLAGS += --no-print-directory

CFLAGS += -Wno-write-strings

CFLAGS += -L$(IRODS_BUILD_DIR)

BOOST_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/include
EXTRAINCS += -I$(IRODSDIR)/lib/core/include/ \
			 -I$(IRODSDIR)/lib/api/include/ \
			 -I$(IRODSDIR)/lib/hasher/include \
			 -I$(IRODSDIR)/server/core/include/ \
			 -I$(IRODSDIR)/server/icat/include/ \
			 -I$(IRODSDIR)/server/re/include/ \
			 -I$(IRODSDIR)/server/drivers/include/ \
			 -I$(LIBARCHIVE_DIR)/libarchive/ \
			 -I/usr/include/openssl \
			$(BOOST_INCLUDES)

.PHONY: default clean

default: api-base

clean: clean-base

include ../Makefile.base
