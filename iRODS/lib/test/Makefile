ifndef buildDir
buildDir = ../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk

vpath %.c src

CFLAGS_OPTIONS := -g $(CFLAGS) $(MY_CFLAG)
CFLAGS = $(CFLAGS_OPTIONS) -I$(buildDir)/server/core/include \
-I$(buildDir)/server/icat/include \
-I$(buildDir)/server/drivers/include \
-I$(buildDir)/server/re/include \
-I$(buildDir)/lib/core/include  \
-I$(buildDir)/lib/api/include -I$(buildDir)/lib/md5/include

LDFLAGS += $(LDADD) -L$(buildDir)/lib/core/obj -l$(LIBRARY_NAME)

TESTOBJS = iTestGenQuery.o luketest.o lowlevtest.o packtest.o l1test.o l1rm.o testrule.o xmltest.o \
l3structFile.o xmsgtest.o listcoll.o

TARGETS = iTestGenQuery luketest lowlevtest packtest l1test l1rm testrule xmltest l3structFile  \
xmsgtest listcoll

ifdef TAR_STRUCT_FILE
# TARGETS+=tartest
# TESTOBJS+=tartest.o
CFLAGS+= -I$(tarDir) -I$(tarDir)/listhash \
                  -I${tarDir}/lib \
                  -I${tarDir}/compat
ifdef tarDir
AG_LDADD+=-L$(tarDir)/lib -ltar
endif
endif

ifdef PHP_LIB
TARGETS+= phptest phpexttest
TESTOBJS+= phptest.o phpexttest.o
CFLAGS+=-I$(PHP_INCLUDE_DIR) -I$(PHP_INCLUDE_DIR)/main -I$(PHP_INCLUDE_DIR)/Zend -I$(PHP_INCLUDE_DIR)/TSRM -I$(PHP_INCLUDE_DIR)/regex -I$(PHP_INCLUDE_DIR)/sapi/embed
AG_LDADD+= -L/data/mwan/php/libxml2-2.6.30/.libs -lrt -lresolv -lm -ldl -lnsl -lxml2 -lz -lcrypt -liconv
endif


.PHONY:	print_cflags print_ldflags
all:: print_cflags print_ldflags $(TARGETS)



# Show compile flags
print_cflags:
	@echo "Compile flags:"
	@echo "    $(CFLAGS_OPTIONS)"

# Show link flags
print_ldflags:
	@echo "Link flags:"
	@echo "    $(LDFLAGS)"
	@echo "PHP-only link flags:"
	@echo "    $(LDFLAGS)" $(AG_LDDD)






clean:
	rm -f $(TARGETS) $(TESTOBJS)

$(TESTOBJS): %.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

lowlevtest: lowlevtest.o
	$(LDR) -o $@ $^ $(LDFLAGS)

packtest: packtest.o
	$(LDR) -o $@ $^ $(LDFLAGS)

luketest: luketest.o
	$(LDR) -o $@ $^ $(LDFLAGS)

iTestGenQuery: iTestGenQuery.o
	$(LDR) -o $@ $^ $(LDFLAGS)

l1test: l1test.o
	$(LDR) -o $@ $^ $(LDFLAGS)

testrule: testrule.o
	$(LDR) -o $@ $^ $(LDFLAGS)

l1rm: l1rm.o
	$(LDR) -o $@ $^ $(LDFLAGS)

xmltest: xmltest.o
	$(LDR) -o $@ $^ $(LDFLAGS)

l3structFile: l3structFile.o
	$(LDR) -o $@ $^ $(LDFLAGS)

xmsgtest: xmsgtest.o
	$(LDR) -o $@ $^ $(LDFLAGS)

phptest: phptest.o
	$(LDR) -o $@ $^ $(LDFLAGS) $(AG_LDADD)

phpexttest: phpexttest.o
	$(LDR) -o $@ $^ $(LDFLAGS) $(AG_LDADD)

listcoll: listcoll.o
	$(LDR) -o $@ $^ $(LDFLAGS) $(AG_LDADD)

ifdef TAR_STRUCT_FILE
tartest: tartest.o
	$(LDR) -o $@ $^ $(LDADD) $(AG_LDADD)
endif
