BASEDIRS = nonblocking \
           passthru \
           replication \
           roundrobin \
           structfile \
           univmss \
           unixfilesystem \
           compound \
           wos \
           random

HPSSDIRS = hpss

######################################################################
# Configuration should occur above this line

include ../../iRODS/config/platform.mk
include ../../iRODS/config/config.mk

GCC = g++ -DRODS_SERVER -DZIP_EXEC_PATH=\"$(ZIP_EXEC_PATH)\" -DUNZIP_EXEC_PATH=\"$(UNZIP_EXEC_PATH)\"

ifeq ($(EIRODS_BUILD_COVERAGE), 1)
GCC += -fprofile-arcs -ftest-coverage -lgcov
endif

OBJDIR = .objs
DEPDIR = .deps

SUBS = ${BASEDIRS}

ifeq ($(EIRODS_BUILD_DEBUG), 0)
#SUBS += ${HPSSDIRS}
endif

.PHONY: ${SUBS} clean

INC = -I ../../iRODS/lib/core/include/ -I ../../iRODS/lib/api/include/ -I../../iRODS/lib/md5/include \
      -I../../iRODS/server/core/include/ -I../../iRODS/server/icat/include/ -I../../iRODS/server/re/include/ \
      -I../../iRODS/server/drivers/include/ -I$(LIBARCHIVE_DIR)/libarchive/ -I$(BOOST_DIR) \
      -I/opt/hpss/include  -L/opt/hpss/lib

default: ${SUBS}

${SUBS}:
	@-mkdir $@/${OBJDIR} > /dev/null 2>&1
	@-mkdir $@/${DEPDIR} > /dev/null 2>&1
	${MAKE} -C $@

clean:
	@-for dir in ${SUBS}; do \
	rm $$dir/${OBJDIR}/*.o > /dev/null 2>&1; \
	done
	@-rm sos/*.so > /dev/null 2>&1

