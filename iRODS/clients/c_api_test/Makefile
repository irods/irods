include ../../config/external_versions.txt

ifndef buildDir
buildDir =      $(CURDIR)/../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

GCC = gcc

CFLAGS = -I../../lib/core/include/ \
	-I../../lib/api/include/ \
	-I../../lib/md5/include/ \
	-I../../lib/sha1/include/ \
	-I../../server/icat/include/ \
	-I../../server/re/include/ \
	-I../../server/core/include/

BOOST_LIBS = $(BOOST_DIR)/stage/lib/libboost_system.a \
	$(BOOST_DIR)/stage/lib/libboost_filesystem.a \
	$(BOOST_DIR)/stage/lib/libboost_regex.a \
	$(BOOST_DIR)/stage/lib/libboost_thread.a \
	$(BOOST_DIR)/stage/lib/libboost_chrono.a \
	$(BOOST_DIR)/stage/lib/libboost_program_options.a

LDFLAGS = ../../lib/core/obj/libRodsAPIs.a $(BOOST_LIBS)

# include jansson for zonereport
CFLAGS += -I../../../external/$(JANSSONVERSION)/src/
LDFLAGS += ../../../external/$(JANSSONVERSION)/src/.libs/libjansson.a

# include for control plane
CFLAGS += -I../../../external/$(ZEROMQVERSION)/include/
LDFLAGS += ../../../external/$(ZEROMQVERSION)/src/.libs/libzmq.a

CFLAGS += -I../../../external/$(AVROVERSION)
LDFLAGS += ../../../external/$(AVROVERSION)/build/libavrocpp_s.a

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
	$(GCC) $(CFLAGS) -o mytest test.c $(LDFLAGS)

clean:
	rm -f mytest
	rm -f *.o
	rm -f *~
