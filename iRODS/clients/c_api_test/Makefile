include ../../config/external_versions.txt

ifndef buildDir
buildDir =      $(CURDIR)/../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk
include $(buildDir)/../packaging/irods_externals_locations.mk

GCC = gcc

ifeq ($(IRODS_BUILD_COVERAGE), 1)
GCC += -fprofile-arcs -ftest-coverage -lgcov
endif

LDFLAGS += $(LIBRARY) $(LIB_CLIENT_EXEC)
CFLAGS = -I../../lib/core/include/ \
	-I../../lib/api/include/ \
	-I../../lib/hasher/include/ \
	-I../../server/icat/include/ \
	-I../../server/re/include/ \
	-I../../server/core/include/ \

BOOST_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/include 
BOOST_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_BOOST)/lib \
    -lboost_system \
	-lboost_filesystem \
	-lboost_regex \
	-lboost_thread \
	-lboost_chrono \
	-lboost_program_options

JANSSON_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_JANSSON)/include 
JANSSON_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_JANSSON)/lib -ljansson

AVRO_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_AVRO)/include 
AVRO_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_AVRO)/lib  -lavrocpp

ZMQ_INCLUDES = -I$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_ZMQ)/include 
ZMQ_LIBS = -L$(IRODS_EXTERNALS_ROOT)/$(BUILD_SUBDIRECTORY_ZMQ)/lib  -lzmq

CFLAGS += $(BOOST_INCLUDES) $(JANSSON_INCLUDES) $(AVRO_INCLUDES) $(ZMQ_INCLUDES)
LDFLAGS += $(BOOST_LIBS) $(JANSSON_LIBS) $(AVRO_LIBS) $(ZMQ_LIBS)

# include miscellaneous
LDFLAGS += -lz -lssl -lcrypto -ldl -lpthread -lm

# include realtime library except for macosx (already included)
ifneq (${OS_platform}, osx_platform)
LDFLAGS += -lrt
endif

# include c++ standard library linkage
LDFLAGS += -lstdc++

# make all symbols available for dynamic linking
LDFLAGS += -rdynamic

default:
	$(V_at)$(GCC) $(CFLAGS) -o mytest test.c $(LDFLAGS)

clean:
	@echo "Cleaning C API test..."
	@rm -f mytest
	@rm -f *.o
	@rm -f *~
